/*
 * In-driver WPA 4-way / group key handshake for firmware offloaded SAE.
 *
 * The firmware of this chip does SAE authentication itself (WL_SAE) but
 * has no in-dongle supplicant (no "sup_wpa") to continue with the 4-way
 * handshake, and an unmodified wpa_supplicant has no way to get the PMK the
 * firmware derived. So the handshake is done here: the PMK is taken from the
 * firmware's SAE key event, EAPOL-Key frames from the AP are consumed before
 * they reach user space, the keys are installed through the regular
 * add_key path and user space is told with a "port authorized" event.
 *
 * This file is #included from wl_cfg80211.c (it needs its static helpers).
 */
#ifdef WL_SAE

#include <crypto/hash.h>
#include <linux/crypto.h>
#include <linux/random.h>
#include <linux/etherdevice.h>

#define S4W_EAPOL_VER_OFF	0
#define S4W_EAPOL_TYPE_KEY	3
#define S4W_KEYDESC_RSN		2

#define S4W_KI_PAIRWISE		0x0008
#define S4W_KI_INSTALL		0x0040
#define S4W_KI_ACK		0x0080
#define S4W_KI_MIC		0x0100
#define S4W_KI_SECURE		0x0200
#define S4W_KI_ERROR		0x0400
#define S4W_KI_REQUEST		0x0800
#define S4W_KI_ENC_DATA		0x1000

/* eapol hdr (4) + key descriptor fixed part (95) */
#define S4W_HDR_LEN		4
#define S4W_FIXED_LEN		95
#define S4W_MIC_OFF		(S4W_HDR_LEN + 1 + 2 + 2 + 8 + 32 + 16 + 8 + 8)
#define S4W_DATALEN_OFF		(S4W_MIC_OFF + 16)
#define S4W_MAX_FRAME		512

struct sae4w_state {
	struct mutex lock;
	bool active;		/* SAE PMK known, handshake(s) are ours */
	bool keyed;		/* pairwise key installed */
	struct bcm_cfg80211 *cfg;
	struct net_device *ndev;
	u8 bssid[ETH_ALEN];
	u8 pmk[64];
	u32 pmk_len;
	u8 anonce[32];
	u8 snonce[32];
	u8 kck[16];
	u8 kek[16];
	u8 tk[16];
	u64 replay;
	bool have_replay;
	bool got_m1;
	struct sk_buff_head q;
	struct work_struct work;
	bool init;
};

static struct sae4w_state sae4w;

/* ---- crypto helpers ---- */

