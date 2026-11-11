/*
 * In-driver WPA authenticator (4-way and group key handshake) for AP mode.
 *
 * Counterpart of wl_sae4way.c: with firmware offloaded SAE the PMK of a
 * station is only known to the firmware and reported to the driver, and
 * hostapd can not do the handshake without it. The driver therefore
 * advertises 4WAY_HANDSHAKE_AP_PSK + SAE_OFFLOAD_AP, does the handshake for
 * SAE and PSK stations itself and tells hostapd with PORT_AUTHORIZED.
 *
 * hostapd still owns the group keys: GTK/IGTK are captured when it installs
 * them, a changed GTK triggers the group key handshake.
 *
 * This file is #included from wl_cfg80211.c after wl_sae4way.c.
 */
#ifdef WL_SAE

#include <linux/list.h>

#define A4W_MAX_STA		16
#define A4W_RETRIES		4
#define A4W_TICK_MS		1000

#define A4W_AKM_PSK		2
#define A4W_AKM_PSK_SHA256	6
#define A4W_AKM_SAE		8

enum a4w_state {
	A4W_IDLE,
	A4W_WAIT_M2,
	A4W_WAIT_M4,
	A4W_DONE,
	A4W_WAIT_G2,
};

struct a4w_sta {
	bool used;
	u8 addr[ETH_ALEN];
	u8 akm;
	enum a4w_state state;
	bool have_pmk;
	u8 pmk[32];
	u8 pmkid[16];
	bool have_pmkid;
	u8 anonce[32];
	u8 kck[16];
	u8 kek[16];
	u8 tk[16];
	u64 replay;
	int retries;
};

enum a4w_ev_type {
	A4W_EV_PMK,
	A4W_EV_ASSOC,
	A4W_EV_DEL,
	A4W_EV_RX,
	A4W_EV_GTK,
};

struct a4w_ev {
	struct list_head list;
	enum a4w_ev_type type;
	u8 addr[ETH_ALEN];
	u8 pmk[32];
	u8 pmkid[16];
	u32 len;
	u8 data[];
};

struct a4w_ctx {
	bool init;
	bool active;			/* AP is up and uses the authenticator */
	struct mutex lock;
	spinlock_t qlock;
	struct list_head q;
	struct work_struct work;
	struct delayed_work tick;
	struct bcm_cfg80211 *cfg;
	struct net_device *ndev;
	u8 psk[32];
	bool have_psk;
	u8 rsne[64];			/* our RSNE as in the beacon */
	u32 rsne_len;
	u8 gtk[32];
	u8 gtk_len;
	u8 gtk_idx;
	bool have_gtk;
	u8 igtk[16];
	u8 igtk_idx;
	u8 igtk_ipn[6];
	bool have_igtk;
	struct a4w_sta sta[A4W_MAX_STA];
};

static struct a4w_ctx a4w;

/* ---- crypto (SHA1 variants, key wrap) ---- */

static int a4w_hmac_sha1(const u8 *key, size_t klen, const u8 *const *parts,
	const size_t *lens, int n, u8 *out)
{
	struct crypto_shash *tfm = crypto_alloc_shash("hmac(sha1)", 0, 0);
	int i, err;

	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	err = crypto_shash_setkey(tfm, key, klen);
	if (!err) {
		SHASH_DESC_ON_STACK(desc, tfm);

		desc->tfm = tfm;
		desc->flags = 0;
		err = crypto_shash_init(desc);
		for (i = 0; !err && i < n; i++)
			err = crypto_shash_update(desc, parts[i], lens[i]);
		if (!err)
			err = crypto_shash_final(desc, out);
	}
	crypto_free_shash(tfm);
	return err;
}

