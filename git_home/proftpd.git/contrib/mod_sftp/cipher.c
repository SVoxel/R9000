/*
 * ProFTPD - mod_sftp ciphers
 * Copyright (c) 2008-2014 TJ Saunders
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 * As a special exemption, TJ Saunders and other respective copyright holders
 * give permission to link this program with OpenSSL, and distribute the
 * resulting executable, without including the source code for OpenSSL in the
 * source distribution.
 *
 * $Id: cipher.c,v 1.18 2014-03-02 06:07:43 castaglia Exp $
 */

#include "mod_sftp.h"

#include "packet.h"
#include "msg.h"
#include "crypto.h"
#include "cipher.h"
#include "session.h"
#include "interop.h"

struct sftp_cipher {
  const char *algo;
  const EVP_CIPHER *cipher;

  unsigned char *iv;
  uint32_t iv_len;

  unsigned char *key;
  uint32_t key_len;

  size_t discard_len;
};

/* We need to keep the old ciphers around, so that we can handle N
 * arbitrary packets to/from the client using the old keys, as during rekeying.
 * Thus we have two read cipher contexts, two write cipher contexts.
 * The cipher idx variable indicates which of the ciphers is currently in use.
 */

static struct sftp_cipher read_ciphers[2] = {
  { NULL, NULL, NULL, 0, NULL, 0, 0 },
  { NULL, NULL, NULL, 0, NULL, 0, 0 }
};
static EVP_CIPHER_CTX read_ctxs[2]; 

static struct sftp_cipher write_ciphers[2] = {
  { NULL, NULL, NULL, 0, NULL, 0, 0 },
  { NULL, NULL, NULL, 0, NULL, 0, 0 }
};
static EVP_CIPHER_CTX write_ctxs[2];

#define SFTP_CIPHER_DEFAULT_BLOCK_SZ		8
static size_t cipher_blockszs[2] = {
  SFTP_CIPHER_DEFAULT_BLOCK_SZ,
  SFTP_CIPHER_DEFAULT_BLOCK_SZ,
};

/* Buffer size for reading/writing keys */
#define SFTP_CIPHER_BUFSZ			4096

static unsigned int read_cipher_idx = 0;
static unsigned int write_cipher_idx = 0;

static void clear_cipher(struct sftp_cipher *);

static unsigned int get_next_read_index(void) {
  if (read_cipher_idx == 1)
    return 0;

  return 1;
}

static unsigned int get_next_write_index(void) {
  if (write_cipher_idx == 1)
    return 0;

  return 1;
}

static void switch_read_cipher(void) {
  /* First, clear the context of the existing read cipher, if any. */
  if (read_ciphers[read_cipher_idx].key) {
    clear_cipher(&(read_ciphers[read_cipher_idx]));
    if (EVP_CIPHER_CTX_cleanup(&(read_ctxs[read_cipher_idx])) != 1) {
      (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
        "error clearing cipher context: %s", sftp_crypto_get_errors());
    }
 
    cipher_blockszs[read_cipher_idx] = SFTP_CIPHER_DEFAULT_BLOCK_SZ; 

    /* Now we can switch the index. */
    if (read_cipher_idx == 1) {
      read_cipher_idx = 0;
      return;
    }

    read_cipher_idx = 1;
  }
}

static void switch_write_cipher(void) {
  /* First, clear the context of the existing read cipher, if any. */
  if (write_ciphers[write_cipher_idx].key) {
    clear_cipher(&(write_ciphers[write_cipher_idx]));
    if (EVP_CIPHER_CTX_cleanup(&(write_ctxs[write_cipher_idx])) != 1) {
      (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
        "error clearing cipher context: %s", sftp_crypto_get_errors());
    }

    cipher_blockszs[write_cipher_idx] = SFTP_CIPHER_DEFAULT_BLOCK_SZ;

    /* Now we can switch the index. */
    if (write_cipher_idx == 1) {
      write_cipher_idx = 0;
      return;
    }

    write_cipher_idx = 1;
  }
}

static void clear_cipher(struct sftp_cipher *cipher) {
  if (cipher->iv) {
    pr_memscrub(cipher->iv, cipher->iv_len);
    free(cipher->iv);
    cipher->iv = NULL;
    cipher->iv_len = 0;
  }

  if (cipher->key) {
    pr_memscrub(cipher->key, cipher->key_len);
    free(cipher->key);
    cipher->key = NULL;
    cipher->key_len = 0;
  }

  cipher->cipher = NULL;
  cipher->algo = NULL;
}

