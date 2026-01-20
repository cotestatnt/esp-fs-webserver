#pragma once

#if defined(ESP8266)
#include <bearssl/bearssl.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MBEDTLS_AES_ENCRYPT 1
#define MBEDTLS_AES_DECRYPT 0

typedef struct {
    br_aes_ct_cbcenc_keys enc;
    br_aes_ct_cbcdec_keys dec;
    uint8_t has_enc;
    uint8_t has_dec;
} mbedtls_aes_context;

static inline void mbedtls_aes_init(mbedtls_aes_context *ctx) {
    if (ctx) {
        memset(ctx, 0, sizeof(*ctx));
    }
}

static inline void mbedtls_aes_free(mbedtls_aes_context *ctx) {
    (void)ctx;
}

static inline int mbedtls_aes_setkey_enc(mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits) {
    if (!ctx || !key || (keybits % 8) != 0) {
        return -1;
    }
    br_aes_ct_cbcenc_init(&ctx->enc, key, keybits / 8);
    ctx->has_enc = 1;
    return 0;
}

static inline int mbedtls_aes_setkey_dec(mbedtls_aes_context *ctx, const unsigned char *key, unsigned int keybits) {
    if (!ctx || !key || (keybits % 8) != 0) {
        return -1;
    }
    br_aes_ct_cbcdec_init(&ctx->dec, key, keybits / 8);
    ctx->has_dec = 1;
    return 0;
}

static inline int mbedtls_aes_crypt_cbc(mbedtls_aes_context *ctx, int mode, size_t length,
                                       unsigned char iv[16], const unsigned char *input, unsigned char *output) {
    if (!ctx || !iv || !input || !output) {
        return -1;
    }
    if (length % 16 != 0) {
        return -1;
    }

    if (input != output) {
        memcpy(output, input, length);
    }

    if (mode == MBEDTLS_AES_ENCRYPT) {
        if (!ctx->has_enc) {
            return -1;
        }
        br_aes_ct_cbcenc_run(&ctx->enc, iv, output, length);
        return 0;
    }

    if (mode == MBEDTLS_AES_DECRYPT) {
        if (!ctx->has_dec) {
            return -1;
        }
        br_aes_ct_cbcdec_run(&ctx->dec, iv, output, length);
        return 0;
    }

    return -1;
}

#ifdef __cplusplus
}
#endif

#else
#include_next <mbedtls/aes.h>
#endif