static int s4w_hmac_sha256(const u8 *key, size_t klen, const u8 *const *parts,
	const size_t *lens, int n, u8 *out)
{
	struct crypto_shash *tfm = crypto_alloc_shash("hmac(sha256)", 0, 0);
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

/* IEEE 802.11 KDF (SHA-256 based PRF) */
static int s4w_kdf_sha256(const u8 *key, size_t klen, const char *label,
	const u8 *data, size_t dlen, u8 *out, size_t outlen)
{
	u16 counter = 1;
	size_t pos = 0;
	u8 hash[32];
	u8 ctr[2], lenbits[2];
	const u8 *parts[4];
	size_t lens[4];
	int err;

	lenbits[0] = (outlen * 8) & 0xff;
	lenbits[1] = ((outlen * 8) >> 8) & 0xff;

	while (pos < outlen) {
		ctr[0] = counter & 0xff;
		ctr[1] = counter >> 8;
		parts[0] = ctr;	lens[0] = 2;
		parts[1] = (const u8 *)label; lens[1] = strlen(label);
		parts[2] = data; lens[2] = dlen;
		parts[3] = lenbits; lens[3] = 2;
		err = s4w_hmac_sha256(key, klen, parts, lens, 4, hash);
		if (err)
			return err;
		memcpy(out + pos, hash, min_t(size_t, 32, outlen - pos));
		pos += 32;
		counter++;
	}
	return 0;
}

static int s4w_cmac(const u8 *key, const u8 *data, size_t len, u8 *out)
{
	struct crypto_shash *tfm = crypto_alloc_shash("cmac(aes)", 0, 0);
	int err;

	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	err = crypto_shash_setkey(tfm, key, 16);
	if (!err) {
		SHASH_DESC_ON_STACK(desc, tfm);

		desc->tfm = tfm;
		desc->flags = 0;
		err = crypto_shash_digest(desc, data, len, out);
	}
	crypto_free_shash(tfm);
	return err;
}

/* RFC 3394 AES key unwrap. in: len bytes (multiple of 8, >= 24) */
static int s4w_aes_unwrap(const u8 *kek, const u8 *in, size_t len, u8 *out)
{
	struct crypto_cipher *tfm;
	u8 a[8], b[16];
	size_t n = len / 8 - 1;
	int i, j, k;

	if (len < 24 || len % 8)
		return -EINVAL;
	tfm = crypto_alloc_cipher("aes", 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	if (crypto_cipher_setkey(tfm, kek, 16)) {
		crypto_free_cipher(tfm);
		return -EINVAL;
	}
	memcpy(a, in, 8);
	memcpy(out, in + 8, len - 8);
	for (j = 5; j >= 0; j--) {
		for (i = n; i >= 1; i--) {
			u32 t = n * j + i;

			memcpy(b, a, 8);
			for (k = 0; k < 4; k++)
				b[7 - k] ^= (t >> (8 * k)) & 0xff;
			memcpy(b + 8, out + 8 * (i - 1), 8);
			crypto_cipher_decrypt_one(tfm, b, b);
			memcpy(a, b, 8);
			memcpy(out + 8 * (i - 1), b + 8, 8);
		}
	}
	crypto_free_cipher(tfm);
	for (i = 0; i < 8; i++)
		if (a[i] != 0xa6)
			return -EBADMSG;
	return 0;
}

/* ---- frame helpers ---- */

static int s4w_mic_ok(const u8 *eapol, size_t eapol_len, const u8 *kck)
{
	u8 *tmp = kmemdup(eapol, eapol_len, GFP_KERNEL);
	u8 mic[16];
	int err, ok;

	if (!tmp)
		return 0;
	memset(tmp + S4W_MIC_OFF, 0, 16);
	err = s4w_cmac(kck, tmp, eapol_len, mic);
	ok = !err && !memcmp(mic, eapol + S4W_MIC_OFF, 16);
	kfree(tmp);
	return ok;
}

/* Build and send an EAPOL-Key frame, MIC'ed with the KCK */
static int s4w_send_key(struct net_device *ndev, const u8 *bssid, u16 key_info,
	u64 replay, const u8 *nonce, const u8 *kdata, u16 kdata_len, bool mic)
{
	size_t elen = S4W_HDR_LEN + S4W_FIXED_LEN + kdata_len;
	struct sk_buff *skb;
	u8 *eth, *e;
	u8 micbuf[16];

	skb = dev_alloc_skb(ETH_HLEN + elen + 128);
	if (!skb)
		return -ENOMEM;
	skb_reserve(skb, 64);
	eth = skb_put(skb, ETH_HLEN + elen);
	memcpy(eth, bssid, ETH_ALEN);
	memcpy(eth + ETH_ALEN, ndev->dev_addr, ETH_ALEN);
	eth[12] = 0x88;
	eth[13] = 0x8e;
	e = eth + ETH_HLEN;
	memset(e, 0, elen);
	e[0] = 2;			/* 802.1X-2004 */
	e[1] = S4W_EAPOL_TYPE_KEY;
	e[2] = (elen - S4W_HDR_LEN) >> 8;
	e[3] = (elen - S4W_HDR_LEN) & 0xff;
	e[4] = S4W_KEYDESC_RSN;
	e[5] = key_info >> 8;
	e[6] = key_info & 0xff;
	/* key length 0 for RSN */
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
		if (s4w_cmac(sae4w.kck, e, elen, micbuf)) {
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

/* Find first element id in IE buffer */
static const u8 *s4w_find_ie(const u8 *ies, u32 len, u8 id)
{
	while (len >= 2) {
		u32 l = ies[1];

		if (l + 2 > len)
			break;
		if (ies[0] == id)
			return ies;
		ies += l + 2;
		len -= l + 2;
	}
	return NULL;
}

static int s4w_install_key(struct net_device *ndev, u8 idx, bool pairwise,
	const u8 *mac, u32 cipher, const u8 *key, int klen, const u8 *seq, int seqlen)
{
	struct bcm_cfg80211 *cfg = sae4w.cfg;
	struct key_params kp;

	memset(&kp, 0, sizeof(kp));
	kp.key = key;
	kp.key_len = klen;
	kp.cipher = cipher;
	kp.seq = seq;
	kp.seq_len = seqlen;
	return wl_cfg80211_add_key(bcmcfg_to_wiphy(cfg), ndev, idx, pairwise, mac, &kp);
}

static int s4w_install_key_dev(struct net_device *ndev, u8 idx, bool pairwise,
	const u8 *mac, u32 cipher, const u8 *key, int klen, const u8 *seq, int seqlen)
{
	struct bcm_cfg80211 *cfg = wl_get_cfg(ndev);
	struct key_params kp;

	memset(&kp, 0, sizeof(kp));
	kp.key = key;
	kp.key_len = klen;
	kp.cipher = cipher;
	kp.seq = seq;
	kp.seq_len = seqlen;
	return wl_cfg80211_add_key(bcmcfg_to_wiphy(cfg), ndev, idx, pairwise, mac, &kp);
}

/* Parse (decrypted) key data KDEs and install GTK / IGTK */
static void s4w_install_group_keys(struct net_device *ndev, const u8 *kd, u32 len,
	const u8 *rsc)
{
	while (len >= 2) {
		u32 l = kd[1];
		const u8 *d = kd + 2;

		if (kd[0] == 0xdd && l == 0)
			break;		/* padding */
		if (l + 2 > len)
			break;
		if (kd[0] == 0xdd && l >= 6 && d[0] == 0x00 && d[1] == 0x0f && d[2] == 0xac) {
			if (d[3] == 1 && l >= 6 + 16) {		/* GTK */
				u8 idx = d[4] & 3;
				int err = s4w_install_key(ndev, idx, false, NULL,
					WLAN_CIPHER_SUITE_CCMP, d + 6, l - 6, rsc, 6);

				WL_INFORM_MEM(("SAE4W: GTK idx %d len %d: %d\n",
					idx, l - 6, err));
			} else if (d[3] == 9 && l >= 4 + 2 + 6 + 16) {	/* IGTK */
				u16 idx = d[4] | (d[5] << 8);
				int err = s4w_install_key(ndev, (u8)idx, false, NULL,
					WLAN_CIPHER_SUITE_AES_CMAC, d + 12, l - 12, d + 6, 6);

				WL_INFORM_MEM(("SAE4W: IGTK idx %d: %d\n", idx, err));
			}
		}
		kd += l + 2;
		len -= l + 2;
	}
}

static void s4w_reset_locked(void)
{
	sae4w.active = false;
	sae4w.keyed = false;
	sae4w.got_m1 = false;
	sae4w.have_replay = false;
	sae4w.pmk_len = 0;
	memzero_explicit(sae4w.pmk, sizeof(sae4w.pmk));
	memzero_explicit(sae4w.kck, sizeof(sae4w.kck));
	memzero_explicit(sae4w.kek, sizeof(sae4w.kek));
	memzero_explicit(sae4w.tk, sizeof(sae4w.tk));
}

static void s4w_handle_m1(const u8 *e, size_t elen)
{
	struct net_device *ndev = sae4w.ndev;
	struct bcm_cfg80211 *cfg = sae4w.cfg;
	struct wl_connect_info *ci = wl_to_conn(cfg);
	u8 ptk[48];
	u8 data[ETH_ALEN * 2 + 64];
	const u8 *aa = sae4w.bssid, *spa = ndev->dev_addr, *rsne;
	u8 *kdata;
	int err, i;

	memcpy(sae4w.anonce, e + 17, 32);
	sae4w.replay = be64_to_cpup((__be64 *)(e + 9));
	sae4w.have_replay = true;
	get_random_bytes(sae4w.snonce, 32);

	/* PTK = KDF(PMK, "Pairwise key expansion", min(AA,SPA)|max|min(N)|max) */
	if (memcmp(aa, spa, ETH_ALEN) < 0) {
		memcpy(data, aa, ETH_ALEN); memcpy(data + 6, spa, ETH_ALEN);
	} else {
		memcpy(data, spa, ETH_ALEN); memcpy(data + 6, aa, ETH_ALEN);
	}
	if (memcmp(sae4w.anonce, sae4w.snonce, 32) < 0) {
		memcpy(data + 12, sae4w.anonce, 32); memcpy(data + 44, sae4w.snonce, 32);
	} else {
		memcpy(data + 12, sae4w.snonce, 32); memcpy(data + 44, sae4w.anonce, 32);
	}
	err = s4w_kdf_sha256(sae4w.pmk, sae4w.pmk_len, "Pairwise key expansion",
		data, 76, ptk, 48);
	if (err) {
		WL_ERR(("SAE4W: PTK derivation failed %d\n", err));
		return;
	}
	memcpy(sae4w.kck, ptk, 16);
	memcpy(sae4w.kek, ptk + 16, 16);
	memcpy(sae4w.tk, ptk + 32, 16);
	memzero_explicit(ptk, sizeof(ptk));

	/* M2 key data: RSNE from our association request. The event thread
	 * reads the assoc IEs (wl_get_assoc_ies) when it handles the assoc
	 * event; do not race it on cfg->extra_buf, just wait for the result.
	 */
	for (i = 0; i < 100; i++) {
		if (ci->req_ie_len &&
		    (rsne = s4w_find_ie(ci->req_ie, ci->req_ie_len, 0x30)) != NULL)
			break;
		rsne = NULL;
		msleep(10);
	}
	if (!rsne) {
		WL_ERR(("SAE4W: no RSNE in assoc req IEs\n"));
		return;
	}
	kdata = kmemdup(rsne, rsne[1] + 2, GFP_KERNEL);
	if (!kdata)
		return;
	err = s4w_send_key(ndev, aa, S4W_KI_PAIRWISE | S4W_KI_MIC, sae4w.replay,
		sae4w.snonce, kdata, rsne[1] + 2, true);
	kfree(kdata);
	sae4w.got_m1 = true;
	WL_INFORM_MEM(("SAE4W: M1 -> M2 sent: %d\n", err));
}

static void s4w_handle_m3(const u8 *e, size_t elen)
{
	struct net_device *ndev = sae4w.ndev;
	struct bcm_cfg80211 *cfg = sae4w.cfg;
	u16 dlen = (e[S4W_DATALEN_OFF] << 8) | e[S4W_DATALEN_OFF + 1];
	u64 replay = be64_to_cpup((__be64 *)(e + 9));
	u8 *plain;
	int err;

	if (!sae4w.got_m1 || memcmp(e + 17, sae4w.anonce, 32)) {
		WL_ERR(("SAE4W: M3 without matching M1\n"));
		return;
	}
	if (sae4w.have_replay && replay <= sae4w.replay) {
		WL_ERR(("SAE4W: M3 replay counter not increasing\n"));
		return;
	}
	if (!s4w_mic_ok(e, elen, sae4w.kck)) {
		WL_ERR(("SAE4W: M3 MIC failure\n"));
		return;
	}
	sae4w.replay = replay;
	plain = kzalloc(dlen, GFP_KERNEL);
	if (!plain)
		return;
	err = s4w_aes_unwrap(sae4w.kek, e + S4W_DATALEN_OFF + 2, dlen, plain);
	if (err) {
		WL_ERR(("SAE4W: key data unwrap failed %d\n", err));
		kfree(plain);
		return;
	}

	/* M4 first (unencrypted), then install the keys */
	err = s4w_send_key(ndev, sae4w.bssid, S4W_KI_PAIRWISE | S4W_KI_MIC | S4W_KI_SECURE,
		replay, NULL, NULL, 0, true);
	WL_INFORM_MEM(("SAE4W: M3 -> M4 sent: %d\n", err));
	msleep(20);

	err = s4w_install_key(ndev, 0, true, sae4w.bssid, WLAN_CIPHER_SUITE_CCMP,
		sae4w.tk, 16, NULL, 0);
	WL_INFORM_MEM(("SAE4W: PTK install: %d\n", err));
	s4w_install_group_keys(ndev, plain, dlen - 8, e + 65);
	kfree(plain);

	err = wldev_ioctl_set(ndev, WLC_SCB_AUTHORIZE, sae4w.bssid, ETH_ALEN);
	WL_INFORM_MEM(("SAE4W: authorize: %d\n", err));
	sae4w.keyed = true;
	cfg80211_port_authorized(ndev, sae4w.bssid, GFP_KERNEL);
	BCM_REFERENCE(cfg);
}

static void s4w_handle_group_m1(const u8 *e, size_t elen)
{
	struct net_device *ndev = sae4w.ndev;
	u16 dlen = (e[S4W_DATALEN_OFF] << 8) | e[S4W_DATALEN_OFF + 1];
	u64 replay = be64_to_cpup((__be64 *)(e + 9));
	u8 *plain;
	int err;

	if (sae4w.have_replay && replay <= sae4w.replay) {
		WL_ERR(("SAE4W: group M1 replay counter not increasing\n"));
		return;
	}
	if (!s4w_mic_ok(e, elen, sae4w.kck)) {
		WL_ERR(("SAE4W: group M1 MIC failure\n"));
		return;
	}
	sae4w.replay = replay;
	plain = kzalloc(dlen, GFP_KERNEL);
	if (!plain)
		return;
	if (s4w_aes_unwrap(sae4w.kek, e + S4W_DATALEN_OFF + 2, dlen, plain)) {
		WL_ERR(("SAE4W: group key data unwrap failed\n"));
		kfree(plain);
		return;
	}
	s4w_install_group_keys(ndev, plain, dlen - 8, e + 65);
	kfree(plain);
	err = s4w_send_key(ndev, sae4w.bssid, S4W_KI_MIC | S4W_KI_SECURE, replay,
		NULL, NULL, 0, true);
	WL_INFORM_MEM(("SAE4W: group M1 -> M2 sent: %d\n", err));
}

static void s4w_work(struct work_struct *w)
{
	struct sk_buff *skb;

	while ((skb = skb_dequeue(&sae4w.q)) != NULL) {
		const u8 *e = skb->data + ETH_HLEN;
		size_t elen = skb->len - ETH_HLEN;
		u16 ki = (e[5] << 8) | e[6];
		u16 dl = (e[S4W_DATALEN_OFF] << 8) | e[S4W_DATALEN_OFF + 1];

		/* trim ethernet padding, validate lengths */
		if (4 + ((e[2] << 8) | e[3]) < elen)
			elen = 4 + ((e[2] << 8) | e[3]);
		if (elen < S4W_HDR_LEN + S4W_FIXED_LEN ||
		    S4W_HDR_LEN + S4W_FIXED_LEN + dl > elen ||
		    ((ki & S4W_KI_ENC_DATA) && (dl < 24 || (dl % 8)))) {
			WL_ERR(("SAE4W: malformed key frame dropped\n"));
			kfree_skb(skb);
			continue;
		}

			mutex_lock(&sae4w.lock);
		if (sae4w.active && !memcmp(skb->data + ETH_ALEN, sae4w.bssid, ETH_ALEN)) {
			if ((ki & S4W_KI_PAIRWISE) && (ki & S4W_KI_ACK) &&
			    !(ki & S4W_KI_MIC) && !(ki & S4W_KI_SECURE)) {
				s4w_handle_m1(e, elen);
			} else if ((ki & S4W_KI_PAIRWISE) && (ki & S4W_KI_ACK) &&
				   (ki & S4W_KI_MIC) && (ki & S4W_KI_INSTALL)) {
				s4w_handle_m3(e, elen);
			} else if (!(ki & S4W_KI_PAIRWISE) && (ki & S4W_KI_ACK) &&
				   (ki & S4W_KI_MIC) && sae4w.keyed) {
				s4w_handle_group_m1(e, elen);
			} else {
				WL_ERR(("SAE4W: unexpected key frame, key_info 0x%x\n", ki));
			}
		}
		mutex_unlock(&sae4w.lock);
		kfree_skb(skb);
	}
}

/* ---- hooks ---- */

bool wl_ap4way_rx(struct net_device *ndev, const u8 *frame, u32 len);

static void wl_sae4way_init(void)
{
	if (sae4w.init)
		return;
	mutex_init(&sae4w.lock);
	skb_queue_head_init(&sae4w.q);
	INIT_WORK(&sae4w.work, s4w_work);
	sae4w.init = true;
}

/* Called with the PMK from the firmware's SAE success event */
void wl_sae4way_start(struct bcm_cfg80211 *cfg, struct net_device *ndev,
	const u8 *bssid, const u8 *pmk, u32 pmk_len)
{
	wl_sae4way_init();
	mutex_lock(&sae4w.lock);
	s4w_reset_locked();
	if (pmk_len > sizeof(sae4w.pmk)) {
		mutex_unlock(&sae4w.lock);
		return;
	}
	sae4w.cfg = cfg;
	sae4w.ndev = ndev;
	/* make s4w_handle_m1() wait for the IEs of this association */
	wl_to_conn(cfg)->req_ie_len = 0;
	memcpy(sae4w.bssid, bssid, ETH_ALEN);
	memcpy(sae4w.pmk, pmk, pmk_len);
	sae4w.pmk_len = pmk_len;
	sae4w.active = true;
	mutex_unlock(&sae4w.lock);
	WL_INFORM_MEM(("SAE4W: armed for " MACDBG " pmk_len %d\n",
		MAC2STRDBG(bssid), pmk_len));
}

void wl_sae4way_reset(void)
{
	if (!sae4w.init)
		return;
	if (!mutex_trylock(&sae4w.lock)) {
		/* worker busy: just stop taking frames */
		sae4w.active = false;
		return;
	}
	s4w_reset_locked();
	mutex_unlock(&sae4w.lock);
	skb_queue_purge(&sae4w.q);
}

/* rx path: returns true if the EAPOL frame was consumed */
bool wl_sae4way_rx(struct net_device *ndev, const u8 *frame, u32 len)
{
	struct sk_buff *skb;
	const u8 *e;

	if (ndev->ieee80211_ptr &&
	    (ndev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP ||
	     ndev->ieee80211_ptr->iftype == NL80211_IFTYPE_P2P_GO))
		return wl_ap4way_rx(ndev, frame, len);

	if (!sae4w.init || !sae4w.active || ndev != sae4w.ndev)
		return false;
	if (len < ETH_HLEN + S4W_HDR_LEN + S4W_FIXED_LEN || len > S4W_MAX_FRAME)
		return false;
	if (memcmp(frame + ETH_ALEN, sae4w.bssid, ETH_ALEN))
		return false;
	e = frame + ETH_HLEN;
	if (e[1] != S4W_EAPOL_TYPE_KEY || e[4] != S4W_KEYDESC_RSN)
		return false;

	skb = alloc_skb(len, GFP_ATOMIC);
	if (!skb)
		return true;
	memcpy(skb_put(skb, len), frame, len);
	skb_queue_tail(&sae4w.q, skb);
	schedule_work(&sae4w.work);
	return true;
}

#else /* !WL_SAE */
#endif /* WL_SAE */