/* IEEE 802.11 PRF (SHA-1 based) */
static int a4w_prf_sha1(const u8 *key, size_t klen, const char *label,
	const u8 *data, size_t dlen, u8 *out, size_t outlen)
{
	u8 ctr = 0, zero = 0, hash[20];
	size_t pos = 0;
	const u8 *parts[4];
	size_t lens[4];
	int err;

	while (pos < outlen) {
		parts[0] = (const u8 *)label; lens[0] = strlen(label);
		parts[1] = &zero; lens[1] = 1;
		parts[2] = data; lens[2] = dlen;
		parts[3] = &ctr; lens[3] = 1;
		err = a4w_hmac_sha1(key, klen, parts, lens, 4, hash);
		if (err)
			return err;
		memcpy(out + pos, hash, min_t(size_t, 20, outlen - pos));
		pos += 20;
		ctr++;
	}
	return 0;
}

/* RFC 3394 AES key wrap. in: len bytes (multiple of 8, >= 16) -> out: len + 8 */
static int a4w_aes_wrap(const u8 *kek, const u8 *in, size_t len, u8 *out)
{
	struct crypto_cipher *tfm;
	size_t n = len / 8;
	u8 a[8], b[16];
	int i, j, k;

	if (len < 16 || len % 8)
		return -EINVAL;
	tfm = crypto_alloc_cipher("aes", 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	if (crypto_cipher_setkey(tfm, kek, 16)) {
		crypto_free_cipher(tfm);
		return -EINVAL;
	}
	memset(a, 0xa6, 8);
	memcpy(out + 8, in, len);
	for (j = 0; j <= 5; j++) {
		for (i = 1; i <= n; i++) {
			u32 t = n * j + i;

			memcpy(b, a, 8);
			memcpy(b + 8, out + 8 * i, 8);
			crypto_cipher_encrypt_one(tfm, b, b);
			memcpy(a, b, 8);
			for (k = 0; k < 4; k++)
				a[7 - k] ^= (t >> (8 * k)) & 0xff;
			memcpy(out + 8 * i, b + 8, 8);
		}
	}
	memcpy(out, a, 8);
	crypto_free_cipher(tfm);
	return 0;
}

/* ---- helpers ---- */

static struct a4w_sta *a4w_find(const u8 *addr, bool create)
{
	struct a4w_sta *free = NULL;
	int i;

	for (i = 0; i < A4W_MAX_STA; i++) {
		if (a4w.sta[i].used && !memcmp(a4w.sta[i].addr, addr, ETH_ALEN))
			return &a4w.sta[i];
		if (!a4w.sta[i].used && !free)
			free = &a4w.sta[i];
	}
	if (create && !free) {
		/* table full: drop a cached PMK of a station that is gone */
		for (i = 0; i < A4W_MAX_STA; i++) {
			if (a4w.sta[i].state == A4W_IDLE) {
				free = &a4w.sta[i];
				break;
			}
		}
	}
	if (create && free) {
		memset(free, 0, sizeof(*free));
		free->used = true;
		memcpy(free->addr, addr, ETH_ALEN);
		return free;
	}
	return NULL;
}

static u16 a4w_keyver(u8 akm)
{
	return akm == A4W_AKM_PSK ? 2 : (akm == A4W_AKM_PSK_SHA256 ? 3 : 0);
}

static int a4w_mic(const struct a4w_sta *s, const u8 *eapol, size_t len, u8 *mic)
{
	if (s->akm == A4W_AKM_PSK) {
		const u8 *parts[1] = { eapol };
		size_t lens[1] = { len };
		u8 full[20];
		int err = a4w_hmac_sha1(s->kck, 16, parts, lens, 1, full);

		if (!err)
			memcpy(mic, full, 16);
		return err;
	}
	return s4w_cmac(s->kck, eapol, len, mic);
}

static bool a4w_mic_ok(const struct a4w_sta *s, const u8 *e, size_t elen)
{
	u8 *tmp = kmemdup(e, elen, GFP_KERNEL);
	u8 mic[16];
	bool ok;

	if (!tmp)
		return false;
	memset(tmp + S4W_MIC_OFF, 0, 16);
	ok = !a4w_mic(s, tmp, elen, mic) && !memcmp(mic, e + S4W_MIC_OFF, 16);
	kfree(tmp);
	return ok;
}

static int a4w_derive_ptk(struct a4w_sta *s, const u8 *snonce)
{
	const u8 *aa = a4w.ndev->dev_addr, *spa = s->addr;
	u8 data[ETH_ALEN * 2 + 64], ptk[48];
	int err;

	if (memcmp(aa, spa, ETH_ALEN) < 0) {
		memcpy(data, aa, ETH_ALEN); memcpy(data + 6, spa, ETH_ALEN);
	} else {
		memcpy(data, spa, ETH_ALEN); memcpy(data + 6, aa, ETH_ALEN);
	}
	if (memcmp(s->anonce, snonce, 32) < 0) {
		memcpy(data + 12, s->anonce, 32); memcpy(data + 44, snonce, 32);
	} else {
		memcpy(data + 12, snonce, 32); memcpy(data + 44, s->anonce, 32);
	}
	if (s->akm == A4W_AKM_PSK)
		err = a4w_prf_sha1(s->pmk, 32, "Pairwise key expansion", data, 76, ptk, 48);
	else
		err = s4w_kdf_sha256(s->pmk, 32, "Pairwise key expansion", data, 76, ptk, 48);
	if (err)
		return err;
	memcpy(s->kck, ptk, 16);
	memcpy(s->kek, ptk + 16, 16);
	memcpy(s->tk, ptk + 32, 16);
	memzero_explicit(ptk, sizeof(ptk));
	return 0;
}

/* Build + send an EAPOL-Key frame from the AP to a station */
static int a4w_send(struct a4w_sta *s, u16 key_info, u64 replay, const u8 *nonce,
	u16 key_len, const u8 *kdata, u16 kdata_len, bool mic)
{
	struct net_device *ndev = a4w.ndev;
	size_t elen = S4W_HDR_LEN + S4W_FIXED_LEN + kdata_len;
	struct sk_buff *skb;
	u8 *eth, *e;

	skb = dev_alloc_skb(ETH_HLEN + elen + 128);
	if (!skb)
		return -ENOMEM;
	skb_reserve(skb, 64);
	eth = skb_put(skb, ETH_HLEN + elen);
	memcpy(eth, s->addr, ETH_ALEN);
	memcpy(eth + ETH_ALEN, ndev->dev_addr, ETH_ALEN);
	eth[12] = 0x88;
	eth[13] = 0x8e;
	e = eth + ETH_HLEN;
	memset(e, 0, elen);
	e[0] = 2;
	e[1] = S4W_EAPOL_TYPE_KEY;
	e[2] = (elen - S4W_HDR_LEN) >> 8;
	e[3] = (elen - S4W_HDR_LEN) & 0xff;
	e[4] = S4W_KEYDESC_RSN;
	e[5] = key_info >> 8;
	e[6] = key_info & 0xff;
	e[7] = key_len >> 8;
	e[8] = key_len & 0xff;
	e[9] = replay >> 56; e[10] = replay >> 48; e[11] = replay >> 40;
	e[12] = replay >> 32; e[13] = replay >> 24; e[14] = replay >> 16;
	e[15] = replay >> 8; e[16] = replay;
	if (nonce)
		memcpy(e + 17, nonce, 32);
	e[S4W_DATALEN_OFF] = kdata_len >> 8;
	e[S4W_DATALEN_OFF + 1] = kdata_len & 0xff;
	if (kdata_len)
		memcpy(e + S4W_DATALEN_OFF + 2, kdata, kdata_len);
	if (mic) {
		u8 micbuf[16];

		if (a4w_mic(s, e, elen, micbuf)) {
			kfree_skb(skb);
			return -EIO;
		}
		memcpy(e + S4W_MIC_OFF, micbuf, 16);
	}
	skb->dev = ndev;
	skb->protocol = htons(0x888e);
	skb_reset_network_header(skb);
	return dev_queue_xmit(skb);
}

static int a4w_send_m1(struct a4w_sta *s)
{
	u8 kd[2 + 4 + 16];
	u16 kdl = 0;
	int err;

	/* new ANonce for every attempt */
	get_random_bytes(s->anonce, 32);
	s->replay++;
	if (s->akm == A4W_AKM_SAE && s->have_pmkid) {
		kd[0] = 0xdd; kd[1] = 4 + 16;
		kd[2] = 0x00; kd[3] = 0x0f; kd[4] = 0xac; kd[5] = 4;
		memcpy(kd + 6, s->pmkid, 16);
		kdl = 22;
	}
	err = a4w_send(s, a4w_keyver(s->akm) | S4W_KI_PAIRWISE | S4W_KI_ACK,
		s->replay, s->anonce, 16, kd, kdl, false);
	s->state = A4W_WAIT_M2;
	return err;
}

/* GTK (+IGTK) KDEs, padded and wrapped with the KEK. Returns length or <0 */
static int a4w_wrapped_kdata(struct a4w_sta *s, bool with_rsne, u8 *out, size_t max)
{
	u8 plain[160];
	size_t n = 0;

	if (!a4w.have_gtk)
		return -ENOENT;
	if (with_rsne) {
		if (!a4w.rsne_len)
			return -ENOENT;
		memcpy(plain, a4w.rsne, a4w.rsne_len);
		n = a4w.rsne_len;
	}
	plain[n++] = 0xdd; plain[n++] = 4 + 2 + a4w.gtk_len;
	plain[n++] = 0x00; plain[n++] = 0x0f; plain[n++] = 0xac; plain[n++] = 1;
	plain[n++] = a4w.gtk_idx & 3; plain[n++] = 0;
	memcpy(plain + n, a4w.gtk, a4w.gtk_len); n += a4w.gtk_len;
	if (a4w.have_igtk) {
		plain[n++] = 0xdd; plain[n++] = 4 + 2 + 6 + 16;
		plain[n++] = 0x00; plain[n++] = 0x0f; plain[n++] = 0xac; plain[n++] = 9;
		plain[n++] = a4w.igtk_idx; plain[n++] = 0;
		memcpy(plain + n, a4w.igtk_ipn, 6); n += 6;
		memcpy(plain + n, a4w.igtk, 16); n += 16;
	}
	if (n % 8 || n < 16) {
		plain[n++] = 0xdd;
		while (n % 8 || n < 16)
			plain[n++] = 0;
	}
	if (n + 8 > max)
		return -ENOSPC;
	if (a4w_aes_wrap(s->kek, plain, n, out))
		return -EIO;
	return n + 8;
}

static void a4w_send_m3(struct a4w_sta *s)
{
	u8 kd[192];
	int kdl;

	kdl = a4w_wrapped_kdata(s, true, kd, sizeof(kd));
	if (kdl < 0) {
		WL_ERR(("A4W: M3 key data failed %d\n", kdl));
		return;
	}
	s->replay++;
	a4w_send(s, a4w_keyver(s->akm) | S4W_KI_PAIRWISE | S4W_KI_INSTALL | S4W_KI_ACK |
		S4W_KI_MIC | S4W_KI_SECURE | S4W_KI_ENC_DATA, s->replay, s->anonce,
		16, kd, kdl, true);
	s->state = A4W_WAIT_M4;
}

static void a4w_send_group_m1(struct a4w_sta *s)
{
	u8 kd[160];
	int kdl = a4w_wrapped_kdata(s, false, kd, sizeof(kd));

	if (kdl < 0)
		return;
	s->replay++;
	a4w_send(s, a4w_keyver(s->akm) | S4W_KI_ACK | S4W_KI_MIC | S4W_KI_SECURE |
		S4W_KI_ENC_DATA, s->replay, NULL, 16, kd, kdl, true);
	s->state = A4W_WAIT_G2;
}

static int a4w_install(struct a4w_sta *s)
{
	return s4w_install_key_dev(a4w.ndev, 0, true, s->addr, WLAN_CIPHER_SUITE_CCMP,
		s->tk, 16, NULL, 0);
}

/* ---- event handling (worker context) ---- */

static void a4w_ev_assoc(const u8 *addr, const u8 *ies, u32 len)
{
	struct a4w_sta *s;
	const u8 *rsne;
	u8 akm;

	rsne = s4w_find_ie(ies, len, 0x30);
	if (!rsne || rsne[1] < 20) {
		WL_INFORM_MEM(("A4W: " MACDBG " assoc without RSNE\n", MAC2STRDBG(addr)));
		return;
	}
	/* RSN IE: ver(2) group(4) pcount(2) pairwise(4*n) acount(2) akm(4*n)... */
	{
		u16 pc = rsne[8] | (rsne[9] << 8);
		u32 off = 10 + 4 * pc;

		if (off + 2 + 4 > (u32)rsne[1] + 2) {
			WL_ERR(("A4W: bad RSNE\n"));
			return;
		}
		akm = rsne[off + 2 + 3];
	}
	s = a4w_find(addr, true);
	if (!s) {
		WL_ERR(("A4W: no free station slot\n"));
		return;
	}
	s->akm = akm;
	s->retries = 0;
	s->replay = 0;
	if (akm == A4W_AKM_SAE) {
		if (!s->have_pmk) {
			WL_ERR(("A4W: no SAE PMK for " MACDBG "\n", MAC2STRDBG(addr)));
			s->state = A4W_IDLE;
			return;
		}
	} else if (akm == A4W_AKM_PSK || akm == A4W_AKM_PSK_SHA256) {
		if (!a4w.have_psk) {
			WL_ERR(("A4W: no PSK for " MACDBG "\n", MAC2STRDBG(addr)));
			return;
		}
		memcpy(s->pmk, a4w.psk, 32);
		s->have_pmk = true;
		s->have_pmkid = false;
	} else {
		WL_ERR(("A4W: unsupported AKM %d\n", akm));
		return;
	}
	WL_INFORM_MEM(("A4W: " MACDBG " akm %d -> M1: %d\n", MAC2STRDBG(addr), akm,
		a4w_send_m1(s)));
}

static void a4w_ev_rx(const u8 *frame, u32 flen)
{
	const u8 *e = frame + ETH_HLEN;
	size_t elen = flen - ETH_HLEN;
	u16 ki = (e[5] << 8) | e[6];
	u16 dl = (e[S4W_DATALEN_OFF] << 8) | e[S4W_DATALEN_OFF + 1];
	u64 replay = be64_to_cpup((__be64 *)(e + 9));
	struct a4w_sta *s = a4w_find(frame + ETH_ALEN, false);
	int err;

	if (!s || s->state == A4W_IDLE)
		return;
	if (4 + ((e[2] << 8) | e[3]) < elen)
		elen = 4 + ((e[2] << 8) | e[3]);
	if (elen < S4W_HDR_LEN + S4W_FIXED_LEN || S4W_HDR_LEN + S4W_FIXED_LEN + dl > elen)
		return;
	if (!(ki & S4W_KI_MIC) || (ki & (S4W_KI_ACK | S4W_KI_ERROR | S4W_KI_REQUEST)))
		return;
	if (replay != s->replay) {
		WL_ERR(("A4W: replay counter mismatch from " MACDBG "\n", MAC2STRDBG(s->addr)));
		return;
	}

	if ((ki & S4W_KI_PAIRWISE) && s->state == A4W_WAIT_M2 && !(ki & S4W_KI_SECURE)) {
		if (a4w_derive_ptk(s, e + 17)) {
			WL_ERR(("A4W: PTK derivation failed\n"));
			return;
		}
		if (!a4w_mic_ok(s, e, elen)) {
			WL_ERR(("A4W: M2 MIC failure from " MACDBG "\n", MAC2STRDBG(s->addr)));
			return;
		}
		s->retries = 0;
		a4w_send_m3(s);
		WL_INFORM_MEM(("A4W: " MACDBG " M2 ok -> M3\n", MAC2STRDBG(s->addr)));
	} else if ((ki & S4W_KI_PAIRWISE) && s->state == A4W_WAIT_M4 && (ki & S4W_KI_SECURE)) {
		if (!a4w_mic_ok(s, e, elen)) {
			WL_ERR(("A4W: M4 MIC failure from " MACDBG "\n", MAC2STRDBG(s->addr)));
			return;
		}
		err = a4w_install(s);
		WL_INFORM_MEM(("A4W: " MACDBG " PTK install: %d\n", MAC2STRDBG(s->addr), err));
		err = wldev_ioctl_set(a4w.ndev, WLC_SCB_AUTHORIZE, s->addr, ETH_ALEN);
		WL_INFORM_MEM(("A4W: authorize: %d\n", err));
		s->state = A4W_DONE;
		s->retries = 0;
		cfg80211_port_authorized(a4w.ndev, s->addr, GFP_KERNEL);
	} else if (!(ki & S4W_KI_PAIRWISE) && s->state == A4W_WAIT_G2 && (ki & S4W_KI_SECURE)) {
		if (a4w_mic_ok(s, e, elen)) {
			s->state = A4W_DONE;
			s->retries = 0;
		}
	}
}

static void a4w_ev_gtk(void)
{
	int i;

	for (i = 0; i < A4W_MAX_STA; i++) {
		struct a4w_sta *s = &a4w.sta[i];

		if (s->used && (s->state == A4W_DONE || s->state == A4W_WAIT_G2)) {
			s->retries = 0;
			a4w_send_group_m1(s);
		}
	}
}

static bool a4w_any_pending(void)
{
	int i;

	for (i = 0; i < A4W_MAX_STA; i++)
		if (a4w.sta[i].used && (a4w.sta[i].state == A4W_WAIT_M2 ||
		    a4w.sta[i].state == A4W_WAIT_M4 || a4w.sta[i].state == A4W_WAIT_G2))
			return true;
	return false;
}

static void a4w_work(struct work_struct *w)
{
	struct a4w_ev *ev;

	for (;;) {
		unsigned long fl;

		spin_lock_irqsave(&a4w.qlock, fl);
		if (list_empty(&a4w.q)) {
			spin_unlock_irqrestore(&a4w.qlock, fl);
			break;
		}
		ev = list_first_entry(&a4w.q, struct a4w_ev, list);
		list_del(&ev->list);
		spin_unlock_irqrestore(&a4w.qlock, fl);

		mutex_lock(&a4w.lock);
		if (a4w.active) {
			switch (ev->type) {
			case A4W_EV_PMK: {
				struct a4w_sta *s = a4w_find(ev->addr, true);

				if (s) {
					memcpy(s->pmk, ev->pmk, 32);
					s->have_pmk = true;
					memcpy(s->pmkid, ev->pmkid, 16);
					s->have_pmkid = true;
				}
				break;
			}
			case A4W_EV_ASSOC:
				a4w_ev_assoc(ev->addr, ev->data, ev->len);
				break;
			case A4W_EV_DEL: {
				struct a4w_sta *s = a4w_find(ev->addr, false);

				if (s) {
					/* keep the PMK: a SAE station may reconnect with
					 * PMKSA caching, then the firmware sends no PMK
					 */
					s->state = A4W_IDLE;
					memzero_explicit(s->kck, sizeof(s->kck));
					memzero_explicit(s->kek, sizeof(s->kek));
					memzero_explicit(s->tk, sizeof(s->tk));
				}
				break;
			}
			case A4W_EV_RX:
				a4w_ev_rx(ev->data, ev->len);
				break;
			case A4W_EV_GTK:
				a4w_ev_gtk();
				break;
			}
			if (a4w_any_pending())
				schedule_delayed_work(&a4w.tick, msecs_to_jiffies(A4W_TICK_MS));
		}
		mutex_unlock(&a4w.lock);
		kfree(ev);
	}
}

/* retransmissions */
static void a4w_tick(struct work_struct *w)
{
	int i;

	mutex_lock(&a4w.lock);
	if (!a4w.active) {
		mutex_unlock(&a4w.lock);
		return;
	}
	for (i = 0; i < A4W_MAX_STA; i++) {
		struct a4w_sta *s = &a4w.sta[i];

		if (!s->used)
			continue;
		if (s->state != A4W_WAIT_M2 && s->state != A4W_WAIT_M4 &&
		    s->state != A4W_WAIT_G2)
			continue;
		if (++s->retries > A4W_RETRIES) {
			WL_ERR(("A4W: " MACDBG " handshake timeout\n", MAC2STRDBG(s->addr)));
			s->state = A4W_IDLE;
			continue;
		}
		if (s->state == A4W_WAIT_M2)
			a4w_send_m1(s);
		else if (s->state == A4W_WAIT_M4)
			a4w_send_m3(s);
		else
			a4w_send_group_m1(s);
	}
	if (a4w_any_pending())
		schedule_delayed_work(&a4w.tick, msecs_to_jiffies(A4W_TICK_MS));
	mutex_unlock(&a4w.lock);
}

static void a4w_queue(enum a4w_ev_type type, const u8 *addr, const u8 *data, u32 len,
	const u8 *pmk, const u8 *pmkid)
{
	struct a4w_ev *ev;
	unsigned long fl;

	if (!a4w.init || !a4w.active)
		return;
	ev = kzalloc(sizeof(*ev) + len, GFP_ATOMIC);
	if (!ev)
		return;
	ev->type = type;
	if (addr)
		memcpy(ev->addr, addr, ETH_ALEN);
	if (pmk)
		memcpy(ev->pmk, pmk, 32);
	if (pmkid)
		memcpy(ev->pmkid, pmkid, 16);
	ev->len = len;
	if (len)
		memcpy(ev->data, data, len);
	spin_lock_irqsave(&a4w.qlock, fl);
	list_add_tail(&ev->list, &a4w.q);
	spin_unlock_irqrestore(&a4w.qlock, fl);
	schedule_work(&a4w.work);
}

/* ---- hooks ---- */

static void a4w_init_once(void)
{
	if (a4w.init)
		return;
	mutex_init(&a4w.lock);
	spin_lock_init(&a4w.qlock);
	INIT_LIST_HEAD(&a4w.q);
	INIT_WORK(&a4w.work, a4w_work);
	INIT_DELAYED_WORK(&a4w.tick, a4w_tick);
	a4w.init = true;
}

/* start_ap: remember PSK and our RSNE, enable the authenticator */
void wl_ap4way_start(struct bcm_cfg80211 *cfg, struct net_device *ndev,
	const u8 *psk, const u8 *rsne, u32 rsne_len)
{
	a4w_init_once();
	mutex_lock(&a4w.lock);
	memset(a4w.sta, 0, sizeof(a4w.sta));
	a4w.cfg = cfg;
	a4w.ndev = ndev;
	a4w.have_psk = !!psk;
	if (psk)
		memcpy(a4w.psk, psk, 32);
	a4w.rsne_len = 0;
	if (rsne && rsne_len && rsne_len <= sizeof(a4w.rsne)) {
		memcpy(a4w.rsne, rsne, rsne_len);
		a4w.rsne_len = rsne_len;
	}
	a4w.have_gtk = false;
	a4w.have_igtk = false;
	a4w.active = true;
	mutex_unlock(&a4w.lock);
	WL_INFORM_MEM(("A4W: authenticator up, psk %d rsne %u\n", !!psk, rsne_len));
}

void wl_ap4way_stop(void)
{
	unsigned long fl;
	struct a4w_ev *ev, *tmp;
	LIST_HEAD(tmpl);

	if (!a4w.init)
		return;
	a4w.active = false;
	cancel_delayed_work_sync(&a4w.tick);
	cancel_work_sync(&a4w.work);
	spin_lock_irqsave(&a4w.qlock, fl);
	list_splice_init(&a4w.q, &tmpl);
	spin_unlock_irqrestore(&a4w.qlock, fl);
	list_for_each_entry_safe(ev, tmp, &tmpl, list) {
		list_del(&ev->list);
		kfree(ev);
	}
	mutex_lock(&a4w.lock);
	memzero_explicit(a4w.sta, sizeof(a4w.sta));
	memzero_explicit(a4w.psk, sizeof(a4w.psk));
	memzero_explicit(a4w.gtk, sizeof(a4w.gtk));
	memzero_explicit(a4w.igtk, sizeof(a4w.igtk));
	mutex_unlock(&a4w.lock);
}

void wl_ap4way_pmk(const u8 *peer, const u8 *pmk, u32 pmk_len, const u8 *pmkid,
	u32 pmkid_len)
{
	u8 p[32] = {0}, id[16] = {0};

	if (pmk_len != 32)
		return;
	memcpy(p, pmk, 32);
	if (pmkid && pmkid_len >= 16)
		memcpy(id, pmkid, 16);
	a4w_queue(A4W_EV_PMK, peer, NULL, 0, p, id);
}

void wl_ap4way_assoc(const u8 *peer, const u8 *ies, u32 len)
{
	a4w_queue(A4W_EV_ASSOC, peer, ies, len, NULL, NULL);
}

void wl_ap4way_del(const u8 *peer)
{
	a4w_queue(A4W_EV_DEL, peer, NULL, 0, NULL, NULL);
}

/* hostapd installed a group key: remember it, a change triggers rekeying */
void wl_ap4way_group_key(u8 idx, u32 cipher, const u8 *key, int klen, const u8 *seq,
	int seqlen)
{
	bool changed = false;

	if (!a4w.init || !a4w.active)
		return;
	mutex_lock(&a4w.lock);
	if (cipher == WLAN_CIPHER_SUITE_AES_CMAC) {
		if (klen == 16) {
			a4w.igtk_idx = idx;
			memcpy(a4w.igtk, key, 16);
			memset(a4w.igtk_ipn, 0, 6);
			if (seq && seqlen >= 6)
				memcpy(a4w.igtk_ipn, seq, 6);
			a4w.have_igtk = true;
		}
	} else if (klen > 0 && klen <= (int)sizeof(a4w.gtk)) {
		changed = a4w.have_gtk && (a4w.gtk_idx != idx ||
			memcmp(a4w.gtk, key, klen));
		a4w.gtk_idx = idx;
		a4w.gtk_len = klen;
		memcpy(a4w.gtk, key, klen);
		a4w.have_gtk = true;
	}
	mutex_unlock(&a4w.lock);
	if (changed)
		a4w_queue(A4W_EV_GTK, NULL, NULL, 0, NULL, NULL);
}

bool wl_ap4way_rx(struct net_device *ndev, const u8 *frame, u32 len)
{
	const u8 *e;
	bool known = false;
	int i;

	if (!a4w.init || !a4w.active || ndev != a4w.ndev)
		return false;
	if (len < ETH_HLEN + S4W_HDR_LEN + S4W_FIXED_LEN || len > S4W_MAX_FRAME)
		return false;
	e = frame + ETH_HLEN;
	if (e[1] != S4W_EAPOL_TYPE_KEY || e[4] != S4W_KEYDESC_RSN)
		return false;
	/* the table is only changed in worker context, a stale read here
	 * at worst makes us pass a frame on to hostapd
	 */
	for (i = 0; i < A4W_MAX_STA; i++) {
		if (a4w.sta[i].used && !memcmp(a4w.sta[i].addr, frame + ETH_ALEN, ETH_ALEN)) {
			known = true;
			break;
		}
	}
	if (!known)
		return false;
	a4w_queue(A4W_EV_RX, frame + ETH_ALEN, frame, len, NULL, NULL);
	return true;
}

#endif /* WL_SAE */
