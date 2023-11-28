# Exynos FMP makefile
CONFIG_EXYNOS_FMP_FIPS_FUNC_TEST=n

CC_NO_CLANG := $(shell $(CC) -dM -E -x c /dev/null | grep -Fq "__clang__"; echo $$?)
ifeq ($(CC_NO_CLANG), 0)
ccflags-y += -DCC_USE_CLANG
CFLAGS_fmp_fips_selftest.o = -fno-merge-all-constants
endif
ifeq ($(CC_NO_CLANG), 1)
CFLAGS_fmp_fips_selftest.o = -fno-merge-constants
endif
ccflags-y += $(DISABLE_LTO_CLANG) $(DISABLE_CFI_CLANG) -fno-data-sections -fno-function-sections
ccflags-$(CONFIG_EXYNOS_FMP_FIPS_FUNC_TEST) += -DCONFIG_EXYNOS_FMP_FIPS_FUNC_TEST

obj-$(CONFIG_EXYNOS_FMP_FIPS) += first_file.o
obj-$(CONFIG_EXYNOS_FMP) += fmp.o
obj-$(CONFIG_EXYNOS_FMP_FIPS) += fmp_test.o \
				fmp_fips_main.o \
				fmp_fips_fops.o \
				fmp_fips_selftest.o \
				fmp_fips_integrity.o \
				hmac-sha256.o \
				fmp_fips_cipher.o \
				sha256.o

obj-$(CONFIG_EXYNOS_FMP_FIPS_FUNC_TEST) += fmp_fips_func_test.o
obj-$(CONFIG_EXYNOS_FMP_FIPS) += last_file.o
obj-$(CONFIG_EXYNOS_FMP_FIPS) += fips_out.o
