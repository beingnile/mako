/*!
 * hash256.c - hash256 function for libsatoshi
 * Copyright (c) 2020, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/libsatoshi
 *
 * Resources:
 *   https://github.com/bitcoin/bitcoin/blob/master/src/hash.h
 */

#include <stddef.h>
#include <stdint.h>
#include <satoshi/crypto/hash.h>

/*
 * Hash256
 */

void
btc_hash256_init(btc_hash256_t *ctx) {
  btc_sha256_init(ctx);
}

void
btc_hash256_update(btc_hash256_t *ctx, const void *data, size_t len) {
  btc_sha256_update(ctx, data, len);
}

void
btc_hash256_final(btc_hash256_t *ctx, uint8_t *out) {
  btc_sha256_final(ctx, out);
  btc_sha256_init(ctx);
  btc_sha256_update(ctx, out, 32);
  btc_sha256_final(ctx, out);
}

void
btc_hash256(uint8_t *out, const void *data, size_t size) {
  btc_hash256_t ctx;
  btc_hash256_init(&ctx);
  btc_hash256_update(&ctx, data, size);
  btc_hash256_final(&ctx, out);
}