static int set_cipher_iv(struct sftp_cipher *cipher, const EVP_MD *hash,
    const unsigned char *k, uint32_t klen, const char *h, uint32_t hlen,
    char *letter, const unsigned char *id, uint32_t id_len) {

  EVP_MD_CTX ctx;
  unsigned char *iv = NULL;
  size_t cipher_iv_len = 0, iv_sz = 0;
  uint32_t iv_len = 0;

  if (strncmp(cipher->algo, "none", 5) == 0) {
    cipher->iv = iv;
    cipher->iv_len = iv_len;

    return 0;
  }

   /* Some ciphers do not use IVs; handle this case. */
  cipher_iv_len = EVP_CIPHER_iv_length(cipher->cipher);
  if (cipher_iv_len != 0) {
    iv_sz = sftp_crypto_get_size(cipher_iv_len, EVP_MD_size(hash));

  } else {
    iv_sz = EVP_MD_size(hash);
  }

  if (iv_sz == 0) {
    (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
      "unable to determine IV length for cipher '%s'", cipher->algo);
     errno = EINVAL;
    return -1;
  }

  iv = malloc(iv_sz);
  if (iv == NULL) {
    pr_log_pri(PR_LOG_ALERT, MOD_SFTP_VERSION ": Out of memory!");
    _exit(1);
  }

  EVP_DigestInit(&ctx, hash);
  if (sftp_interop_supports_feature(SFTP_SSH2_FEAT_CIPHER_USE_K)) {
    EVP_DigestUpdate(&ctx, k, klen);
  }
  EVP_DigestUpdate(&ctx, h, hlen);
  EVP_DigestUpdate(&ctx, letter, sizeof(char));
  EVP_DigestUpdate(&ctx, (char *) id, id_len);
  EVP_DigestFinal(&ctx, iv, &iv_len);

  /* If we need more, keep hashing, as per RFC, until we have enough
   * material.
   */
  while (iv_sz > iv_len) {
    uint32_t len = iv_len;

    pr_signals_handle();

    EVP_DigestInit(&ctx, hash);
    if (sftp_interop_supports_feature(SFTP_SSH2_FEAT_CIPHER_USE_K)) {
      EVP_DigestUpdate(&ctx, k, klen);
    }
    EVP_DigestUpdate(&ctx, h, hlen);
    EVP_DigestUpdate(&ctx, iv, len);
    EVP_DigestFinal(&ctx, iv + len, &len);

    iv_len += len;
  }

  cipher->iv = iv;
  cipher->iv_len = iv_len;

  return 0;
}

static int set_cipher_key(struct sftp_cipher *cipher, const EVP_MD *hash,
    const unsigned char *k, uint32_t klen, const char *h, uint32_t hlen,
    char *letter, const unsigned char *id, uint32_t id_len) {

  EVP_MD_CTX ctx;
  unsigned char *key = NULL;
  size_t key_sz = 0;
  uint32_t key_len = 0;

  if (strncmp(cipher->algo, "none", 5) == 0) {
    cipher->key = key;
    cipher->key_len = key_len;

    return 0;
  }

  key_sz = sftp_crypto_get_size(cipher->key_len > 0 ?
      cipher->key_len : EVP_CIPHER_key_length(cipher->cipher),
    EVP_MD_size(hash));

  if (key_sz == 0) {
    (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
      "unable to determine key length for cipher '%s'", cipher->algo);
    errno = EINVAL;
    return -1;
  }

  key = malloc(key_sz);
  if (key == NULL) {
    pr_log_pri(PR_LOG_ALERT, MOD_SFTP_VERSION ": Out of memory!");
    _exit(1);
  }

  EVP_DigestInit(&ctx, hash);
  EVP_DigestUpdate(&ctx, k, klen);
  EVP_DigestUpdate(&ctx, h, hlen);
  EVP_DigestUpdate(&ctx, letter, sizeof(char));
  EVP_DigestUpdate(&ctx, (char *) id, id_len);
  EVP_DigestFinal(&ctx, key, &key_len);

  /* If we need more, keep hashing, as per RFC, until we have enough
   * material.
   */
  while (key_sz > key_len) {
    uint32_t len = key_len;

    pr_signals_handle();

    EVP_DigestInit(&ctx, hash);
    EVP_DigestUpdate(&ctx, k, klen);
    EVP_DigestUpdate(&ctx, h, hlen);
    EVP_DigestUpdate(&ctx, key, len);
    EVP_DigestFinal(&ctx, key + len, &len);

    key_len += len;
  }

  cipher->key = key;
  cipher->key_len = key_len;

  return 0;
}

/* If the chosen cipher requires that we discard some of the initial bytes of
 * the cipher stream, then do so.  (This is mostly for any RC4 ciphers.)
 */
static int set_cipher_discarded(struct sftp_cipher *cipher,
    EVP_CIPHER_CTX *cipher_ctx) {
  unsigned char *garbage_in, *garbage_out;

  if (cipher->discard_len == 0) {
    return 0;
  }

  garbage_in = malloc(cipher->discard_len);
  if (garbage_in == NULL) {
    pr_log_pri(PR_LOG_ALERT, MOD_SFTP_VERSION ": Out of memory!");
    _exit(1);
  }

  garbage_out = malloc(cipher->discard_len);
  if (garbage_out == NULL) {
    pr_log_pri(PR_LOG_ALERT, MOD_SFTP_VERSION ": Out of memory!");
    free(garbage_in);
    _exit(1);
  }

  if (EVP_Cipher(cipher_ctx, garbage_out, garbage_in,
      cipher->discard_len) != 1) {
    (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
      "error ciphering discard data: %s", sftp_crypto_get_errors());
    free(garbage_in);
    pr_memscrub(garbage_out, cipher->discard_len);
    free(garbage_out);

    return -1;
  }

  free(garbage_in);
  pr_memscrub(garbage_out, cipher->discard_len);
  free(garbage_out);

  return 0;
}

size_t sftp_cipher_get_block_size(void) {
  return cipher_blockszs[read_cipher_idx];
}

void sftp_cipher_set_block_size(size_t blocksz) {
  if (blocksz > cipher_blockszs[read_cipher_idx]) {
    cipher_blockszs[read_cipher_idx] = blocksz;
  }
}

const char *sftp_cipher_get_read_algo(void) {
  if (read_ciphers[read_cipher_idx].key != NULL ||
      strncmp(read_ciphers[read_cipher_idx].algo, "none", 5) == 0) {
    return read_ciphers[read_cipher_idx].algo;
  }

  return NULL;
}

int sftp_cipher_set_read_algo(const char *algo) {
  unsigned int idx = read_cipher_idx;

  if (read_ciphers[idx].key) {
    /* If we have an existing key, it means that we are currently rekeying. */
    idx = get_next_read_index();
  }

  read_ciphers[idx].cipher = sftp_crypto_get_cipher(algo,
    (size_t *) &(read_ciphers[idx].key_len),
    &(read_ciphers[idx].discard_len));

  if (read_ciphers[idx].cipher == NULL)
    return -1;

  read_ciphers[idx].algo = algo;
  return 0;
}

int sftp_cipher_set_read_key(pool *p, const EVP_MD *hash, const BIGNUM *k,
    const char *h, uint32_t hlen) {
  const unsigned char *id = NULL;
  unsigned char *buf, *ptr;
  char letter;
  uint32_t buflen, bufsz, id_len;
  int key_len;
  struct sftp_cipher *cipher;
  EVP_CIPHER_CTX *cipher_ctx;

  switch_read_cipher();

  cipher = &(read_ciphers[read_cipher_idx]);
  cipher_ctx = &(read_ctxs[read_cipher_idx]);

  /* XXX EVP_CIPHER_CTX_init() first appeared in OpenSSL 0.9.7.  What to do
   * for older OpenSSL installations?
   */
  EVP_CIPHER_CTX_init(cipher_ctx);

  bufsz = buflen = SFTP_CIPHER_BUFSZ;
  ptr = buf = sftp_msg_getbuf(p, bufsz);

  /* Need to use SSH2-style format of K for the IV and key. */
  sftp_msg_write_mpint(&buf, &buflen, k);

  id_len = sftp_session_get_id(&id);

  /* First, initialize the cipher, but don't provide the key or IV yet. */
  if (EVP_CipherInit(cipher_ctx, cipher->cipher, NULL, NULL, 0) != 1) {
    (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
      "error initializing %s cipher for decryption: %s", cipher->algo,
      sftp_crypto_get_errors());
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  /* IV: HASH(K || H || "A" || session_id) */
  letter = 'A';
  if (set_cipher_iv(cipher, hash, ptr, (bufsz - buflen), h, hlen, &letter, id,
      id_len) < 0) {
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  key_len = (int) cipher->key_len;

  /* Key: HASH(K || H || "C" || session_id) */
  letter = 'C';
  if (set_cipher_key(cipher, hash, ptr, (bufsz - buflen), h, hlen, &letter,
      id, id_len) < 0) {
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  if (key_len > 0) {
    /* Next, set the key length. */
    if (EVP_CIPHER_CTX_set_key_length(cipher_ctx, key_len) != 1) {
      (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
        "error setting key length (%d bytes) for %s cipher for decryption: %s",
        key_len, cipher->algo, sftp_crypto_get_errors());
      pr_memscrub(ptr, bufsz);
      return -1;
    }
  }

  /* Now provide the key and IV. */
  if (EVP_CipherInit(cipher_ctx, NULL, cipher->key, cipher->iv, -1) != 1) {
    (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
      "error setting key/IV for %s cipher for decryption: %s", cipher->algo,
      sftp_crypto_get_errors());
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  if (set_cipher_discarded(cipher, cipher_ctx) < 0) {
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  pr_memscrub(ptr, bufsz);
  sftp_cipher_set_block_size(EVP_CIPHER_block_size(cipher->cipher));
  return 0;
}

int sftp_cipher_read_data(pool *p, unsigned char *data, uint32_t data_len,
    unsigned char **buf, uint32_t *buflen) {
  struct sftp_cipher *cipher;
  EVP_CIPHER_CTX *cipher_ctx;
  size_t cipher_blocksz;

  cipher = &(read_ciphers[read_cipher_idx]);
  cipher_ctx = &(read_ctxs[read_cipher_idx]);
  cipher_blocksz = cipher_blockszs[read_cipher_idx];

  if (cipher->key) {
    int res;
    unsigned char *ptr = NULL;
    size_t bufsz;

    if (*buflen % cipher_blocksz != 0) {
      (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
        "bad input length for decryption (%u bytes, %u block size)", *buflen,
        (unsigned int) cipher_blocksz);
      return -1;
    }

    if (*buf == NULL) {
      /* Allocate a buffer that's large enough. */
      bufsz = (data_len + cipher_blocksz - 1);
      ptr = palloc(p, bufsz);

    } else {
      ptr = *buf;
    }

    res = EVP_Cipher(cipher_ctx, ptr, data, data_len);
    if (res != 1) {
      (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
        "error decrypting %s data from client: %s", cipher->algo,
        sftp_crypto_get_errors());
      return -1;
    }

    *buflen = data_len;
    *buf = ptr;

    return 0;
  }

  *buf = data;
  *buflen = data_len;
  return 0;
}

const char *sftp_cipher_get_write_algo(void) {
  if (write_ciphers[write_cipher_idx].key != NULL ||
      strncmp(write_ciphers[write_cipher_idx].algo, "none", 5) == 0) {
    return write_ciphers[write_cipher_idx].algo;
  }

  return NULL;
}

int sftp_cipher_set_write_algo(const char *algo) {
  unsigned int idx = write_cipher_idx;

  if (write_ciphers[idx].key) {
    /* If we have an existing key, it means that we are currently rekeying. */
    idx = get_next_write_index();
  }

  write_ciphers[idx].cipher = sftp_crypto_get_cipher(algo,
    (size_t *) &(write_ciphers[idx].key_len),
    &(write_ciphers[idx].discard_len));

  if (write_ciphers[idx].cipher == NULL)
    return -1;

  write_ciphers[idx].algo = algo;
  return 0;
}

int sftp_cipher_set_write_key(pool *p, const EVP_MD *hash, const BIGNUM *k,
    const char *h, uint32_t hlen) {
  const unsigned char *id = NULL;
  unsigned char *buf, *ptr;
  char letter;
  uint32_t buflen, bufsz, id_len;
  int key_len;
  struct sftp_cipher *cipher;
  EVP_CIPHER_CTX *cipher_ctx;

  switch_write_cipher();

  cipher = &(write_ciphers[write_cipher_idx]);
  cipher_ctx = &(write_ctxs[write_cipher_idx]);

  /* XXX EVP_CIPHER_CTX_init() first appeared in OpenSSL 0.9.7.  What to do
   * for older OpenSSL installations?
   */
  EVP_CIPHER_CTX_init(cipher_ctx);

  bufsz = buflen = SFTP_CIPHER_BUFSZ;
  ptr = buf = sftp_msg_getbuf(p, bufsz);

  /* Need to use SSH2-style format of K for the IV and key. */
  sftp_msg_write_mpint(&buf, &buflen, k);

  id_len = sftp_session_get_id(&id);

  /* First, initialize the cipher, but don't provide the key or IV yet. */
  if (EVP_CipherInit(cipher_ctx, cipher->cipher, NULL, NULL, 1) != 1) {
    (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
      "error initializing %s cipher for encryption: %s", cipher->algo,
      sftp_crypto_get_errors());
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  /* IV: HASH(K || H || "B" || session_id) */
  letter = 'B';
  if (set_cipher_iv(cipher, hash, ptr, (bufsz - buflen), h, hlen, &letter, id,
      id_len) < 0) {
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  key_len = (int) cipher->key_len;

  /* Key: HASH(K || H || "D" || session_id) */
  letter = 'D';
  if (set_cipher_key(cipher, hash, ptr, (bufsz - buflen), h, hlen, &letter,
      id, id_len) < 0) {
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  if (key_len > 0) {
    /* Next, set the key length. */
    if (EVP_CIPHER_CTX_set_key_length(cipher_ctx, key_len) != 1) {
      (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
        "error setting key length (%d bytes) for %s cipher for decryption: %s",
        key_len, cipher->algo, sftp_crypto_get_errors());
      pr_memscrub(ptr, bufsz);
      return -1;
    }
  }

  /* Now provide the key and IV. */
  if (EVP_CipherInit(cipher_ctx, NULL, cipher->key, cipher->iv, -1) != 1) {
    (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
      "error setting key/IV for %s cipher for encryption: %s", cipher->algo,
      sftp_crypto_get_errors());
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  if (set_cipher_discarded(cipher, cipher_ctx) < 0) {
    pr_memscrub(ptr, bufsz);
    return -1;
  }

  pr_memscrub(ptr, bufsz);
  return 0;
}

int sftp_cipher_write_data(struct ssh2_packet *pkt, unsigned char *buf,
    size_t *buflen) {
  struct sftp_cipher *cipher;
  EVP_CIPHER_CTX *cipher_ctx;

  cipher = &(write_ciphers[write_cipher_idx]);
  cipher_ctx = &(write_ctxs[write_cipher_idx]);

  if (cipher->key) {
    int res;
    unsigned char *data, *ptr;
    uint32_t datalen, datasz = sizeof(uint32_t) + pkt->packet_len;

    datalen = datasz;
    ptr = data = palloc(pkt->pool, datasz);

    sftp_msg_write_int(&data, &datalen, pkt->packet_len);
    sftp_msg_write_byte(&data, &datalen, pkt->padding_len);
    sftp_msg_write_data(&data, &datalen, pkt->payload, pkt->payload_len, FALSE);
    sftp_msg_write_data(&data, &datalen, pkt->padding, pkt->padding_len, FALSE);

    res = EVP_Cipher(cipher_ctx, buf, ptr, (datasz - datalen));
    if (res != 1) {
      (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
        "error encrypting %s data for client: %s", cipher->algo,
        sftp_crypto_get_errors());
      errno = EIO;
      return -1;
    }

    *buflen = (datasz - datalen);

#ifdef SFTP_DEBUG_PACKET
{
  unsigned int i;

  (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
    "encrypted packet data (len %lu):", (unsigned long) *buflen);
  for (i = 0; i < *buflen;) {
    (void) pr_log_writefile(sftp_logfd, MOD_SFTP_VERSION,
      "  %02x%02x %02x%02x %02x%02x %02x%02x",
      ((unsigned char *) buf)[i], ((unsigned char *) buf)[i+1],
      ((unsigned char *) buf)[i+2], ((unsigned char *) buf)[i+3],
      ((unsigned char *) buf)[i+4], ((unsigned char *) buf)[i+5],
      ((unsigned char *) buf)[i+6], ((unsigned char *) buf)[i+7]);
    i += 8;
  }
}
#endif

    return 0;
  }

  *buflen = 0;
  return 0;
}
