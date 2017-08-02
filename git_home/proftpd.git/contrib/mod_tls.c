/*
 * mod_tls - An RFC2228 SSL/TLS module for ProFTPD
 *
 * Copyright (c) 2000-2002 Peter 'Luna' Runestig <peter@runestig.com>
 * Copyright (c) 2002-2016 TJ Saunders <tj@castaglia.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modifi-
 * cation, are permitted provided that the following conditions are met:
 *
 *    o Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *    o Redistributions in binary form must reproduce the above copyright no-
 *      tice, this list of conditions and the following disclaimer in the do-
 *      cumentation and/or other materials provided with the distribution.
 *
 *    o The names of the contributors may not be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LI-
 * ABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 * TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEV-
 * ER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABI-
 * LITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  --- DO NOT DELETE BELOW THIS LINE ----
 *  $Libraries: -lssl -lcrypto$
 */

#include "conf.h"
#include "privs.h"
#include "mod_tls.h"

#ifdef PR_USE_CTRLS
# include "mod_ctrls.h"
#endif

/* Note that the openssl/ssl.h header is already included in mod_tls.h, so
 * we don't need to include it here.
*/

#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#if OPENSSL_VERSION_NUMBER > 0x000907000L
# include <openssl/engine.h>
# include <openssl/ocsp.h>
#endif
#ifdef PR_USE_OPENSSL_ECC
# include <openssl/ec.h>
# include <openssl/ecdh.h>
#endif /* PR_USE_OPENSSL_ECC */


#ifdef HAVE_MLOCK
# include <sys/mman.h>
#endif

#define MOD_TLS_VERSION		"mod_tls/2.6"

/* Make sure the version of proftpd is as necessary. */
#if PROFTPD_VERSION_NUMBER < 0x0001030504 
# error "ProFTPD 1.3.5rc4 or later required"
#endif

extern session_t session;
extern xaset_t *server_list;

/* DH parameters.  These are generated using:
 *
 *  # openssl dhparam -2|-5 512|768|1024|1536|2048 -C
 *
 * These should be regenerated periodically by the mod_tls maintainer.
 * Last updated on 2013-01-13.
 */

/*
-----BEGIN DH PARAMETERS-----
MEYCQQC6VcB8+WFeFz/HQnk/vXreozxdppNBY4gN8rdzitjTDppPBswzU4ZL/hBS
e1Crmu4uTtpPNxVVA+g3FyB/GfUTAgEF
-----END DH PARAMETERS-----
*/

static unsigned char dh512_p[] = {
  0xBA,0x55,0xC0,0x7C,0xF9,0x61,0x5E,0x17,0x3F,0xC7,0x42,0x79,
  0x3F,0xBD,0x7A,0xDE,0xA3,0x3C,0x5D,0xA6,0x93,0x41,0x63,0x88,
  0x0D,0xF2,0xB7,0x73,0x8A,0xD8,0xD3,0x0E,0x9A,0x4F,0x06,0xCC,
  0x33,0x53,0x86,0x4B,0xFE,0x10,0x52,0x7B,0x50,0xAB,0x9A,0xEE,
  0x2E,0x4E,0xDA,0x4F,0x37,0x15,0x55,0x03,0xE8,0x37,0x17,0x20,
  0x7F,0x19,0xF5,0x13,
};

static unsigned char dh512_g[] = {
  0x05,
};

static DH *get_dh512(void) {
  DH *dh;

  dh = DH_new();
  if (dh == NULL)
    return NULL;

  dh->p = BN_bin2bn(dh512_p, sizeof(dh512_p), NULL);
  dh->g = BN_bin2bn(dh512_g, sizeof(dh512_g), NULL);

  if (dh->p == NULL ||
      dh->g == NULL) {
    DH_free(dh);
    return NULL;
  }

  return dh;
}

/*
-----BEGIN DH PARAMETERS-----
MGYCYQDhEinscQZuuGMmmzoYLuNHbA8rZmDilZjB8RasGJ1Mo8knn9tFLtRGHVxw
IxUaxkQgeqAy2FjQFz2Z9hU3a1dCgb3VEgRPDXqMSW8t4UsVrHN/AMqszID/PZb+
VTp+9osCAQI=
-----END DH PARAMETERS-----
*/

static unsigned char dh768_p[] = {
  0xE1,0x12,0x29,0xEC,0x71,0x06,0x6E,0xB8,0x63,0x26,0x9B,0x3A,
  0x18,0x2E,0xE3,0x47,0x6C,0x0F,0x2B,0x66,0x60,0xE2,0x95,0x98,
  0xC1,0xF1,0x16,0xAC,0x18,0x9D,0x4C,0xA3,0xC9,0x27,0x9F,0xDB,
  0x45,0x2E,0xD4,0x46,0x1D,0x5C,0x70,0x23,0x15,0x1A,0xC6,0x44,
  0x20,0x7A,0xA0,0x32,0xD8,0x58,0xD0,0x17,0x3D,0x99,0xF6,0x15,
  0x37,0x6B,0x57,0x42,0x81,0xBD,0xD5,0x12,0x04,0x4F,0x0D,0x7A,
  0x8C,0x49,0x6F,0x2D,0xE1,0x4B,0x15,0xAC,0x73,0x7F,0x00,0xCA,
  0xAC,0xCC,0x80,0xFF,0x3D,0x96,0xFE,0x55,0x3A,0x7E,0xF6,0x8B,
};

static unsigned char dh768_g[] = {
  0x02,
};

static DH *get_dh768(void) {
  DH *dh;

  dh = DH_new();
  if (dh == NULL)
    return NULL;

  dh->p = BN_bin2bn(dh768_p, sizeof(dh768_p), NULL);
  dh->g = BN_bin2bn(dh768_g, sizeof(dh768_g), NULL);

  if (dh->p == NULL ||
      dh->g == NULL) {
    DH_free(dh);
    return NULL;
  }

  return dh;
}

/*
-----BEGIN DH PARAMETERS-----
MIGHAoGBAPLQY9o7o6wDAUOW4OYq+Cp9j0cnN2qgkdCDQTMHHdqotbF7TiYv3iuP
a6Qu1WwNyVoa9k7vyzfW7ZxvLLChUBvTFZ1gBAKlVpvhq/XmWHLwXY4Q/auCrHjR
j6VNC3D6sD68viO0Cqf8yWjBGwdiXkW6tMNuz1iPS4/KZD09QJrDAgEC
-----END DH PARAMETERS-----
*/

static unsigned char dh1024_p[] = {
  0xF2,0xD0,0x63,0xDA,0x3B,0xA3,0xAC,0x03,0x01,0x43,0x96,0xE0,
  0xE6,0x2A,0xF8,0x2A,0x7D,0x8F,0x47,0x27,0x37,0x6A,0xA0,0x91,
  0xD0,0x83,0x41,0x33,0x07,0x1D,0xDA,0xA8,0xB5,0xB1,0x7B,0x4E,
  0x26,0x2F,0xDE,0x2B,0x8F,0x6B,0xA4,0x2E,0xD5,0x6C,0x0D,0xC9,
  0x5A,0x1A,0xF6,0x4E,0xEF,0xCB,0x37,0xD6,0xED,0x9C,0x6F,0x2C,
  0xB0,0xA1,0x50,0x1B,0xD3,0x15,0x9D,0x60,0x04,0x02,0xA5,0x56,
  0x9B,0xE1,0xAB,0xF5,0xE6,0x58,0x72,0xF0,0x5D,0x8E,0x10,0xFD,
  0xAB,0x82,0xAC,0x78,0xD1,0x8F,0xA5,0x4D,0x0B,0x70,0xFA,0xB0,
  0x3E,0xBC,0xBE,0x23,0xB4,0x0A,0xA7,0xFC,0xC9,0x68,0xC1,0x1B,
  0x07,0x62,0x5E,0x45,0xBA,0xB4,0xC3,0x6E,0xCF,0x58,0x8F,0x4B,
  0x8F,0xCA,0x64,0x3D,0x3D,0x40,0x9A,0xC3,
};

static unsigned char dh1024_g[] = {
  0x02,
};

static DH *get_dh1024(void) {
  DH *dh;

  dh = DH_new();
  if (dh == NULL)
    return NULL;

  dh->p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), NULL);
  dh->g = BN_bin2bn(dh1024_g, sizeof(dh1024_g), NULL);

  if (dh->p == NULL ||
      dh->g == NULL) {
    DH_free(dh);
    return NULL;
  }

  return(dh);
}

/*
-----BEGIN DH PARAMETERS-----
MIHHAoHBAJSrJ66X9HpCRRUSuODRIhkQCFiJbNthRZclqtVRqJxIU6rpIkRD1L/A
lPlja3mSNiVwn37oOHP5lujp7e6mLkCj+1QSuStZgDg4MMFZK0vEyneCVZa/4HzN
tCZ8mdTcBU/HVOhkkFckJBzX6LVTd1DDrNOvdu5j+zKbMlgzmz+/YRHlPlEVrzSc
Jp3CM6YLxHbTa0NXIjlzHWjQEtMw6UDsrDJLDnU8dMbMSQorTTpBY/O/eeXeA05x
5cXAtpNOMwIBBQ==
-----END DH PARAMETERS-----
*/

static unsigned char dh1536_p[] = {
  0x94,0xAB,0x27,0xAE,0x97,0xF4,0x7A,0x42,0x45,0x15,0x12,0xB8,
  0xE0,0xD1,0x22,0x19,0x10,0x08,0x58,0x89,0x6C,0xDB,0x61,0x45,
  0x97,0x25,0xAA,0xD5,0x51,0xA8,0x9C,0x48,0x53,0xAA,0xE9,0x22,
  0x44,0x43,0xD4,0xBF,0xC0,0x94,0xF9,0x63,0x6B,0x79,0x92,0x36,
  0x25,0x70,0x9F,0x7E,0xE8,0x38,0x73,0xF9,0x96,0xE8,0xE9,0xED,
  0xEE,0xA6,0x2E,0x40,0xA3,0xFB,0x54,0x12,0xB9,0x2B,0x59,0x80,
  0x38,0x38,0x30,0xC1,0x59,0x2B,0x4B,0xC4,0xCA,0x77,0x82,0x55,
  0x96,0xBF,0xE0,0x7C,0xCD,0xB4,0x26,0x7C,0x99,0xD4,0xDC,0x05,
  0x4F,0xC7,0x54,0xE8,0x64,0x90,0x57,0x24,0x24,0x1C,0xD7,0xE8,
  0xB5,0x53,0x77,0x50,0xC3,0xAC,0xD3,0xAF,0x76,0xEE,0x63,0xFB,
  0x32,0x9B,0x32,0x58,0x33,0x9B,0x3F,0xBF,0x61,0x11,0xE5,0x3E,
  0x51,0x15,0xAF,0x34,0x9C,0x26,0x9D,0xC2,0x33,0xA6,0x0B,0xC4,
  0x76,0xD3,0x6B,0x43,0x57,0x22,0x39,0x73,0x1D,0x68,0xD0,0x12,
  0xD3,0x30,0xE9,0x40,0xEC,0xAC,0x32,0x4B,0x0E,0x75,0x3C,0x74,
  0xC6,0xCC,0x49,0x0A,0x2B,0x4D,0x3A,0x41,0x63,0xF3,0xBF,0x79,
  0xE5,0xDE,0x03,0x4E,0x71,0xE5,0xC5,0xC0,0xB6,0x93,0x4E,0x33,
};

static unsigned char dh1536_g[] = {
  0x05,
};

static DH *get_dh1536(void) {
  DH *dh;

  dh = DH_new();
  if (dh == NULL)
    return NULL;

  dh->p = BN_bin2bn(dh1536_p, sizeof(dh1536_p), NULL);
  dh->g = BN_bin2bn(dh1536_g, sizeof(dh1536_g), NULL);

  if (dh->p == NULL ||
      dh->g == NULL) {
    DH_free(dh);
    return NULL;
  }

  return dh;
}

/*
-----BEGIN DH PARAMETERS-----
MIIBCAKCAQEA8uoKASu5Z9sdFVdEvpQOhZvbpHT7a+ZEKrUu+FRnA9vzK3uGn6gk
GwrLE/wcWcxcLO56mAY91kiordKHZYTW8KYq6416bA3JrOtBwiZveSAXG6pa+SSk
g3Dn6iK2rMado8s2y1MTUYQDQ8LsqnYOrHv551fP0kMq7v9bV0rr90bF54P54RFd
VKMx82r76n5gEtsVNsVKbTabob3wZVjdCCIlSVpumGYWJXbu2zFPF03taSP8zDGr
Z9jCVY+cEoU4z1hrWgHTtovfhoW8i1ULNrGfOOcdMzGhK/VtuIU6RMKqfC6OhmZL
Md/fa3Ip5joGRWGnl2oEQEK29ARJxG7UAwIBBQ==
-----END DH PARAMETERS-----
*/

static unsigned char dh2048_p[] = {
  0xF2,0xEA,0x0A,0x01,0x2B,0xB9,0x67,0xDB,0x1D,0x15,0x57,0x44,
  0xBE,0x94,0x0E,0x85,0x9B,0xDB,0xA4,0x74,0xFB,0x6B,0xE6,0x44,
  0x2A,0xB5,0x2E,0xF8,0x54,0x67,0x03,0xDB,0xF3,0x2B,0x7B,0x86,
  0x9F,0xA8,0x24,0x1B,0x0A,0xCB,0x13,0xFC,0x1C,0x59,0xCC,0x5C,
  0x2C,0xEE,0x7A,0x98,0x06,0x3D,0xD6,0x48,0xA8,0xAD,0xD2,0x87,
  0x65,0x84,0xD6,0xF0,0xA6,0x2A,0xEB,0x8D,0x7A,0x6C,0x0D,0xC9,
  0xAC,0xEB,0x41,0xC2,0x26,0x6F,0x79,0x20,0x17,0x1B,0xAA,0x5A,
  0xF9,0x24,0xA4,0x83,0x70,0xE7,0xEA,0x22,0xB6,0xAC,0xC6,0x9D,
  0xA3,0xCB,0x36,0xCB,0x53,0x13,0x51,0x84,0x03,0x43,0xC2,0xEC,
  0xAA,0x76,0x0E,0xAC,0x7B,0xF9,0xE7,0x57,0xCF,0xD2,0x43,0x2A,
  0xEE,0xFF,0x5B,0x57,0x4A,0xEB,0xF7,0x46,0xC5,0xE7,0x83,0xF9,
  0xE1,0x11,0x5D,0x54,0xA3,0x31,0xF3,0x6A,0xFB,0xEA,0x7E,0x60,
  0x12,0xDB,0x15,0x36,0xC5,0x4A,0x6D,0x36,0x9B,0xA1,0xBD,0xF0,
  0x65,0x58,0xDD,0x08,0x22,0x25,0x49,0x5A,0x6E,0x98,0x66,0x16,
  0x25,0x76,0xEE,0xDB,0x31,0x4F,0x17,0x4D,0xED,0x69,0x23,0xFC,
  0xCC,0x31,0xAB,0x67,0xD8,0xC2,0x55,0x8F,0x9C,0x12,0x85,0x38,
  0xCF,0x58,0x6B,0x5A,0x01,0xD3,0xB6,0x8B,0xDF,0x86,0x85,0xBC,
  0x8B,0x55,0x0B,0x36,0xB1,0x9F,0x38,0xE7,0x1D,0x33,0x31,0xA1,
  0x2B,0xF5,0x6D,0xB8,0x85,0x3A,0x44,0xC2,0xAA,0x7C,0x2E,0x8E,
  0x86,0x66,0x4B,0x31,0xDF,0xDF,0x6B,0x72,0x29,0xE6,0x3A,0x06,
  0x45,0x61,0xA7,0x97,0x6A,0x04,0x40,0x42,0xB6,0xF4,0x04,0x49,
  0xC4,0x6E,0xD4,0x03,
};

static unsigned char dh2048_g[] = {
  0x05,
};

static DH *get_dh2048(void) {
  DH *dh;

  dh = DH_new();
  if (dh == NULL)
    return NULL;

  dh->p = BN_bin2bn(dh2048_p, sizeof(dh2048_p), NULL);
  dh->g = BN_bin2bn(dh2048_g, sizeof(dh2048_g), NULL);

  if (dh->p == NULL ||
      dh->g == NULL) {
    DH_free(dh);
    return NULL;
  }

  return dh;
}

/* ASN1_BIT_STRING_cmp was renamed in 0.9.5 */
#if OPENSSL_VERSION_NUMBER < 0x00905100L
# define M_ASN1_BIT_STRING_cmp ASN1_BIT_STRING_cmp
#endif

/* From src/dirtree.c */
extern int ServerUseReverseDNS;

module tls_module;

typedef struct tls_pkey_obj {
  struct tls_pkey_obj *next;

  size_t pkeysz;

  char *rsa_pkey;
  void *rsa_pkey_ptr;

  char *dsa_pkey;
  void *dsa_pkey_ptr;

#ifdef PR_USE_OPENSSL_ECC
  char *ec_pkey;
  void *ec_pkey_ptr;
#endif /* PR_USE_OPENSSL_ECC */

  /* Used for stashing the password for a PKCS12 file, which should
   * contain a certificate.  Any passphrase for the private key for that
   * certificate should be in one of the above RSA/DSA buffers.
   */
  char *pkcs12_passwd;
  void *pkcs12_passwd_ptr;

  unsigned int flags;

  server_rec *server;

} tls_pkey_t;

#define TLS_PKEY_USE_RSA		0x0100
#define TLS_PKEY_USE_DSA		0x0200
#define TLS_PKEY_USE_EC			0x0400

static tls_pkey_t *tls_pkey_list = NULL;
static unsigned int tls_npkeys = 0;

#define TLS_DEFAULT_CIPHER_SUITE	"DEFAULT:!ADH:!EXPORT:!DES"

/* Module variables */
#if OPENSSL_VERSION_NUMBER > 0x000907000L
static const char *tls_crypto_device = NULL;
#endif
static unsigned char tls_engine = FALSE;
static unsigned long tls_flags = 0UL, tls_opts = 0UL;
static tls_pkey_t *tls_pkey = NULL;
static int tls_logfd = -1;
static char *tls_logname = NULL;

static char *tls_passphrase_provider = NULL;
#define TLS_PASSPHRASE_TIMEOUT		10
#define TLS_PASSPHRASE_FL_RSA_KEY	0x0001
#define TLS_PASSPHRASE_FL_DSA_KEY	0x0002
#define TLS_PASSPHRASE_FL_PKCS12_PASSWD	0x0004
#define TLS_PASSPHRASE_FL_EC_KEY	0x0008

#define TLS_PROTO_SSL_V3		0x0001
#define TLS_PROTO_TLS_V1		0x0002
#define TLS_PROTO_TLS_V1_1		0x0004
#define TLS_PROTO_TLS_V1_2		0x0008

#if OPENSSL_VERSION_NUMBER >= 0x10001000L
# define TLS_PROTO_DEFAULT		(TLS_PROTO_TLS_V1|TLS_PROTO_TLS_V1_1|TLS_PROTO_TLS_V1_2)
#else
# define TLS_PROTO_DEFAULT		(TLS_PROTO_TLS_V1)
#endif /* OpenSSL 1.0.1 or later */

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
static int tls_ssl_opts = (SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_SINGLE_DH_USE)^SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
#else
/* OpenSSL-0.9.6 and earlier (yes, it appears people still have these versions
 * installed) does not define the DONT_INSERT_EMPTY_FRAGMENTS option.
 */
static int tls_ssl_opts = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_SINGLE_DH_USE;
#endif

static int tls_required_on_auth = 0;
static int tls_required_on_ctrl = 0;
static int tls_required_on_data = 0;
static unsigned char *tls_authenticated = NULL;

/* Define the minimum DH group length we allow (unless the AllowWeakDH
 * TLSOption is used).  Ideally this would be 2048, per https://weakdh.org,
 * but for compatibility with older Java versions, which only support up to
 * 1024, we'll use 1024.  For now.
 */
#define TLS_DH_MIN_LEN				1024

/* mod_tls session flags */
#define	TLS_SESS_ON_CTRL			0x0001
#define TLS_SESS_ON_DATA			0x0002
#define TLS_SESS_PBSZ_OK			0x0004
#define TLS_SESS_TLS_REQUIRED			0x0010
#define TLS_SESS_VERIFY_CLIENT			0x0020
#define TLS_SESS_NO_PASSWD_NEEDED		0x0040
#define TLS_SESS_NEED_DATA_PROT			0x0100
#define TLS_SESS_CTRL_RENEGOTIATING		0x0200
#define TLS_SESS_DATA_RENEGOTIATING		0x0400
#define TLS_SESS_HAVE_CCC			0x0800
#define TLS_SESS_VERIFY_SERVER			0x1000
#define TLS_SESS_VERIFY_SERVER_NO_DNS		0x2000

/* mod_tls option flags */
#define TLS_OPT_NO_CERT_REQUEST				0x0001
#define TLS_OPT_VERIFY_CERT_FQDN			0x0002
#define TLS_OPT_VERIFY_CERT_IP_ADDR			0x0004
#define TLS_OPT_ALLOW_DOT_LOGIN				0x0008
#define TLS_OPT_EXPORT_CERT_DATA			0x0010
#define TLS_OPT_STD_ENV_VARS				0x0020
#define TLS_OPT_ALLOW_PER_USER				0x0040
#define TLS_OPT_ENABLE_DIAGS				0x0080
#define TLS_OPT_NO_SESSION_REUSE_REQUIRED		0x0100
#define TLS_OPT_USE_IMPLICIT_SSL			0x0200
#define TLS_OPT_ALLOW_CLIENT_RENEGOTIATIONS		0x0400
#define TLS_OPT_VERIFY_CERT_CN				0x0800
#define TLS_OPT_ALLOW_WEAK_DH				0x1000

/* mod_tls SSCN modes */
#define TLS_SSCN_MODE_SERVER				0
#define TLS_SSCN_MODE_CLIENT				1
static unsigned int tls_sscn_mode = TLS_SSCN_MODE_SERVER;

/* mod_tls cleanup flags */
#define TLS_CLEANUP_FL_SESS_INIT	0x0001

/* mod_tls OCSP constants */
#define TLS_OCSP_RESP_MAX_AGE_SECS	300

static char *tls_cipher_suite = NULL;
static char *tls_crl_file = NULL, *tls_crl_path = NULL;
static char *tls_dhparam_file = NULL;
static char *tls_ec_cert_file = NULL, *tls_ec_key_file = NULL;
static char *tls_dsa_cert_file = NULL, *tls_dsa_key_file = NULL;
static char *tls_pkcs12_file = NULL;
static char *tls_rsa_cert_file = NULL, *tls_rsa_key_file = NULL;
static char *tls_rand_file = NULL;

/* Timeout given for TLS handshakes.  The default is 5 minutes. */
static unsigned int tls_handshake_timeout = 300;
static unsigned char tls_handshake_timed_out = FALSE;
static int tls_handshake_timer_id = -1;

/* Note: 9 is the default OpenSSL depth. */
static int tls_verify_depth = 9;

#if OPENSSL_VERSION_NUMBER > 0x000907000L
/* Renegotiate control channel on TLS sessions after 4 hours, by default. */
static int tls_ctrl_renegotiate_timeout = 14400;

/* Renegotiate data channel on TLS sessions after 1 gigabyte, by default. */
static off_t tls_data_renegotiate_limit = 1024 * 1024 * 1024;

/* Timeout given for renegotiations to occur before the TLS session is
 * shutdown.  The default is 30 seconds.
 */
static int tls_renegotiate_timeout = 30;

/* Is client acceptance of a requested renegotiation required? */
static unsigned char tls_renegotiate_required = TRUE;
#endif

#define TLS_NETIO_NOTE		"mod_tls.SSL"

static pr_netio_t *tls_ctrl_netio = NULL;
static pr_netio_stream_t *tls_ctrl_rd_nstrm = NULL;
static pr_netio_stream_t *tls_ctrl_wr_nstrm = NULL;

static pr_netio_t *tls_data_netio = NULL;
static pr_netio_stream_t *tls_data_rd_nstrm = NULL;
static pr_netio_stream_t *tls_data_wr_nstrm = NULL;

static tls_sess_cache_t *tls_sess_cache = NULL;

/* OpenSSL variables */
static SSL *ctrl_ssl = NULL;
static SSL_CTX *ssl_ctx = NULL;
static X509_STORE *tls_crl_store = NULL;
static array_header *tls_tmp_dhs = NULL;
static RSA *tls_tmp_rsa = NULL;

static int tls_sess_init(void);

/* SSL/TLS support functions */
static void tls_closelog(void);
static void tls_end_sess(SSL *, int, int);
#define TLS_SHUTDOWN_BIDIRECTIONAL	0x0001

static void tls_fatal_error(long, int);
static const char *tls_get_errors(void);
static char *tls_get_page(size_t, void **);
static size_t tls_get_pagesz(void);
static int tls_get_passphrase(server_rec *, const char *, const char *,
  char *, size_t, int);

static char *tls_get_subj_name(SSL *);

static int tls_openlog(void);
static RSA *tls_rsa_cb(SSL *, int, int);
static int tls_seed_prng(void);
static void tls_setup_environ(SSL *);
static int tls_verify_cb(int, X509_STORE_CTX *);
static int tls_verify_crl(int, X509_STORE_CTX *);
static int tls_verify_ocsp(int, X509_STORE_CTX *);
static char *tls_x509_name_oneline(X509_NAME *);

static int tls_readmore(int);
static int tls_writemore(int);

/* Session cache API */
static tls_sess_cache_t *tls_sess_cache_get_cache(const char *);
static long tls_sess_cache_get_cache_mode(void);
static int tls_sess_cache_open(char *, long);
static int tls_sess_cache_close(void);
#ifdef PR_USE_CTRLS
static int tls_sess_cache_clear(void);
static int tls_sess_cache_remove(void);
static int tls_sess_cache_status(pr_ctrls_t *, int);
#endif /* PR_USE_CTRLS */
static int tls_sess_cache_add_sess_cb(SSL *, SSL_SESSION *);
static SSL_SESSION *tls_sess_cache_get_sess_cb(SSL *, unsigned char *, int,
  int *);
static void tls_sess_cache_delete_sess_cb(SSL_CTX *, SSL_SESSION *);

#ifdef PR_USE_CTRLS
static pool *tls_act_pool = NULL;
static ctrls_acttab_t tls_acttab[];
#endif /* PR_USE_CTRLS */

static int tls_ctrl_need_init_handshake = TRUE;
static int tls_data_need_init_handshake = TRUE;

static const char *trace_channel = "tls";

static void tls_diags_cb(const SSL *ssl, int where, int ret) {
  const char *str = "(unknown)";
  int w;

  pr_signals_handle();

  w = where & ~SSL_ST_MASK;

  if (w & SSL_ST_CONNECT) {
    str = "connecting";

  } else if (w & SSL_ST_ACCEPT) {
    str = "accepting";

  } else {
    int ssl_state;

    ssl_state = SSL_get_state(ssl);
    if (ssl_state == SSL_ST_OK) {
      str = "ok";
    }
  }

  if (where & SSL_CB_ACCEPT_LOOP) {
    int ssl_state;

    ssl_state = SSL_get_state(ssl);

    if (ssl_state == SSL3_ST_SR_CLNT_HELLO_A ||
        ssl_state == SSL23_ST_SR_CLNT_HELLO_A) {

      /* If we have already completed our initial handshake, then this might
       * a session renegotiation.
       */
      if ((ssl == ctrl_ssl && !tls_ctrl_need_init_handshake) ||
          (ssl != ctrl_ssl && !tls_data_need_init_handshake)) {

        /* Yes, this is indeed a session renegotiation. If it's a
         * renegotiation that we requested, allow it.  If it is from a
         * data connection, allow it.  Otherwise, it's a client-initiated
         * renegotiation, and we probably don't want to allow it.
         */

        if (ssl == ctrl_ssl &&
            !(tls_flags & TLS_SESS_CTRL_RENEGOTIATING) &&
            !(tls_flags & TLS_SESS_DATA_RENEGOTIATING)) {

          if (!(tls_opts & TLS_OPT_ALLOW_CLIENT_RENEGOTIATIONS)) {
            tls_log("warning: client-initiated session renegotiation "
              "detected, aborting connection");
            pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION
              ": client-initiated session renegotiation detected, "
              "aborting connection");

            tls_end_sess(ctrl_ssl, PR_NETIO_STRM_CTRL, 0);
            pr_table_remove(tls_ctrl_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
            pr_table_remove(tls_ctrl_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
            ctrl_ssl = NULL;

            pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_CONFIG_ACL,
              "TLSOption AllowClientRenegotiations");
          }
        }
      }

#if OPENSSL_VERSION_NUMBER >= 0x009080cfL
    } else if (ssl_state & SSL_ST_RENEGOTIATE) {
      if ((ssl == ctrl_ssl && !tls_ctrl_need_init_handshake) ||
          (ssl != ctrl_ssl && !tls_data_need_init_handshake)) {

        if (ssl == ctrl_ssl &&
            !(tls_flags & TLS_SESS_CTRL_RENEGOTIATING) &&
            !(tls_flags & TLS_SESS_DATA_RENEGOTIATING)) {

          /* In OpenSSL-0.9.8l and later, SSL session renegotiations are
           * automatically disabled.  Thus if the admin has not explicitly
           * configured support for client-initiated renegotations via the
           * AllowClientRenegotiations TLSOption, then we need to disconnect
           * the client here.  Otherwise, the client would hang (up to the
           * TLSTimeoutHandshake limit).  Since we know, right now, that the
           * handshake won't succeed, just drop the connection.
           */

          if (!(tls_opts & TLS_OPT_ALLOW_CLIENT_RENEGOTIATIONS)) {
            tls_log("warning: client-initiated session renegotiation detected, "
              "aborting connection");
            pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION
              ": client-initiated session renegotiation detected, "
              "aborting connection");

            tls_end_sess(ctrl_ssl, PR_NETIO_STRM_CTRL, 0);
            pr_table_remove(tls_ctrl_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
            pr_table_remove(tls_ctrl_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
            ctrl_ssl = NULL;

            pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_CONFIG_ACL,
              "TLSOption AllowClientRenegotiations");
          }
        }
      }
#endif
    }

    if (tls_opts & TLS_OPT_ENABLE_DIAGS) {
      tls_log("[info] %s: %s", str, SSL_state_string_long(ssl));
    }

  } else if (where & SSL_CB_HANDSHAKE_DONE) {
    if (ssl == ctrl_ssl) {
      if (tls_ctrl_need_init_handshake == FALSE) {
        int reused;

        /* If this is an accepted renegotiation, log the possibly-changed
         * ciphersuite et al.
         */

        reused = SSL_session_reused((SSL *) ssl);
        tls_log("%s renegotiation accepted, using cipher %s (%d bits%s)",
          SSL_get_version(ssl), SSL_get_cipher_name(ssl),
          SSL_get_cipher_bits(ssl, NULL),
          reused > 0 ? ", resumed session" : "");
      }

      tls_ctrl_need_init_handshake = FALSE;

    } else {
      if (tls_data_need_init_handshake == FALSE) {
        /* If this is an accepted renegotiation, log the possibly-changed
         * ciphersuite et al.
         */
        tls_log("%s renegotiation accepted, using cipher %s (%d bits)",
          SSL_get_version(ssl), SSL_get_cipher_name(ssl),
          SSL_get_cipher_bits(ssl, NULL));
      }

      tls_data_need_init_handshake = FALSE;
    }

    /* Clear the flags set for server-requested renegotiations. */
    if (tls_flags & TLS_SESS_CTRL_RENEGOTIATING) {
      tls_flags &= ~TLS_SESS_CTRL_RENEGOTIATING;
    }

    if (tls_flags & ~TLS_SESS_DATA_RENEGOTIATING) {
      tls_flags &= ~TLS_SESS_DATA_RENEGOTIATING;
    }

    if (tls_opts & TLS_OPT_ENABLE_DIAGS) {
      tls_log("[info] %s: %s", str, SSL_state_string_long(ssl));
    }

  } else if (where & SSL_CB_LOOP) {
    if (tls_opts & TLS_OPT_ENABLE_DIAGS) {
      tls_log("[info] %s: %s", str, SSL_state_string_long(ssl));
    }

  } else if (where & SSL_CB_ALERT) {
    if (tls_opts & TLS_OPT_ENABLE_DIAGS) {
      str = (where & SSL_CB_READ) ? "reading" : "writing";
      tls_log("[info] %s: SSL/TLS alert %s: %s", str,
        SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret));
    }

  } else if (where & SSL_CB_EXIT) {
    if (tls_opts & TLS_OPT_ENABLE_DIAGS) {
      if (ret == 0) {
        tls_log("[info] %s: failed in %s: %s", str,
          SSL_state_string_long(ssl), tls_get_errors());

      } else if (ret < 0 &&
                 errno != 0 &&
                 errno != EAGAIN) {
        /* Ignore EAGAIN errors */
        tls_log("[info] %s: error in %s (errno %d: %s)",
          str, SSL_state_string_long(ssl), errno, strerror(errno));
      }
    }
  }
}

#if OPENSSL_VERSION_NUMBER > 0x000907000L
static void tls_msg_cb(int io_flag, int version, int content_type,
    const void *buf, size_t buflen, SSL *ssl, void *arg) {
  char *action_str = NULL;
  char *version_str = NULL;
  char *bytes_str = buflen != 1 ? "bytes" : "byte";

  if (io_flag == 0) {
    action_str = "received";

  } else if (io_flag == 1) {
    action_str = "sent";
  }

  switch (version) {
    case SSL2_VERSION:
      version_str = "SSLv2";
      break;

    case SSL3_VERSION:
      version_str = "SSLv3";
      break;

    case TLS1_VERSION:
      version_str = "TLSv1";
      break;

#if OPENSSL_VERSION_NUMBER >= 0x10001000L
    case TLS1_1_VERSION:
      version_str = "TLSv1.1";
      break;

    case TLS1_2_VERSION:
      version_str = "TLSv1.2";
      break;
#endif

    default:
      tls_log("[msg] unknown/unsupported version: %d", version);
      break;
  }

  if (version == SSL3_VERSION ||
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
      version == TLS1_1_VERSION ||
      version == TLS1_2_VERSION ||
#endif
      version == TLS1_VERSION) {

    switch (content_type) {
      case 20:
        /* ChangeCipherSpec message */
        tls_log("[msg] %s %s ChangeCipherSpec message (%u %s)",
          action_str, version_str, (unsigned int) buflen, bytes_str);
        break;

      case 21: {
        /* Alert messages */
        if (buflen == 2) {
          char *severity_str = NULL;

          /* Peek naughtily into the buffer. */
          switch (((const unsigned char *) buf)[0]) {
            case 1:
              severity_str = "warning";
              break;

            case 2:
              severity_str = "fatal";
              break;
          }

          switch (((const unsigned char *) buf)[1]) {
            case 0:
              tls_log("[msg] %s %s %s 'close_notify' Alert message (%u %s)",
                action_str, version_str, severity_str, (unsigned int) buflen,
                bytes_str);
              break;

            case 10:
              tls_log("[msg] %s %s %s 'unexpected_message' Alert message "
                "(%u %s)", action_str, version_str, severity_str,
                (unsigned int) buflen, bytes_str);
              break;

            case 20:
              tls_log("[msg] %s %s %s 'bad_record_mac' Alert message (%u %s)",
                action_str, version_str, severity_str, (unsigned int) buflen,
                bytes_str);
              break;

            case 21:
              tls_log("[msg] %s %s %s 'decryption_failed' Alert message "
                "(%u %s)", action_str, version_str, severity_str,
                (unsigned int) buflen, bytes_str);
              break;

            case 22:
              tls_log("[msg] %s %s %s 'record_overflow' Alert message (%u %s)",
                action_str, version_str, severity_str, (unsigned int) buflen,
                bytes_str);
              break;

            case 30:
              tls_log("[msg] %s %s %s 'decompression_failure' Alert message "
                "(%u %s)", action_str, version_str, severity_str,
                (unsigned int) buflen, bytes_str);
              break;

            case 40:
              tls_log("[msg] %s %s %s 'handshake_failure' Alert message "
                "(%u %s)", action_str, version_str, severity_str,
                (unsigned int) buflen, bytes_str);
              break;
          }

        } else {
          tls_log("[msg] %s %s Alert message, unknown type (%u %s)", action_str,
            version_str, (unsigned int) buflen, bytes_str);
        }

        break;
      }

      case 22: {
        /* Handshake messages */
        if (buflen > 0) {
          /* Peek naughtily into the buffer. */
          switch (((const unsigned char *) buf)[0]) {
            case 0:
              tls_log("[msg] %s %s 'HelloRequest' Handshake message (%u %s)",
                action_str, version_str, (unsigned int) buflen, bytes_str);
              break;

            case 1:
              tls_log("[msg] %s %s 'ClientHello' Handshake message (%u %s)",
                action_str, version_str, (unsigned int) buflen, bytes_str);
              break;

            case 2:
              tls_log("[msg] %s %s 'ServerHello' Handshake message (%u %s)",
                action_str, version_str, (unsigned int) buflen, bytes_str);
              break;

            case 11:
              tls_log("[msg] %s %s 'Certificate' Handshake message (%u %s)",
                action_str, version_str, (unsigned int) buflen, bytes_str);
              break;

            case 12:
              tls_log("[msg] %s %s 'ServerKeyExchange' Handshake message "
                "(%u %s)", action_str, version_str, (unsigned int) buflen,
                bytes_str);
              break;

            case 13:
              tls_log("[msg] %s %s 'CertificateRequest' Handshake message "
                "(%u %s)", action_str, version_str, (unsigned int) buflen,
                bytes_str);
              break;

            case 14:
              tls_log("[msg] %s %s 'ServerHelloDone' Handshake message (%u %s)",
                action_str, version_str, (unsigned int) buflen, bytes_str);
              break;

            case 15:
              tls_log("[msg] %s %s 'CertificateVerify' Handshake message "
                "(%u %s)", action_str, version_str, (unsigned int) buflen,
                bytes_str);
              break;

            case 16:
              tls_log("[msg] %s %s 'ClientKeyExchange' Handshake message "
                "(%u %s)", action_str, version_str, (unsigned int) buflen,
                bytes_str);
              break;

            case 20:
              tls_log("[msg] %s %s 'Finished' Handshake message (%u %s)",
                action_str, version_str, (unsigned int) buflen, bytes_str);
              break;
          }

        } else {
          tls_log("[msg] %s %s Handshake message, unknown type (%u %s)",
            action_str, version_str, (unsigned int) buflen, bytes_str);
        }

        break;
      }
    }

  } else if (version == SSL2_VERSION) {
    /* SSLv2 message.  Ideally we wouldn't get these, but sometimes badly
     * behaving FTPS clients send them.
     */

    if (buflen > 0) {
      /* Peek naughtily into the buffer. */

      switch (((const unsigned char *) buf)[0]) {
        case 0: {
          /* Error */
          if (buflen > 3) {
            unsigned err_code = (((const unsigned char *) buf)[1] << 8) +
              ((const unsigned char *) buf)[2];

            switch (err_code) {
              case 0x0001:
                tls_log("[msg] %s %s 'NO-CIPHER-ERROR' Error message (%u %s)",
                  action_str, version_str, (unsigned int) buflen, bytes_str);
                break;

              case 0x0002:
                tls_log("[msg] %s %s 'NO-CERTIFICATE-ERROR' Error message "
                  "(%u %s)", action_str, version_str, (unsigned int) buflen,
                  bytes_str);
                break;

              case 0x0004:
                tls_log("[msg] %s %s 'BAD-CERTIFICATE-ERROR' Error message "
                  "(%u %s)", action_str, version_str, (unsigned int) buflen,
                  bytes_str);
                break;

              case 0x0006:
                tls_log("[msg] %s %s 'UNSUPPORTED-CERTIFICATE-TYPE-ERROR' "
                  "Error message (%u %s)", action_str, version_str,
                  (unsigned int) buflen, bytes_str);
                break;
            }

          } else {
            tls_log("[msg] %s %s Error message, unknown type (%u %s)",
              action_str, version_str, (unsigned int) buflen, bytes_str);
          }
          break;
        }

        case 1:
          tls_log("[msg] %s %s 'CLIENT-HELLO' message (%u %s)", action_str,
            version_str, (unsigned int) buflen, bytes_str);
          break;

        case 2:
          tls_log("[msg] %s %s 'CLIENT-MASTER-KEY' message (%u %s)", action_str,
            version_str, (unsigned int) buflen, bytes_str);
          break;

        case 3:
          tls_log("[msg] %s %s 'CLIENT-FINISHED' message (%u %s)", action_str,
            version_str, (unsigned int) buflen, bytes_str);
          break;

        case 4:
          tls_log("[msg] %s %s 'SERVER-HELLO' message (%u %s)", action_str,
            version_str, (unsigned int) buflen, bytes_str);
          break;

        case 5:
          tls_log("[msg] %s %s 'SERVER-VERIFY' message (%u %s)", action_str,
            version_str, (unsigned int) buflen, bytes_str);
          break;

        case 6:
          tls_log("[msg] %s %s 'SERVER-FINISHED' message (%u %s)", action_str,
            version_str, (unsigned int) buflen, bytes_str);
          break;

        case 7:
          tls_log("[msg] %s %s 'REQUEST-CERTIFICATE' message (%u %s)",
            action_str, version_str, (unsigned int) buflen, bytes_str);
          break;

        case 8:
          tls_log("[msg] %s %s 'CLIENT-CERTIFICATE' message (%u %s)",
            action_str, version_str, (unsigned int) buflen, bytes_str);
          break;
      }

    } else {
      tls_log("[msg] %s %s message (%u %s)", action_str, version_str,
        (unsigned int) buflen, bytes_str);
    }

  } else {
    /* This case might indicate an issue with OpenSSL itself; the version
     * given to the msg_callback function was not initialized, or not set to
     * one of the recognized SSL/TLS versions.  Weird.
     */

    tls_log("[msg] %s message of unknown version (%d) (%u %s)", action_str,
      version, (unsigned int) buflen, bytes_str);
  }

}
#endif

static const char *get_printable_subjaltname(pool *p, const char *data,
    size_t datalen) {
  register unsigned int i;
  char *ptr, *res;
  size_t reslen = 0;

  /* First, calculate the length of the resulting printable string we'll
   * be generating.
   */

  for (i = 0; i < datalen; i++) {
    if (PR_ISPRINT(data[i])) {
      reslen++;

    } else {
      reslen += 4;
    }
  }

  /* Leave one space in the allocated string for the terminating NUL. */
  ptr = res = pcalloc(p, reslen + 1);

  for (i = 0; i < datalen; i++) {
    if (PR_ISPRINT(data[i])) {
      *(ptr++) = data[i];

    } else {
      snprintf(ptr, reslen - (ptr - res), "\\x%02x", data[i]);
      ptr += 4;
    }
  }

  return res;
}

static int tls_cert_match_dns_san(pool *p, X509 *cert, const char *dns_name) {
  int matched = 0;
#if OPENSSL_VERSION_NUMBER > 0x000907000L
  STACK_OF(GENERAL_NAME) *sans;

  sans = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
  if (sans != NULL) {
    register unsigned int i;
    int nsans = sk_GENERAL_NAME_num(sans);

    for (i = 0; i < nsans; i++) {
      GENERAL_NAME *alt_name = sk_GENERAL_NAME_value(sans, i);

      if (alt_name->type == GEN_DNS) {
        char *dns_san;
        size_t dns_sanlen;

        dns_san = (char *) ASN1_STRING_data(alt_name->d.ia5);
        dns_sanlen = strlen(dns_san);

        /* Check for subjectAltName values which contain embedded NULs.
         * This can cause verification problems (spoofing), e.g. if the
         * string is "www.goodguy.com\0www.badguy.com"; the use of strcmp()
         * only checks "www.goodguy.com".
         */

        if (ASN1_STRING_length(alt_name->d.ia5) != dns_sanlen) {
          tls_log("%s", "cert dNSName SAN contains embedded NULs, "
            "rejecting as possible spoof attempt");
          tls_log("suspicious dNSName SAN value: '%s'",
            get_printable_subjaltname(p, dns_san,
              ASN1_STRING_length(alt_name->d.dNSName)));

          GENERAL_NAME_free(alt_name);
          sk_GENERAL_NAME_free(sans);
          return 0;
        }

        if (strncasecmp(dns_name, dns_san, dns_sanlen + 1) == 0) {
          pr_trace_msg(trace_channel, 8,
            "found cert dNSName SAN matching '%s'", dns_name);
          matched = 1;

        } else {
          pr_trace_msg(trace_channel, 9,
            "cert dNSName SAN '%s' did not match '%s'", dns_san, dns_name);
        }
      }

      GENERAL_NAME_free(alt_name);
 
      if (matched == 1) {
        break;
      }
    }

    sk_GENERAL_NAME_free(sans);
  }
#endif /* OpenSSL-0.9.7 or later */

  return matched;
}

static int tls_cert_match_ip_san(pool *p, X509 *cert, const char *ipstr) {
  int matched = 0;
  STACK_OF(GENERAL_NAME) *sans;

  sans = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
  if (sans != NULL) {
    register unsigned int i;
    int nsans = sk_GENERAL_NAME_num(sans);

    for (i = 0; i < nsans; i++) {
      GENERAL_NAME *alt_name = sk_GENERAL_NAME_value(sans, i);

      if (alt_name->type == GEN_IPADD) {
        unsigned char *san_data = NULL;
        int have_ipstr = FALSE, san_datalen;
#ifdef PR_USE_IPV6
        char san_ipstr[INET6_ADDRSTRLEN + 1] = {'\0'};
#else
        char san_ipstr[INET_ADDRSTRLEN + 1] = {'\0'};
#endif /* PR_USE_IPV6 */

        san_data = ASN1_STRING_data(alt_name->d.ip);
        memset(san_ipstr, '\0', sizeof(san_ipstr));

        san_datalen = ASN1_STRING_length(alt_name->d.ip);
        if (san_datalen == 4) {
          /* IPv4 address */
          snprintf(san_ipstr, sizeof(san_ipstr)-1, "%u.%u.%u.%u",
            san_data[0], san_data[1], san_data[2], san_data[3]);
          have_ipstr = TRUE;

#ifdef PR_USE_IPV6
        } else if (san_datalen == 16) {
          /* IPv6 address */

          if (pr_inet_ntop(AF_INET6, san_data, san_ipstr,
              sizeof(san_ipstr)-1) == NULL) {
            tls_log("unable to convert cert iPAddress SAN value (length %d) "
              "to IPv6 representation: %s", san_datalen, strerror(errno));

          } else {
            have_ipstr = TRUE;
          }

#endif /* PR_USE_IPV6 */
        } else {
          pr_trace_msg(trace_channel, 3,
            "unsupported cert SAN ipAddress length (%d), ignoring",
            san_datalen);
          continue;
        }

        if (have_ipstr) {
          size_t san_ipstrlen;

          san_ipstrlen = strlen(san_ipstr);

          if (strncmp(ipstr, san_ipstr, san_ipstrlen + 1) == 0) {
            pr_trace_msg(trace_channel, 8,
              "found cert iPAddress SAN matching '%s'", ipstr);
            matched = 1;
 
          } else {
            if (san_datalen == 16) {
              /* We need to handle the case where the iPAddress SAN might
               * have contained an IPv4-mapped IPv6 adress, and we're
               * comparing against an IPv4 address.
               */
              if (san_ipstrlen > 7 &&
                  strncasecmp(san_ipstr, "::ffff:", 7) == 0) {
                if (strncmp(ipstr, san_ipstr + 7, san_ipstrlen - 6) == 0) {
                  pr_trace_msg(trace_channel, 8,
                    "found cert iPAddress SAN '%s' matching '%s'",
                    san_ipstr, ipstr);
                    matched = 1;
                }
              }

            } else {
              pr_trace_msg(trace_channel, 9,
                "cert iPAddress SAN '%s' did not match '%s'", san_ipstr, ipstr);
            }
          }
        }
      }

      GENERAL_NAME_free(alt_name);
 
      if (matched == 1) {
        break;
      }
    }

    sk_GENERAL_NAME_free(sans);
  }

  return matched;
}

static int tls_cert_match_cn(pool *p, X509 *cert, const char *name,
    int allow_wildcards) {
  int matched = 0, idx = -1;
  X509_NAME *subj_name = NULL;
  X509_NAME_ENTRY *cn_entry = NULL;
  ASN1_STRING *cn_asn1 = NULL;
  char *cn_str = NULL;
  size_t cn_len = 0;

  /* Find the position of the CommonName (CN) field within the Subject of
   * the cert.
   */
  subj_name = X509_get_subject_name(cert);
  if (subj_name == NULL) {
    pr_trace_msg(trace_channel, 12,
      "unable to check certificate CommonName against '%s': "
      "unable to get Subject", name);
    return 0;
  }

  idx = X509_NAME_get_index_by_NID(subj_name, NID_commonName, -1);
  if (idx < 0) {
    pr_trace_msg(trace_channel, 12,
      "unable to check certificate CommonName against '%s': "
      "no CommoName atribute found", name);
    return 0;
  }

  cn_entry = X509_NAME_get_entry(subj_name, idx);
  if (cn_entry == NULL) {
    pr_trace_msg(trace_channel, 12,
      "unable to check certificate CommonName against '%s': "
      "error obtaining CommoName atribute found: %s", name, tls_get_errors());
    return 0;
  }

  /* Convert the CN field to a string, by way of an ASN1 object. */
  cn_asn1 = X509_NAME_ENTRY_get_data(cn_entry);
  if (cn_asn1 == NULL) {
    X509_NAME_ENTRY_free(cn_entry);

    pr_trace_msg(trace_channel, 12,
      "unable to check certificate CommonName against '%s': "
      "error converting CommoName atribute to ASN.1: %s", name,
      tls_get_errors());
    return 0;
  }

  cn_str = (char *) ASN1_STRING_data(cn_asn1);

  /* Check for CommonName values which contain embedded NULs.  This can cause
   * verification problems (spoofing), e.g. if the string is
   * "www.goodguy.com\0www.badguy.com"; the use of strcmp() only checks
   * "www.goodguy.com".
   */

  cn_len = strlen(cn_str);

  if (ASN1_STRING_length(cn_asn1) != cn_len) {
    X509_NAME_ENTRY_free(cn_entry);

    tls_log("%s", "cert CommonName contains embedded NULs, rejecting as "
      "possible spoof attempt");
    tls_log("suspicious CommonName value: '%s'",
      get_printable_subjaltname(p, (const char *) cn_str,
        ASN1_STRING_length(cn_asn1)));
    return 0;
  }

  /* Yes, this is deliberately a case-insensitive comparison.  Most CNs
   * contain a hostname (case-insensitive); if they contain an IP address,
   * the case-insensitivity won't hurt anything.  In fact, it's needed for
   * e.g. IPv6 addresses.
   */
  if (strncasecmp(name, cn_str, cn_len + 1) == 0) {
    matched = 1;
  }

  if (matched == 0 &&
      allow_wildcards) {

    /* XXX Implement wildcard checking. */
  }
 
  X509_NAME_ENTRY_free(cn_entry);
  return matched;
}

static int tls_check_client_cert(SSL *ssl, conn_t *conn) {
  X509 *cert = NULL;
  unsigned char have_cn = FALSE, have_dns_ext = FALSE, have_ipaddr_ext = FALSE;
  int ok = -1;

  /* Only perform these more stringent checks if asked to verify clients. */
  if (!(tls_flags & TLS_SESS_VERIFY_CLIENT)) {
    return 0;
  }

  /* Only perform these checks is configured to do so. */
  if (!(tls_opts & TLS_OPT_VERIFY_CERT_FQDN) &&
      !(tls_opts & TLS_OPT_VERIFY_CERT_IP_ADDR) &&
      !(tls_opts & TLS_OPT_VERIFY_CERT_CN)) {
    return 0;
  }

  cert = SSL_get_peer_certificate(ssl);
  if (cert == NULL) {
    /* This can be null in the case where some anonymous (insecure)
     * cipher suite was used.
     */
    tls_log("unable to verify '%s': client did not provide certificate",
      conn->remote_name);
    return -1;
  }

  /* Check the DNS SANs if configured. */
  if (tls_opts & TLS_OPT_VERIFY_CERT_FQDN) {
    int matched;

    matched = tls_cert_match_dns_san(conn->pool, cert, conn->remote_name);
    if (matched == 0) {
      tls_log("client cert dNSName SANs do not match remote name '%s'",
        conn->remote_name);
      return -1;

    } else {
      tls_log("client cert dNSName SAN matches remote name '%s'",
        conn->remote_name);
      have_dns_ext = TRUE;
      ok = 1;
    }
  }

  /* Check the IP SANs, if configured. */
  if (tls_opts & TLS_OPT_VERIFY_CERT_IP_ADDR) {
    int matched;

    matched = tls_cert_match_ip_san(conn->pool, cert,
      pr_netaddr_get_ipstr(conn->remote_addr));
    if (matched == 0) {
      tls_log("client cert iPAddress SANs do not match client IP '%s'",
        pr_netaddr_get_ipstr(conn->remote_addr));
      return -1;

    } else {
      tls_log("client cert iPAddress SAN matches client IP '%s'",
        pr_netaddr_get_ipstr(conn->remote_addr));
      have_ipaddr_ext = TRUE;
      ok = 1;
    }
  }

  /* Check the CN (Common Name) if configured. */
  if (tls_opts & TLS_OPT_VERIFY_CERT_CN) {
    int matched;

    matched = tls_cert_match_cn(conn->pool, cert, conn->remote_name, TRUE);
    if (matched == 0) {
      tls_log("client cert CommonName does not match client FQDN '%s'",
        conn->remote_name);
      return -1;

    } else {
      tls_log("client cert CommonName matches client FQDN '%s'",
        conn->remote_name);
      have_cn = TRUE;
      ok = 1;
    }
  }

  if ((tls_opts & TLS_OPT_VERIFY_CERT_CN) &&
      !have_cn) {
    tls_log("%s", "client cert missing required X509v3 CommonName");
  }

  if ((tls_opts & TLS_OPT_VERIFY_CERT_FQDN) &&
      !have_dns_ext) {
    tls_log("%s", "client cert missing required X509v3 SubjectAltName dNSName");
  }

  if ((tls_opts & TLS_OPT_VERIFY_CERT_IP_ADDR) &&
      !have_ipaddr_ext) {
    tls_log("%s", "client cert missing required X509v3 SubjectAltName iPAddress");
  }

  X509_free(cert);
  return ok;
}

static int tls_check_server_cert(SSL *ssl, conn_t *conn) {
  X509 *cert = NULL;
  int ok = -1;
  long verify_result;

  /* Only perform these more stringent checks if asked to verify servers. */
  if (!(tls_flags & TLS_SESS_VERIFY_SERVER) &&
      !(tls_flags & TLS_SESS_VERIFY_SERVER_NO_DNS)) {
    return 0;
  }

  /* Check SSL_get_verify_result */
  verify_result = SSL_get_verify_result(ssl);
  if (verify_result != X509_V_OK) {
    tls_log("unable to verify '%s' server certificate: %s",
      conn->remote_name, X509_verify_cert_error_string(verify_result));
    return -1;
  }

  cert = SSL_get_peer_certificate(ssl);
  if (cert == NULL) {
    /* This can be null in the case where some anonymous (insecure)
     * cipher suite was used.
     */
    tls_log("unable to verify '%s': server did not provide certificate",
      conn->remote_name);
    return -1;
  }

  /* XXX If using OpenSSL-1.0.2/1.1.0, we might be able to use: 
   * X509_match_host() and X509_match_ip()/X509_match_ip_asc().
   */

  ok = tls_cert_match_ip_san(conn->pool, cert,
    pr_netaddr_get_ipstr(conn->remote_addr));
  if (ok == 0) {
    ok = tls_cert_match_cn(conn->pool, cert,
      pr_netaddr_get_ipstr(conn->remote_addr), FALSE);
  }

  if (ok == 0 &&
      !(tls_opts & TLS_SESS_VERIFY_SERVER_NO_DNS)) {
    int reverse_dns;
    const char *remote_name;

    reverse_dns = pr_netaddr_set_reverse_dns(TRUE);

    /* XXX Should clear the Netaddr cache here, but just for our single
     * name.  Should be an API for that.
     */
    pr_netaddr_clear_cache();

    conn->remote_addr->na_have_dnsstr = FALSE;
    remote_name = pr_netaddr_get_dnsstr(conn->remote_addr);
    pr_netaddr_set_reverse_dns(reverse_dns);

    ok = tls_cert_match_dns_san(conn->pool, cert, remote_name);
    if (ok == 0) {
      ok = tls_cert_match_cn(conn->pool, cert, remote_name, TRUE);
    }
  }

  X509_free(cert);

  ok = 0;
  return ok;
}

struct tls_pkey_data {
  server_rec *s;
  int flags;
  char *buf;
  size_t buflen, bufsz;
  const char *prompt;
};

static void tls_prepare_provider_fds(int stdout_fd, int stderr_fd) {
  unsigned long nfiles = 0;
  register unsigned int i = 0;
  struct rlimit rlim;

  if (stdout_fd != STDOUT_FILENO) {
    if (dup2(stdout_fd, STDOUT_FILENO) < 0)
      tls_log("error duping fd %d to stdout: %s", stdout_fd, strerror(errno));

    close(stdout_fd);
  }

  if (stderr_fd != STDERR_FILENO) {
    if (dup2(stderr_fd, STDERR_FILENO) < 0)
      tls_log("error duping fd %d to stderr: %s", stderr_fd, strerror(errno));

    close(stderr_fd);
  }

  /* Make sure not to pass on open file descriptors. For stdout and stderr,
   * we dup some pipes, so that we can capture what the command may write
   * to stdout or stderr.  The stderr output will be logged to the TLSLog.
   *
   * First, use getrlimit() to obtain the maximum number of open files
   * for this process -- then close that number.
   */
#if defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE)
# if defined(RLIMIT_NOFILE)
  if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) {
# elif defined(RLIMIT_OFILE)
  if (getrlimit(RLIMIT_OFILE, &rlim) < 0) {
# endif
    tls_log("getrlimit error: %s", strerror(errno));

    /* Pick some arbitrary high number. */
    nfiles = 255;

  } else
    nfiles = (unsigned long) rlim.rlim_max;
#else /* no RLIMIT_NOFILE or RLIMIT_OFILE */
   nfiles = 255;
#endif

  if (nfiles > 255)
    nfiles = 255;

  /* Close the "non-standard" file descriptors. */
  for (i = 3; i < nfiles; i++)
    (void) close(i);

  return;
}

static void tls_prepare_provider_pipes(int *stdout_pipe, int *stderr_pipe) {
  if (pipe(stdout_pipe) < 0) {
    tls_log("error opening stdout pipe: %s", strerror(errno));
    stdout_pipe[0] = -1;
    stdout_pipe[1] = STDOUT_FILENO;

  } else {
    if (fcntl(stdout_pipe[0], F_SETFD, FD_CLOEXEC) < 0)
      tls_log("error setting close-on-exec flag on stdout pipe read fd: %s",
        strerror(errno));

    if (fcntl(stdout_pipe[1], F_SETFD, 0) < 0)
      tls_log("error setting close-on-exec flag on stdout pipe write fd: %s",
        strerror(errno));
  }

  if (pipe(stderr_pipe) < 0) {
    tls_log("error opening stderr pipe: %s", strerror(errno));
    stderr_pipe[0] = -1;
    stderr_pipe[1] = STDERR_FILENO;

  } else {
    if (fcntl(stderr_pipe[0], F_SETFD, FD_CLOEXEC) < 0)
      tls_log("error setting close-on-exec flag on stderr pipe read fd: %s",
        strerror(errno));

    if (fcntl(stderr_pipe[1], F_SETFD, 0) < 0)
      tls_log("error setting close-on-exec flag on stderr pipe write fd: %s",
        strerror(errno));
  }
}

static int tls_exec_passphrase_provider(server_rec *s, char *buf, int buflen,
    int flags) {
  pid_t pid;
  int status;
  int stdout_pipe[2], stderr_pipe[2];

  struct sigaction sa_ignore, sa_intr, sa_quit;
  sigset_t set_chldmask, set_save;

  /* Prepare signal dispositions. */
  sa_ignore.sa_handler = SIG_IGN;
  sigemptyset(&sa_ignore.sa_mask);
  sa_ignore.sa_flags = 0;

  if (sigaction(SIGINT, &sa_ignore, &sa_intr) < 0)
    return -1;

  if (sigaction(SIGQUIT, &sa_ignore, &sa_quit) < 0)
    return -1;

  sigemptyset(&set_chldmask);
  sigaddset(&set_chldmask, SIGCHLD);

  if (sigprocmask(SIG_BLOCK, &set_chldmask, &set_save) < 0)
    return -1;

  tls_prepare_provider_pipes(stdout_pipe, stderr_pipe);

  pid = fork();
  if (pid < 0) {
    int xerrno = errno;

    pr_log_pri(PR_LOG_ALERT,
      MOD_TLS_VERSION ": error: unable to fork: %s", strerror(xerrno));

    errno = xerrno;
    status = -1;

  } else if (pid == 0) {
    char nbuf[32];
    pool *tmp_pool;
    char *stdin_argv[4];

    /* Child process */
    session.pid = getpid();

    /* Note: there is no need to clean up this temporary pool, as we've
     * forked.  If the exec call succeeds, this child process will exit
     * normally, and its process space recovered by the OS.  If the exec
     * call fails, we still exit, and the process space is recovered by
     * the OS.  Either way, the memory will be cleaned up without need for
     * us to do it explicitly (unless one wanted to be pedantic about it,
     * of course).
     */
    tmp_pool = make_sub_pool(s->pool);

    /* Restore previous signal actions. */
    sigaction(SIGINT, &sa_intr, NULL);
    sigaction(SIGQUIT, &sa_quit, NULL);
    sigprocmask(SIG_SETMASK, &set_save, NULL);

    stdin_argv[0] = pstrdup(tmp_pool, tls_passphrase_provider);

    memset(nbuf, '\0', sizeof(nbuf));
    snprintf(nbuf, sizeof(nbuf)-1, "%u", (unsigned int) s->ServerPort);
    nbuf[sizeof(nbuf)-1] = '\0';
    stdin_argv[1] = pstrcat(tmp_pool, s->ServerName, ":", nbuf, NULL);

    if (flags & TLS_PASSPHRASE_FL_RSA_KEY) {
      stdin_argv[2] = pstrdup(tmp_pool, "RSA");

    } else if (flags & TLS_PASSPHRASE_FL_DSA_KEY) {
      stdin_argv[2] = pstrdup(tmp_pool, "DSA");

    } else if (flags & TLS_PASSPHRASE_FL_EC_KEY) {
      stdin_argv[2] = pstrdup(tmp_pool, "EC");

    } else if (flags & TLS_PASSPHRASE_FL_PKCS12_PASSWD) {
      stdin_argv[2] = pstrdup(tmp_pool, "PKCS12");
    }

    stdin_argv[3] = NULL;

    PRIVS_ROOT

    pr_trace_msg(trace_channel, 17,
      "executing '%s' with uid %lu (euid %lu), gid %lu (egid %lu)",
      tls_passphrase_provider,
      (unsigned long) getuid(), (unsigned long) geteuid(),
      (unsigned long) getgid(), (unsigned long) getegid());

    pr_log_debug(DEBUG6, MOD_TLS_VERSION
      ": executing '%s' with uid %lu (euid %lu), gid %lu (egid %lu)",
      tls_passphrase_provider,
      (unsigned long) getuid(), (unsigned long) geteuid(),
      (unsigned long) getgid(), (unsigned long) getegid());

    /* Prepare the file descriptors that the process will inherit. */
    tls_prepare_provider_fds(stdout_pipe[1], stderr_pipe[1]);

    errno = 0;
    execv(tls_passphrase_provider, stdin_argv);

    /* Since all previous file descriptors (including those for log files)
     * have been closed, and root privs have been revoked, there's little
     * chance of directing a message of execv() failure to proftpd's log
     * files.  execv() only returns if there's an error; the only way we
     * can signal this to the waiting parent process is to exit with a
     * non-zero value (the value of errno will do nicely).
     */

    exit(errno);

  } else {
    int res;
    int maxfd, fds, send_sigterm = 1;
    fd_set readfds;
    time_t start_time = time(NULL);
    struct timeval tv;

    /* Parent process */

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    maxfd = (stderr_pipe[0] > stdout_pipe[0]) ?
      stderr_pipe[0] : stdout_pipe[0];

    res = waitpid(pid, &status, WNOHANG);
    while (res <= 0) {
      if (res < 0) {
        if (errno != EINTR) {
          pr_log_debug(DEBUG2, MOD_TLS_VERSION
            ": passphrase provider error: unable to wait for pid %u: %s",
            (unsigned int) pid, strerror(errno));
          status = -1;
          break;

        } else {
          pr_signals_handle();
        }
      }

      /* Check the time elapsed since we started. */
      if ((time(NULL) - start_time) > TLS_PASSPHRASE_TIMEOUT) {

        /* Send TERM, the first time, to be polite. */
        if (send_sigterm) {
          send_sigterm = 0;
          pr_log_debug(DEBUG6, MOD_TLS_VERSION
            ": '%s' has exceeded the timeout (%lu seconds), sending "
            "SIGTERM (signal %d)", tls_passphrase_provider,
            (unsigned long) TLS_PASSPHRASE_TIMEOUT, SIGTERM);
          kill(pid, SIGTERM);

        } else {
          /* The child is still around?  Terminate with extreme prejudice. */
          pr_log_debug(DEBUG6, MOD_TLS_VERSION
            ": '%s' has exceeded the timeout (%lu seconds), sending "
            "SIGKILL (signal %d)", tls_passphrase_provider,
            (unsigned long) TLS_PASSPHRASE_TIMEOUT, SIGKILL);
          kill(pid, SIGKILL);
        }
      }

      /* Select on the pipe read fds, to see if the child has anything
       * to tell us.
       */
      FD_ZERO(&readfds);

      FD_SET(stdout_pipe[0], &readfds);
      FD_SET(stderr_pipe[0], &readfds);

      /* Note: this delay should be configurable somehow. */
      tv.tv_sec = 2L;
      tv.tv_usec = 0L;

      fds = select(maxfd + 1, &readfds, NULL, NULL, &tv);

      if (fds == -1 &&
          errno == EINTR)
        pr_signals_handle();

      if (fds > 0) {
        /* The child sent us something.  How thoughtful. */

        if (FD_ISSET(stdout_pipe[0], &readfds)) {
          res = read(stdout_pipe[0], buf, buflen);
          if (res > 0) {
              while (res && (buf[res-1] == '\r' || buf[res-1] == '\n'))
                res--;
              buf[res] = '\0';

              pr_trace_msg(trace_channel, 18,
                "read passphrase from '%s'", tls_passphrase_provider);

          } else if (res < 0) {
            int xerrno = errno;

            pr_trace_msg(trace_channel, 3,
              "error reading stdout from '%s': %s",
              tls_passphrase_provider, strerror(xerrno));

            pr_log_debug(DEBUG2, MOD_TLS_VERSION
              ": error reading stdout from '%s': %s",
              tls_passphrase_provider, strerror(xerrno));
          }
        }

        if (FD_ISSET(stderr_pipe[0], &readfds)) {
          int stderrlen;
          char stderrbuf[PIPE_BUF];

          memset(stderrbuf, '\0', sizeof(stderrbuf));
          stderrlen = read(stderr_pipe[0], stderrbuf, sizeof(stderrbuf)-1);
          if (stderrlen > 0) {
            while (stderrlen &&
                   (stderrbuf[stderrlen-1] == '\r' ||
                    stderrbuf[stderrlen-1] == '\n'))
              stderrlen--;
            stderrbuf[stderrlen] = '\0';

            pr_log_debug(DEBUG5, MOD_TLS_VERSION
              ": stderr from '%s': %s", tls_passphrase_provider,
              stderrbuf);

          } else if (res < 0) {
            pr_log_debug(DEBUG2, MOD_TLS_VERSION
              ": error reading stderr from '%s': %s",
              tls_passphrase_provider, strerror(errno));
          }
        }
      }

      res = waitpid(pid, &status, WNOHANG);
    }
  }

  /* Restore the previous signal actions. */
  if (sigaction(SIGINT, &sa_intr, NULL) < 0)
    return -1;

  if (sigaction(SIGQUIT, &sa_quit, NULL) < 0)
    return -1; 

  if (sigprocmask(SIG_SETMASK, &set_save, NULL) < 0)
    return -1;

  if (WIFSIGNALED(status)) {
    pr_log_debug(DEBUG2, MOD_TLS_VERSION
      ": '%s' died from signal %d", tls_passphrase_provider,
      WTERMSIG(status));
    errno = EPERM;
    return -1;
  }

  return 0;
}

static int tls_passphrase_cb(char *buf, int buflen, int rwflag, void *d) {
  static int need_banner = TRUE;
  struct tls_pkey_data *pdata = d;

  if (!tls_passphrase_provider) {
    register unsigned int attempt;
    int pwlen = 0;

    tls_log("requesting passphrase from admin");

    /* Similar to Apache's mod_ssl, we want to be nice, and display an
     * informative message to the proftpd admin, telling them for what
     * server they are being requested to provide a passphrase.  
     */

    if (need_banner) {
      fprintf(stderr, "\nPlease provide passphrases for these encrypted certificate keys:\n");
      need_banner = FALSE;
    }

    /* You get three attempts at entering the passphrase correctly. */
    for (attempt = 0; attempt < 3; attempt++) {
      int res;

      /* Always handle signals in a loop. */
      pr_signals_handle();

      res = EVP_read_pw_string(buf, buflen, pdata->prompt, TRUE);

      /* A return value of zero from EVP_read_pw_string() means success; -1
       * means a system error occurred, and 1 means user interaction problems.
       */
      if (res != 0) {
         fprintf(stderr, "\nPassphrases do not match.  Please try again.\n");
         continue;
      }

      pwlen = strlen(buf);
      if (pwlen < 1) {
        fprintf(stderr, "Error: passphrase must be at least one character\n");

      } else {
        sstrncpy(pdata->buf, buf, pdata->bufsz);
        pdata->buflen = pwlen;

        return pwlen;
      }
    }

  } else {
    tls_log("requesting passphrase from '%s'", tls_passphrase_provider);

    if (tls_exec_passphrase_provider(pdata->s, buf, buflen, pdata->flags) < 0) {
      tls_log("error obtaining passphrase from '%s': %s",
        tls_passphrase_provider, strerror(errno));

    } else {
      sstrncpy(pdata->buf, buf, pdata->bufsz);
      pdata->buflen = strlen(buf);

      return pdata->buflen;
    }
  }

#if OPENSSL_VERSION_NUMBER < 0x00908001
  PEMerr(PEM_F_DEF_CALLBACK, PEM_R_PROBLEMS_GETTING_PASSWORD);
#else
  PEMerr(PEM_F_PEM_DEF_CALLBACK, PEM_R_PROBLEMS_GETTING_PASSWORD);
#endif

  pr_memscrub(buf, buflen);
  return -1;
}

static int prompt_fd = -1;

static void set_prompt_fds(void) {

  /* Reconnect stderr to the term because proftpd connects stderr, earlier,
   * to the general stderr logfile.
   */
  prompt_fd = open("/dev/null", O_WRONLY);
  if (prompt_fd == -1)
    /* This is an arbitrary, meaningless placeholder number. */
    prompt_fd = 76;

  dup2(STDERR_FILENO, prompt_fd);
  dup2(STDOUT_FILENO, STDERR_FILENO);
}

static void restore_prompt_fds(void) {
  dup2(prompt_fd, STDERR_FILENO);
  close(prompt_fd);
  prompt_fd = -1;
}

static int tls_get_pkcs12_passwd(server_rec *s, FILE *fp, const char *prompt,
    char *buf, size_t bufsz, int flags, struct tls_pkey_data *pdata) {
  EVP_PKEY *pkey = NULL;
  X509 *cert = NULL;
  PKCS12 *p12 = NULL;
  char *passwd = NULL;
  int res, ok = FALSE;

  p12 = d2i_PKCS12_fp(fp, NULL);
  if (p12 != NULL) {

    /* Check if a password is needed. */
    res = PKCS12_verify_mac(p12, NULL, 0);
    if (res == 1) {
      passwd = NULL;

    } else if (res == 0) {
      res = PKCS12_verify_mac(p12, "", 0);
      if (res == 1) {
        passwd = "";
      }
    }

    if (res == 0) {
      register unsigned int attempt;

      /* This PKCS12 file is password-protected; need to get the password
       * from the admin.
       */
      for (attempt = 0; attempt < 3; attempt++) {
        int len = -1;

        /* Always handle signals in a loop. */
        pr_signals_handle();

        /* Clear the error queue at the start of the loop. */
        ERR_clear_error();

        len = tls_passphrase_cb(buf, bufsz, 0, pdata);
        if (len > 0) {
          res = PKCS12_verify_mac(p12, buf, -1);
          if (res == 1) {
#if OPENSSL_VERSION_NUMBER >= 0x000905000L
            /* Use the obtained password as additional entropy, ostensibly
             * unknown to attackers who may be watching the network, for
             * OpenSSL's PRNG.
             *
             * Human language gives about 2-3 bits of entropy per byte
             * (as per RFC1750).
             */
            RAND_add(buf, pdata->buflen, pdata->buflen * 0.25);
#endif

            res = PKCS12_parse(p12, buf, &pkey, &cert, NULL);
            if (res != 1) {
              PKCS12_free(p12);
              return -1;
            }

            ok = TRUE;
            break;
          }
        }
 
        fprintf(stderr, "\nWrong password for this PKCS12 file.  Please try again.\n");
      }
    } else {
      res = PKCS12_parse(p12, passwd, &pkey, &cert, NULL);
      if (res != 1) {
        PKCS12_free(p12);
        return -1;
      }

      ok = TRUE;
    }

  } else {
    fprintf(stderr, "\nUnable to read PKCS12 file.\n");
    return -1;
  }

  /* Now we should have an EVP_PKEY (which may or may not need a passphrase),
   * and a cert.  We don't really care about the cert right now.  But we
   * DO need to get the passphrase for the private key.  Do this by writing
   * the key to a BIO, then calling tls_get_passphrase() for that BIO.
   *
   * It looks like OpenSSL's pkcs12 command-line tool does not allow
   * passphrase-protected keys to be written into a PKCS12 structure;
   * the key is decrypted first (hence, probably, the password protection 
   * for the entire PKCS12 structure).  Can the same be assumed to be true
   * for PKCS12 files created via other applications?
   *
   * For now, assume yes, that all PKCS12 files will have private keys which
   * are not encrypted.  If this is found to NOT be the case, then we
   * will need to write the obtained private key out to a BIO somehow,
   * then call tls_get_passphrase() on that BIO, rather than on a path.
   */

  if (cert)
    X509_free(cert);

  if (pkey)
    EVP_PKEY_free(pkey);

  if (p12)
    PKCS12_free(p12);

  if (!ok) {
#if OPENSSL_VERSION_NUMBER < 0x00908001
    PEMerr(PEM_F_DEF_CALLBACK, PEM_R_PROBLEMS_GETTING_PASSWORD);
#else
    PEMerr(PEM_F_PEM_DEF_CALLBACK, PEM_R_PROBLEMS_GETTING_PASSWORD);
#endif
    return -1;
  }

  ERR_clear_error();
  return res;
}

static int tls_get_passphrase(server_rec *s, const char *path,
    const char *prompt, char *buf, size_t bufsz, int flags) {
  FILE *keyf = NULL;
  EVP_PKEY *pkey = NULL;
  struct tls_pkey_data pdata;
  register unsigned int attempt;

  if (path) {
    int fd, res, xerrno;

    /* Open an fp on the cert file. */
    PRIVS_ROOT
    fd = open(path, O_RDONLY);
    xerrno = errno;
    PRIVS_RELINQUISH

    if (fd < 0) {
      SYSerr(SYS_F_FOPEN, xerrno);
      return -1;
    }

    /* Make sure the fd isn't one of the big three. */
    if (fd <= STDERR_FILENO) {
      res = pr_fs_get_usable_fd(fd);
      if (res >= 0) {
        close(fd);
        fd = res;
      }
    }

    keyf = fdopen(fd, "r");
    if (keyf == NULL) {
      xerrno = errno;

      (void) close(fd);
      SYSerr(SYS_F_FOPEN, xerrno);
      return -1;
    }
  }

  pdata.s = s;
  pdata.flags = flags;
  pdata.buf = buf;
  pdata.buflen = 0;
  pdata.bufsz = bufsz;
  pdata.prompt = prompt;

  set_prompt_fds();

  if (flags & TLS_PASSPHRASE_FL_PKCS12_PASSWD) {
    int res;

    res = tls_get_pkcs12_passwd(s, keyf, prompt, buf, bufsz, flags, &pdata);

    if (keyf) {
      fclose(keyf);
      keyf = NULL;
    }

    /* Restore the normal stderr logging. */
    restore_prompt_fds();

    return res;
  }

  /* The user gets three tries to enter the correct passphrase. */
  for (attempt = 0; attempt < 3; attempt++) {

    /* Always handle signals in a loop. */
    pr_signals_handle();

    /* Clear the error queue at the start of the loop. */
    ERR_clear_error();

    pkey = PEM_read_PrivateKey(keyf, NULL, tls_passphrase_cb, &pdata);
    if (pkey)
      break;

    if (keyf) {
      fseek(keyf, 0, SEEK_SET);
    }

    fprintf(stderr, "\nWrong passphrase for this key.  Please try again.\n");
  }

  if (keyf) {
    fclose(keyf);
    keyf = NULL;
  }

  /* Restore the normal stderr logging. */
  restore_prompt_fds();

  if (pkey == NULL)
    return -1;

  EVP_PKEY_free(pkey);

#if OPENSSL_VERSION_NUMBER >= 0x000905000L
  /* Use the obtained passphrase as additional entropy, ostensibly
   * unknown to attackers who may be watching the network, for
   * OpenSSL's PRNG.
   *
   * Human language gives about 2-3 bits of entropy per byte (RFC1750).
   */
  RAND_add(buf, pdata.buflen, pdata.buflen * 0.25);
#endif

#ifdef HAVE_MLOCK
   PRIVS_ROOT
   if (mlock(buf, bufsz) < 0) {
     pr_log_debug(DEBUG1, MOD_TLS_VERSION
       ": error locking passphrase into memory: %s", strerror(errno));

   } else {
     pr_log_debug(DEBUG1, MOD_TLS_VERSION ": passphrase locked into memory");
   }
   PRIVS_RELINQUISH
#endif

  return 0;
}

static int tls_handshake_timeout_cb(CALLBACK_FRAME) {
  tls_handshake_timed_out = TRUE;
  return 0;
}

static tls_pkey_t *tls_lookup_pkey(void) {
  tls_pkey_t *k, *pkey = NULL;

  for (k = tls_pkey_list; k; k = k->next) {

    /* If this pkey matches the current server_rec, mark it and move on. */
    if (k->server == main_server) {

#ifdef HAVE_MLOCK
      /* mlock() the passphrase memory areas again; page locks are not
       * inherited across forks.
       */
      PRIVS_ROOT
      if (k->rsa_pkey) {
        if (mlock(k->rsa_pkey, k->pkeysz) < 0) {
          tls_log("error locking passphrase into memory: %s", strerror(errno));
        }
      }

      if (k->dsa_pkey) {
        if (mlock(k->dsa_pkey, k->pkeysz) < 0) {
          tls_log("error locking passphrase into memory: %s", strerror(errno));
        }
      }

# ifdef PR_USE_OPENSSL_ECC
      if (k->ec_pkey) {
        if (mlock(k->ec_pkey, k->pkeysz) < 0) {
          tls_log("error locking passphrase into memory: %s", strerror(errno));
        }
      }
# endif /* PR_USE_OPENSSL_ECC */

      if (k->pkcs12_passwd) {
        if (mlock(k->pkcs12_passwd, k->pkeysz) < 0) {
          tls_log("error locking password into memory: %s", strerror(errno));
        }
      }
      PRIVS_RELINQUISH
#endif /* HAVE_MLOCK */

      pkey = k;
      continue;
    }

    /* Otherwise, scrub the passphrase's memory areas. */
    if (k->rsa_pkey) {
      pr_memscrub(k->rsa_pkey, k->pkeysz);
      free(k->rsa_pkey_ptr);
      k->rsa_pkey = k->rsa_pkey_ptr = NULL;
    }

    if (k->dsa_pkey) {
      pr_memscrub(k->dsa_pkey, k->pkeysz);
      free(k->dsa_pkey_ptr);
      k->dsa_pkey = k->dsa_pkey_ptr = NULL;
    }

# ifdef PR_USE_OPENSSL_ECC
    if (k->ec_pkey) {
      pr_memscrub(k->ec_pkey, k->pkeysz);
      free(k->ec_pkey_ptr);
      k->ec_pkey = k->ec_pkey_ptr = NULL;
    }
# endif /* PR_USE_OPENSSL_ECC */

    if (k->pkcs12_passwd) {
      pr_memscrub(k->pkcs12_passwd, k->pkeysz);
      free(k->pkcs12_passwd_ptr);
      k->pkcs12_passwd = k->pkcs12_passwd_ptr = NULL;
    }
  }

  return pkey;
}

static int tls_pkey_cb(char *buf, int buflen, int rwflag, void *data) {
  tls_pkey_t *k;

  if (!data)
    return 0;

  k = (tls_pkey_t *) data;

  if ((k->flags & TLS_PKEY_USE_RSA) && k->rsa_pkey) {
    sstrncpy(buf, k->rsa_pkey, buflen);
    buf[buflen - 1] = '\0';
    return strlen(buf);
  }

  if ((k->flags & TLS_PKEY_USE_DSA) && k->dsa_pkey) {
    sstrncpy(buf, k->dsa_pkey, buflen);
    buf[buflen - 1] = '\0';
    return strlen(buf);
  }

#ifdef PR_USE_OPENSSL_ECC
  if ((k->flags & TLS_PKEY_USE_EC) && k->ec_pkey) {
    sstrncpy(buf, k->ec_pkey, buflen);
    buf[buflen - 1] = '\0';
    return strlen(buf);
  }
#endif /* PR_USE_OPENSSL_ECC */

  return 0;
}

static void tls_scrub_pkeys(void) {
  tls_pkey_t *k;

  /* Scrub and free all passphrases in memory. */
  if (tls_pkey_list) {
    pr_log_debug(DEBUG5, MOD_TLS_VERSION
      ": scrubbing %u %s from memory",
      tls_npkeys, tls_npkeys != 1 ? "passphrases" : "passphrase");

  } else {
    return;
  }

  for (k = tls_pkey_list; k; k = k->next) {
    if (k->rsa_pkey) {
      pr_memscrub(k->rsa_pkey, k->pkeysz);
      free(k->rsa_pkey_ptr);
      k->rsa_pkey = k->rsa_pkey_ptr = NULL;
    }

    if (k->dsa_pkey) {
      pr_memscrub(k->dsa_pkey, k->pkeysz);
      free(k->dsa_pkey_ptr);
      k->dsa_pkey = k->dsa_pkey_ptr = NULL;
    }

#ifdef PR_USE_OPENSSL_ECC
    if (k->ec_pkey) {
      pr_memscrub(k->ec_pkey, k->pkeysz);
      free(k->ec_pkey_ptr);
      k->ec_pkey = k->ec_pkey_ptr = NULL;
    }
#endif /* PR_USE_OPENSSL_ECC */

    if (k->pkcs12_passwd) {
      pr_memscrub(k->pkcs12_passwd, k->pkeysz);
      free(k->pkcs12_passwd_ptr);
      k->pkcs12_passwd = k->pkcs12_passwd_ptr = NULL;
    }
  }

  tls_pkey_list = NULL;
  tls_npkeys = 0;
}

#if OPENSSL_VERSION_NUMBER > 0x000907000L
static int tls_renegotiate_timeout_cb(CALLBACK_FRAME) {
  if ((tls_flags & TLS_SESS_ON_CTRL) &&
      (tls_flags & TLS_SESS_CTRL_RENEGOTIATING)) {

    if (!SSL_renegotiate_pending(ctrl_ssl)) {
      tls_log("%s", "control channel TLS session renegotiated");
      tls_flags &= ~TLS_SESS_CTRL_RENEGOTIATING;

    } else if (tls_renegotiate_required) {
      tls_log("%s", "requested TLS renegotiation timed out on control channel");
      tls_log("%s", "shutting down control channel TLS session");
      tls_end_sess(ctrl_ssl, PR_NETIO_STRM_CTRL, 0);
      pr_table_remove(tls_ctrl_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
      pr_table_remove(tls_ctrl_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
      ctrl_ssl = NULL;
    }
  }

  if ((tls_flags & TLS_SESS_ON_DATA) &&
      (tls_flags & TLS_SESS_DATA_RENEGOTIATING)) {
    SSL *ssl;

    ssl = pr_table_get(tls_data_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
    if (!SSL_renegotiate_pending(ssl)) {
      tls_log("%s", "data channel TLS session renegotiated");
      tls_flags &= ~TLS_SESS_DATA_RENEGOTIATING;

    } else if (tls_renegotiate_required) {
      tls_log("%s", "requested TLS renegotiation timed out on data channel");
      tls_log("%s", "shutting down data channel TLS session");
      tls_end_sess(ssl, PR_NETIO_STRM_DATA, 0);
      pr_table_remove(tls_data_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
      pr_table_remove(tls_data_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
    }
  }

  return 0;
}

static int tls_ctrl_renegotiate_cb(CALLBACK_FRAME) {

  /* Guard against a timer firing as the SSL session is being torn down. */
  if (ctrl_ssl == NULL) {
    return 0;
  }

  if (tls_flags & TLS_SESS_ON_CTRL) {

    if (TRUE
#if OPENSSL_VERSION_NUMBER >= 0x009080cfL
        /* In OpenSSL-0.9.8l and later, SSL session renegotiations
         * (both client- and server-initiated) are automatically disabled.
         * Unless the admin explicitly configured support for
         * client-initiated renegotations via the AllowClientRenegotiations
         * TLSOption, we can't request renegotiations ourselves.
         */
        && (tls_opts & TLS_OPT_ALLOW_CLIENT_RENEGOTIATIONS) 
#endif
      ) {
      tls_flags |= TLS_SESS_CTRL_RENEGOTIATING;

      tls_log("requesting TLS renegotiation on control channel "
        "(%lu sec renegotiation interval)", p1);
      SSL_renegotiate(ctrl_ssl);
      /* SSL_do_handshake(ctrl_ssl); */
  
      pr_timer_add(tls_renegotiate_timeout, -1, &tls_module,
        tls_renegotiate_timeout_cb, "SSL/TLS renegotiation");

      /* Restart the timer. */
      return 1;
    }
  }

  return 0;
}
#endif

static DH *tls_dh_cb(SSL *ssl, int is_export, int keylen) {
  DH *dh = NULL;
  EVP_PKEY *pkey;
  int pkeylen = 0, use_pkeylen = FALSE;

  /* OpenSSL will only ever call us (currently) with a keylen of 512 or 1024;
   * see the SSL_EXPORT_PKEYLENGTH macro in ssl_locl.h.  Sigh.
   *
   * Thus we adjust the DH parameter length according to the size of the
   * RSA/DSA private key used for the current connection.
   *
   * NOTE: This MAY cause interoperability issues with some clients, notably
   * Java 7 (and earlier) clients, since Java 7 and earlier supports
   * Diffie-Hellman only up to 1024 bits.  More sighs.  To deal with these
   * clients, then, you need to configure a certificate/key of 1024 bits.
   */
  pkey = SSL_get_privatekey(ssl);
  if (pkey != NULL) {
    if (EVP_PKEY_type(pkey->type) == EVP_PKEY_RSA ||
        EVP_PKEY_type(pkey->type) == EVP_PKEY_DSA) {
      pkeylen = EVP_PKEY_bits(pkey);

      if (pkeylen < TLS_DH_MIN_LEN) {
        if (!(tls_opts & TLS_OPT_ALLOW_WEAK_DH)) {
          pr_trace_msg(trace_channel, 11,
            "certificate private key length %d less than %d bits, using %d "
            "(see AllowWeakDH TLSOption)", pkeylen, TLS_DH_MIN_LEN,
            TLS_DH_MIN_LEN);
          pkeylen = TLS_DH_MIN_LEN;
        }
      }

      if (pkeylen != keylen) {
        pr_trace_msg(trace_channel, 13,
          "adjusted DH parameter length from %d to %d bits", keylen, pkeylen);
        use_pkeylen = TRUE;
      }
    }
  }

  if (tls_tmp_dhs != NULL &&
      tls_tmp_dhs->nelts > 0) {
    register unsigned int i;
    DH *best_dh = NULL, **dhs;
    int best_dhlen = 0;

    dhs = tls_tmp_dhs->elts;

    /* Search the configured list of DH parameters twice: once for any sizes
     * matching the actual requested size (usually 1024), and once for any
     * matching the certificate private key size (pkeylen).
     *
     * This behavior allows site admins to configure a TLSDHParamFile that
     * contains 1024-bit parameters, for e.g. Java 7 (and earlier) clients.
     */

    /* Note: the keylen argument is in BITS, but DH_size() returns the number
     * of BYTES.
     */
    for (i = 0; i < tls_tmp_dhs->nelts; i++) {
      int dhlen;

      dhlen = DH_size(dhs[i]) * 8;
      if (dhlen == keylen) {
        pr_trace_msg(trace_channel, 11,
          "found matching DH parameter for key length %d", keylen);
        return dhs[i];
      }

      /* Try to find the next "best" DH to use, where "best" means
       * the smallest DH that is larger than the necessary keylen.
       */
      if (dhlen > keylen) {
        if (best_dh != NULL) {
          if (dhlen < best_dhlen) {
            best_dh = dhs[i];
            best_dhlen = dhlen;
          }

        } else {
          best_dh = dhs[i];
          best_dhlen = dhlen;
        }
      }
    }

    for (i = 0; i < tls_tmp_dhs->nelts; i++) {
      int dhlen;

      dhlen = DH_size(dhs[i]) * 8;
      if (dhlen == pkeylen) {
        pr_trace_msg(trace_channel, 11,
          "found matching DH parameter for certificate private key length %d",
          pkeylen);
        return dhs[i];
      }

      if (dhlen > pkeylen) {
        if (best_dh != NULL) {
          if (dhlen < best_dhlen) {
            best_dh = dhs[i];
            best_dhlen = dhlen;
          }

        } else {
          best_dh = dhs[i];
          best_dhlen = dhlen;
        }
      }
    }

    if (best_dh != NULL) {
      pr_trace_msg(trace_channel, 11,
        "using best DH parameter for key length %d (length %d)", keylen,
        best_dhlen);
      return best_dh;
    }
  }

  /* Still no DH parameters found?  Use the built-in ones. */

  if (keylen < TLS_DH_MIN_LEN) {
    if (!(tls_opts & TLS_OPT_ALLOW_WEAK_DH)) {
      pr_trace_msg(trace_channel, 11,
        "requested key length %d less than %d bits, using %d "
        "(see AllowWeakDH TLSOption)", keylen, TLS_DH_MIN_LEN, TLS_DH_MIN_LEN);
      keylen = TLS_DH_MIN_LEN;
    }
  }

  if (use_pkeylen) {
    keylen = pkeylen;
  }

  switch (keylen) {
    case 512:
      dh = get_dh512();
      break;

    case 768:
      dh = get_dh768();
      break;

    case 1024:
      dh = get_dh1024();
      break;

    case 1536:
      dh = get_dh1536();
      break;

    case 2048:
      dh = get_dh2048();
      break;

    default:
      tls_log("unsupported DH key length %d requested, returning 1024 bits",
        keylen);
      dh = get_dh1024();
      break;
  }

  pr_trace_msg(trace_channel, 11, "using builtin DH for %d bits", keylen);

  /* Add this DH to the list, so that it can be freed properly later. */
  if (tls_tmp_dhs == NULL) {
    tls_tmp_dhs = make_array(session.pool, 1, sizeof(DH *));
  }

  *((DH **) push_array(tls_tmp_dhs)) = dh;
  return dh;
}

#ifdef PR_USE_OPENSSL_ECC
static EC_KEY *tls_ecdh_cb(SSL *ssl, int is_export, int keylen) {
  static EC_KEY *ecdh = NULL;
  static int init = 0;

  /* XXX Uses 256-bit key for now. TODO: support other sizes. */

  if (init == 0) {
    ecdh = EC_KEY_new();

    if (ecdh != NULL) {
      /* ecdh->group = EC_GROUP_new_by_nid(NID_secp160r2); */
      EC_KEY_set_group(ecdh,
        EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    }

    init = 1;
  }

  return ecdh;
}
#endif /* PR_USE_OPENSSL_ECC */

/* Post 0.9.7a, RSA blinding is turned on by default, so there is no need to
 * do this manually.
 */
#if OPENSSL_VERSION_NUMBER < 0x0090702fL
static void tls_blinding_on(SSL *ssl) {
  EVP_PKEY *pkey = NULL;
  RSA *rsa = NULL;

  /* RSA keys are subject to timing attacks.  To attempt to make such
   * attacks harder, use RSA blinding.
   */

  pkey = SSL_get_privatekey(ssl);

  if (pkey)
    rsa = EVP_PKEY_get1_RSA(pkey);

  if (rsa) {
    if (RSA_blinding_on(rsa, NULL) != 1) {
      tls_log("error setting RSA blinding: %s",
        ERR_error_string(ERR_get_error(), NULL));

    } else {
      tls_log("set RSA blinding on");
    }

    /* Now, "free" the RSA pointer, to properly decrement the reference
     * counter.
     */
    RSA_free(rsa);

  } else {

    /* The administrator may have configured DSA keys rather than RSA keys.
     * In this case, there is nothing to do.
     */
  }

  return;
}
#endif

static int tls_init_ctx(void) {
  config_rec *c;
  int ssl_opts = tls_ssl_opts;
  long ssl_mode = 0;

  if (pr_define_exists("TLS_USE_FIPS") &&
      ServerType == SERVER_INETD) {
#ifdef OPENSSL_FIPS
    if (!FIPS_mode()) {
      /* Make sure OpenSSL is set to use the default RNG, as per an email
       * discussion on the OpenSSL developer list:
       *
       *  "The internal FIPS logic uses the default RNG to see the FIPS RNG
       *   as part of the self test process..."
       */
      RAND_set_rand_method(NULL);

      if (!FIPS_mode_set(1)) {
        const char *errstr;

        errstr = tls_get_errors();
        tls_log("unable to use FIPS mode: %s", errstr);
        pr_log_pri(PR_LOG_ERR, MOD_TLS_VERSION
          ": unable to use FIPS mode: %s", errstr);

        errno = EPERM;
        return -1;

      } else {
        pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION ": FIPS mode enabled");
      }

    } else {
      pr_log_pri(PR_LOG_DEBUG, MOD_TLS_VERSION ": FIPS mode already enabled");
    }
#else
    pr_log_pri(PR_LOG_WARNING, MOD_TLS_VERSION ": FIPS mode requested, but " OPENSSL_VERSION_TEXT " not built with FIPS support");
#endif /* OPENSSL_FIPS */
  }

  if (ssl_ctx != NULL) {
    SSL_CTX_free(ssl_ctx);
    ssl_ctx = NULL;
  }

  ssl_ctx = SSL_CTX_new(SSLv23_server_method());
  if (ssl_ctx == NULL) {
    pr_log_debug(DEBUG0, MOD_TLS_VERSION ": error: SSL_CTX_new(): %s",
      tls_get_errors());
    return -1;
  }

#if OPENSSL_VERSION_NUMBER > 0x000906000L
  /* The SSL_MODE_AUTO_RETRY mode was added in 0.9.6. */
  ssl_mode |= SSL_MODE_AUTO_RETRY;
#endif

#if OPENSSL_VERSION_NUMBER >= 0x1000001fL
  /* The SSL_MODE_RELEASE_BUFFERS mode was added in 1.0.0a. */
  ssl_mode |= SSL_MODE_RELEASE_BUFFERS;
#endif

  if (ssl_mode != 0) {
    SSL_CTX_set_mode(ssl_ctx, ssl_mode);
  }

  /* If using OpenSSL-0.9.7 or greater, prevent session resumptions on
   * renegotiations (more secure).
   */
#if OPENSSL_VERSION_NUMBER > 0x000907000L
  ssl_opts |= SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
#endif

  /* Disable SSL tickets, for now. */
#ifdef SSL_OP_NO_TICKET
  ssl_opts |= SSL_OP_NO_TICKET;
#endif

  /* Disable SSL compression. */
#ifdef SSL_OP_NO_COMPRESSION
  ssl_opts |= SSL_OP_NO_COMPRESSION;
#endif /* SSL_OP_NO_COMPRESSION */

#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
  c = find_config(main_server->conf, CONF_PARAM, "TLSServerCipherPreference",
    FALSE);
  if (c != NULL) {
    int use_server_pref;

    use_server_pref = *((int *) c->argv[0]);
    if (use_server_pref == TRUE) {
      ssl_opts |= SSL_OP_CIPHER_SERVER_PREFERENCE;
    }
  }
#endif /* SSL_OP_CIPHER_SERVER_PREFERENCE */

  SSL_CTX_set_options(ssl_ctx, ssl_opts);

  /* Set up session caching. */
  c = find_config(main_server->conf, CONF_PARAM, "TLSSessionCache", FALSE);
  if (c != NULL) {
    const char *provider;
    long timeout;

    /* Look up and initialize the configured session cache provider. */
    provider = c->argv[0];
    timeout = *((long *) c->argv[2]);

    if (provider != NULL) {
      if (strncmp(provider, "internal", 9) != 0) {
        tls_sess_cache = tls_sess_cache_get_cache(provider);

        pr_log_debug(DEBUG8, MOD_TLS_VERSION ": opening '%s' TLSSessionCache",
          provider);

        if (tls_sess_cache_open(c->argv[1], timeout) == 0) {
          long cache_mode, cache_flags;

          cache_mode = SSL_SESS_CACHE_SERVER;

          /* We could force OpenSSL to use ONLY the configured external session
           * caching mechanism by using the SSL_SESS_CACHE_NO_INTERNAL mode flag
           * (available in OpenSSL 0.9.6h and later).
           *
           * However, consider the case where the serialized session data is
           * too large for the external cache, or the external cache refuses
           * to add the session for some reason.  If OpenSSL is using only our
           * external cache, that session is lost (which could cause problems
           * e.g. for later protected data transfers, which require that the
           * SSL session from the control connection be reused).
           *
           * If the external cache can be reasonably sure that session data
           * can be added, then the NO_INTERNAL flag is a good idea; it keeps
           * OpenSSL from allocating more memory than necessary.  Having both
           * an internal and an external cache of the same data is a bit
           * unresourceful.  Thus we ask the external cache mechanism what
           * additional cache mode flags to use.
           */

          cache_flags = tls_sess_cache_get_cache_mode();
          cache_mode |= cache_flags;

          SSL_CTX_set_session_cache_mode(ssl_ctx, cache_mode);
          SSL_CTX_set_timeout(ssl_ctx, timeout);

          SSL_CTX_sess_set_new_cb(ssl_ctx, tls_sess_cache_add_sess_cb);
          SSL_CTX_sess_set_get_cb(ssl_ctx, tls_sess_cache_get_sess_cb);
          SSL_CTX_sess_set_remove_cb(ssl_ctx, tls_sess_cache_delete_sess_cb);

        } else {
          pr_log_debug(DEBUG1, MOD_TLS_VERSION
            ": error opening '%s' TLSSessionCache: %s", provider,
            strerror(errno));

          /* Default to using OpenSSL's own internal session caching. */
          SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_SERVER);
        }

      } else {
        /* Default to using OpenSSL's own internal session caching. */
        SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_SERVER);
        SSL_CTX_set_timeout(ssl_ctx, timeout);
      }

    } else {
      /* SSL session caching has been explicitly turned off. */

      pr_log_debug(DEBUG3, MOD_TLS_VERSION
        ": TLSSessionCache off, disabling SSL session caching and setting "
        "NoSessionReuseRequired TLSOption");

      SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_OFF);

      /* Make sure we automatically relax the "SSL session reuse required
       * for data connections" requirement; to enforce such a requirement
       * SSL session caching MUST be enabled.  So if session caching has been
       * explicitly disabled, relax that policy as well.
       */
      tls_opts |= TLS_OPT_NO_SESSION_REUSE_REQUIRED;
    }

  } else {
    long timeout = 0;

#if OPENSSL_VERSION_NUMBER > 0x000907000L
    /* If we support renegotations, then make the cache lifetime be 10%
     * longer than the control connection renegotiation timer.
     */
    timeout = 15840;

#else
    /* If we are not supporting reneogtiations because the OpenSSL version
     * is too old, then set the default cache lifetime to 30 minutes.
     */
    timeout = 1800;
#endif /* OpenSSL older than 0.9.7. */

    /* Make sure that we enable OpenSSL internal caching, AND that the
     * cache timeout is longer than the default control channel
     * renegotiation.
     */

    SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_SERVER);
    SSL_CTX_set_timeout(ssl_ctx, timeout);
  }

  SSL_CTX_set_tmp_dh_callback(ssl_ctx, tls_dh_cb);
#ifdef PR_USE_OPENSSL_ECC
  SSL_CTX_set_tmp_ecdh_callback(ssl_ctx, tls_ecdh_cb);
#endif /* PR_USE_OPENSSL_ECC */

  if (tls_seed_prng() < 0) {
    pr_log_debug(DEBUG1, MOD_TLS_VERSION ": unable to properly seed PRNG");
  }

  return 0;
}

static const char *tls_get_proto_str(pool *p, unsigned int protos,
    unsigned int *count) {
  char *proto_str = "";
  unsigned int nproto = 0;

  if (protos & TLS_PROTO_SSL_V3) {
    proto_str = pstrcat(p, proto_str, *proto_str ? ", " : "",
      "SSLv3", NULL);
    nproto++;
  }

  if (protos & TLS_PROTO_TLS_V1) {
    proto_str = pstrcat(p, proto_str, *proto_str ? ", " : "",
      "TLSv1", NULL);
    nproto++;
  }

  if (protos & TLS_PROTO_TLS_V1_1) {
    proto_str = pstrcat(p, proto_str, *proto_str ? ", " : "",
      "TLSv1.1", NULL);
    nproto++;
  }

  if (protos & TLS_PROTO_TLS_V1_2) {
    proto_str = pstrcat(p, proto_str, *proto_str ? ", " : "",
      "TLSv1.2", NULL);
    nproto++;
  }

  *count = nproto;
  return proto_str;
}

/* Construct the options value that disables all unsupported protocols.  
 */
static int get_disabled_protocols(unsigned int supported_protocols) {
  int disabled_protocols;

  /* First, create an options value where ALL protocols are disabled. */
  disabled_protocols = (SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1);

#ifdef SSL_OP_NO_TLSv1_1
  disabled_protocols |= SSL_OP_NO_TLSv1_1;
#endif
#ifdef SSL_OP_NO_TLSv1_2
  disabled_protocols |= SSL_OP_NO_TLSv1_2;
#endif

  /* Now, based on the given bitset of supported protocols, clear the
   * necessary bits.
   */

  if (supported_protocols & TLS_PROTO_SSL_V3) {
    disabled_protocols &= ~SSL_OP_NO_SSLv3;
  }

  if (supported_protocols & TLS_PROTO_TLS_V1) {
    disabled_protocols &= ~SSL_OP_NO_TLSv1;
  }

#if OPENSSL_VERSION_NUMBER >= 0x10001000L
  if (supported_protocols & TLS_PROTO_TLS_V1_1) {
    disabled_protocols &= ~SSL_OP_NO_TLSv1_1;
  }

  if (supported_protocols & TLS_PROTO_TLS_V1_2) {
    disabled_protocols &= ~SSL_OP_NO_TLSv1_2;
  }
#endif /* OpenSSL-1.0.1 or later */

  return disabled_protocols;
}

static int tls_init_server(void) {
  config_rec *c = NULL;
  char *tls_ca_cert = NULL, *tls_ca_path = NULL, *tls_ca_chain = NULL;
  X509 *server_ec_cert = NULL, *server_dsa_cert = NULL, *server_rsa_cert = NULL;
  int verify_mode = SSL_VERIFY_PEER;
  unsigned int enabled_proto_count = 0, tls_protocol = TLS_PROTO_DEFAULT;
  int disabled_proto;
  const char *enabled_proto_str = NULL;

  c = find_config(main_server->conf, CONF_PARAM, "TLSProtocol", FALSE);
  if (c != NULL) {
    tls_protocol = *((unsigned int *) c->argv[0]);
  }

  disabled_proto = get_disabled_protocols(tls_protocol);

  /* Per the comments in <ssl/ssl.h>, SSL_CTX_set_options() uses |= on
   * the previous value.  This means we can easily OR in our new option
   * values with any previously set values.
   */
  enabled_proto_str = tls_get_proto_str(main_server->pool, tls_protocol,
    &enabled_proto_count);

  pr_log_debug(DEBUG8, MOD_TLS_VERSION ": supporting %s %s",
    enabled_proto_str,
    enabled_proto_count != 1 ? "protocols" : "protocol only");
  SSL_CTX_set_options(ssl_ctx, disabled_proto);

  tls_ca_cert = get_param_ptr(main_server->conf, "TLSCACertificateFile", FALSE);
  tls_ca_path = get_param_ptr(main_server->conf, "TLSCACertificatePath", FALSE);

  if (tls_ca_cert || tls_ca_path) {

    /* Set the locations used for verifying certificates. */
    PRIVS_ROOT
    if (SSL_CTX_load_verify_locations(ssl_ctx, tls_ca_cert, tls_ca_path) != 1) {
      PRIVS_RELINQUISH
      tls_log("unable to set CA verification using file '%s' or "
        "directory '%s': %s", tls_ca_cert ? tls_ca_cert : "(none)",
        tls_ca_path ? tls_ca_path : "(none)",
        ERR_error_string(ERR_get_error(), NULL));
      return -1;
    }
    PRIVS_RELINQUISH

  } else {
    /* Default to using locations set in the OpenSSL config file. */
    pr_trace_msg(trace_channel, 9,
      "using default OpenSSL verification locations (see $SSL_CERT_DIR "
      "environment variable)");

    if (SSL_CTX_set_default_verify_paths(ssl_ctx) != 1) {
      tls_log("error setting default verification locations: %s",
        tls_get_errors());
    }
  }

  if (tls_flags & TLS_SESS_VERIFY_CLIENT) {
    /* If we are verifying clients, make sure the client sends a cert;
     * the protocol allows for the client to disregard a request for
     * its cert by the server.
     */
    verify_mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;

    if (tls_opts & TLS_OPT_NO_CERT_REQUEST) {
      /* Warn about the incompatibility of using "TLSVerifyClient on" and
       * "TLSOption NoCertRequest" at the same time.
       */
      tls_log("TLSVerifyClient in effect, ignoring NoCertRequest TLSOption");
    }

  } else if (tls_opts & TLS_OPT_NO_CERT_REQUEST) {
    verify_mode = 0;
  }

  if (verify_mode != 0) {
    SSL_CTX_set_verify(ssl_ctx, verify_mode, tls_verify_cb);

    /* Note: we add one to the configured depth purposefully.  As noted
     * in the OpenSSL man pages, the verification process will silently
     * stop at the configured depth, and the error messages ensuing will
     * be that of an incomplete certificate chain, rather than the
     * "chain too long" error that might be expected.  To log the "chain
     * too long" condition, we add one to the configured depth, and catch,
     * in the verify callback, the exceeding of the actual depth.
     */
    SSL_CTX_set_verify_depth(ssl_ctx, tls_verify_depth + 1);
  }

  if (tls_ca_cert) {
    STACK_OF(X509_NAME) *sk;

    /* Use SSL_load_client_CA_file() to load all of the CA certs (since
     * there can be more than one) from the TLSCACertificateFile.  The
     * entire list of CAs in that file will be present to the client as
     * the "acceptable client CA" list, assuming that
     * TLSOptions NoCertRequest" is not in use.
     */

    PRIVS_ROOT
    sk = SSL_load_client_CA_file(tls_ca_cert);
    PRIVS_RELINQUISH

    if (sk) {
      SSL_CTX_set_client_CA_list(ssl_ctx, sk);

    } else {
      tls_log("unable to read certificates in '%s': %s", tls_ca_cert,
        tls_get_errors());
    }

    if (tls_ca_path) {
      DIR *cacertdir = NULL;

      PRIVS_ROOT
      cacertdir = opendir(tls_ca_path);
      PRIVS_RELINQUISH

      if (cacertdir) {
        struct dirent *cadent = NULL;
        pool *tmp_pool = make_sub_pool(permanent_pool);

        while ((cadent = readdir(cacertdir)) != NULL) {
          FILE *cacertf;
          char *cacertname;

          pr_signals_handle();

          /* Skip dot directories. */
          if (is_dotdir(cadent->d_name)) {
            continue;
          }

          cacertname = pdircat(tmp_pool, tls_ca_path, cadent->d_name, NULL);

          PRIVS_ROOT
          cacertf = fopen(cacertname, "r");
          PRIVS_RELINQUISH

          if (cacertf) {
            X509 *x509 = PEM_read_X509(cacertf, NULL, NULL, NULL);

            if (x509) {
              if (SSL_CTX_add_client_CA(ssl_ctx, x509) != 1) {
                tls_log("error adding '%s' to client CA list: %s", cacertname,
                  tls_get_errors());
              }

            } else {
              tls_log("unable to read '%s': %s", cacertname, tls_get_errors());
            }

            fclose(cacertf);

          } else {
            tls_log("unable to open '%s': %s", cacertname, strerror(errno));
          }
        }

        destroy_pool(tmp_pool);
        closedir(cacertdir);
 
      } else {
        tls_log("unable to add CAs in '%s': %s", tls_ca_path,
          strerror(errno));
      }
    }
  }

  /* Assume that, if no separate key files are configured, the keys are
   * in the same file as the corresponding certificate.
   */
  if (tls_rsa_key_file == NULL) {
    tls_rsa_key_file = tls_rsa_cert_file;
  }

  if (tls_dsa_key_file == NULL) {
    tls_dsa_key_file = tls_dsa_cert_file;
  }

  if (tls_ec_key_file == NULL) {
    tls_ec_key_file = tls_ec_cert_file;
  }

  PRIVS_ROOT
  if (tls_rsa_cert_file != NULL) {
    FILE *fh = NULL;
    int res, xerrno;
    X509 *cert = NULL;

    fh = fopen(tls_rsa_cert_file, "r");
    xerrno = errno;
    if (fh == NULL) {
      PRIVS_RELINQUISH
      tls_log("error reading TLSRSACertificateFile '%s': %s", tls_rsa_cert_file,
        strerror(xerrno));
      errno = xerrno;
      return -1;
    }

    cert = PEM_read_X509(fh, NULL, ssl_ctx->default_passwd_callback,
      ssl_ctx->default_passwd_callback_userdata);
    if (cert == NULL) {
      PRIVS_RELINQUISH
      tls_log("error reading TLSRSACertificateFile '%s': %s", tls_rsa_cert_file,
        tls_get_errors());
      fclose(fh);
      return -1;
    }

    fclose(fh);

    /* SSL_CTX_use_certificate() will increment the refcount on cert, so we
     * can safely call X509_free() on it.  However, we need to keep that
     * pointer around until after the handling of a cert chain file.
     */
    res = SSL_CTX_use_certificate(ssl_ctx, cert);
    if (res <= 0) {
      PRIVS_RELINQUISH

      tls_log("error loading TLSRSACertificateFile '%s': %s", tls_rsa_cert_file,
        tls_get_errors());
      return -1;
    }

    SSL_CTX_set_tmp_rsa_callback(ssl_ctx, tls_rsa_cb);
    server_rsa_cert = cert;
  }

  if (tls_rsa_key_file != NULL) {
    int res;

    if (tls_pkey) {
      tls_pkey->flags |= TLS_PKEY_USE_RSA;
      tls_pkey->flags &= ~(TLS_PKEY_USE_DSA|TLS_PKEY_USE_EC);
    }

    res = SSL_CTX_use_PrivateKey_file(ssl_ctx, tls_rsa_key_file,
      X509_FILETYPE_PEM);

    if (res <= 0) {
      PRIVS_RELINQUISH

      tls_log("error loading TLSRSACertificateKeyFile '%s': %s",
        tls_rsa_key_file, tls_get_errors());
      return -1;
    }

    res = SSL_CTX_check_private_key(ssl_ctx);
    if (res != 1) {
      PRIVS_RELINQUISH 

      tls_log("error checking key from TLSRSACertificateKeyFile '%s': %s",
        tls_rsa_key_file, tls_get_errors());
      return -1;
    }
  }

  if (tls_dsa_cert_file != NULL) {
    FILE *fh = NULL;
    int res, xerrno;
    X509 *cert = NULL;

    fh = fopen(tls_dsa_cert_file, "r");
    xerrno = errno;
    if (fh == NULL) {
      PRIVS_RELINQUISH
      tls_log("error reading TLSDSACertificateFile '%s': %s", tls_dsa_cert_file,
        strerror(xerrno));
      errno = xerrno;
      return -1;
    }

    cert = PEM_read_X509(fh, NULL, ssl_ctx->default_passwd_callback,
      ssl_ctx->default_passwd_callback_userdata);
    if (cert == NULL) {
      PRIVS_RELINQUISH
      tls_log("error reading TLSDSACertificateFile '%s': %s", tls_dsa_cert_file,
        tls_get_errors());
      fclose(fh);
      return -1;
    }

    fclose(fh);

    /* SSL_CTX_use_certificate() will increment the refcount on cert, so we
     * can safely call X509_free() on it.  However, we need to keep that
     * pointer around until after the handling of a cert chain file.
     */
    res = SSL_CTX_use_certificate(ssl_ctx, cert);
    if (res <= 0) {
      PRIVS_RELINQUISH

      tls_log("error loading TLSDSACertificateFile '%s': %s", tls_dsa_cert_file,
        tls_get_errors());
      return -1;
    }

    server_dsa_cert = cert;
  }

  if (tls_dsa_key_file != NULL) {
    int res;

    if (tls_pkey) {
      tls_pkey->flags |= TLS_PKEY_USE_DSA;
      tls_pkey->flags &= ~(TLS_PKEY_USE_RSA|TLS_PKEY_USE_EC);
    }

    res = SSL_CTX_use_PrivateKey_file(ssl_ctx, tls_dsa_key_file,
      X509_FILETYPE_PEM);

    if (res <= 0) {
      PRIVS_RELINQUISH

      tls_log("error loading TLSDSACertificateKeyFile '%s': %s",
        tls_dsa_key_file, tls_get_errors());
      return -1;
    }

    res = SSL_CTX_check_private_key(ssl_ctx);
    if (res != 1) {
      PRIVS_RELINQUISH

      tls_log("error checking key from TLSDSACertificateKeyFile '%s': %s",
        tls_dsa_key_file, tls_get_errors());
      return -1;
    }
  }

#ifdef PR_USE_OPENSSL_ECC
  if (tls_ec_cert_file != NULL) {
    FILE *fh = NULL;
    int res, xerrno;
    X509 *cert = NULL;

    fh = fopen(tls_ec_cert_file, "r");
    xerrno = errno;
    if (fh == NULL) {
      PRIVS_RELINQUISH
      tls_log("error reading TLSECCertificateFile '%s': %s", tls_ec_cert_file,
        strerror(xerrno));
      errno = xerrno;
      return -1;
    }

    cert = PEM_read_X509(fh, NULL, ssl_ctx->default_passwd_callback,
      ssl_ctx->default_passwd_callback_userdata);
    if (cert == NULL) {
      PRIVS_RELINQUISH
      tls_log("error reading TLSECCertificateFile '%s': %s", tls_ec_cert_file,
        tls_get_errors());
      fclose(fh);
      return -1;
    }

    fclose(fh);

    /* SSL_CTX_use_certificate() will increment the refcount on cert, so we
     * can safely call X509_free() on it.  However, we need to keep that
     * pointer around until after the handling of a cert chain file.
     */
    res = SSL_CTX_use_certificate(ssl_ctx, cert);
    if (res <= 0) {
      PRIVS_RELINQUISH

      tls_log("error loading TLSECCertificateFile '%s': %s", tls_ec_cert_file,
        tls_get_errors());
      return -1;
    }

    server_ec_cert = cert;
  }

  if (tls_ec_key_file != NULL) {
    int res;

    if (tls_pkey) {
      tls_pkey->flags |= TLS_PKEY_USE_EC;
      tls_pkey->flags &= ~(TLS_PKEY_USE_RSA|TLS_PKEY_USE_DSA);
    }

    res = SSL_CTX_use_PrivateKey_file(ssl_ctx, tls_ec_key_file,
      X509_FILETYPE_PEM);

    if (res <= 0) {
      PRIVS_RELINQUISH

      tls_log("error loading TLSECCertificateKeyFile '%s': %s",
        tls_ec_key_file, tls_get_errors());
      return -1;
    }

    res = SSL_CTX_check_private_key(ssl_ctx);
    if (res != 1) {
      PRIVS_RELINQUISH

      tls_log("error checking key from TLSECCertificateKeyFile '%s': %s",
        tls_ec_key_file, tls_get_errors());
      return -1;
    }
  }
#endif /* PR_USE_OPENSSL_ECC */

  if (tls_pkcs12_file != NULL) {
    int res;
    FILE *fp;
    X509 *cert = NULL;
    EVP_PKEY *pkey = NULL;
    PKCS12 *p12 = NULL;
    char *passwd = "";

    if (tls_pkey) {
      passwd = tls_pkey->pkcs12_passwd;
    }

    fp = fopen(tls_pkcs12_file, "r");
    if (fp == NULL) {
      int xerrno = errno;

      PRIVS_RELINQUISH
      tls_log("error opening TLSPKCS12File '%s': %s", tls_pkcs12_file,
        strerror(xerrno));
      return -1;
    }

    /* Note that this should NOT fail; we will have already parsed the
     * PKCS12 file already, in order to get the password and key passphrases.
     */
    p12 = d2i_PKCS12_fp(fp, NULL);
    if (p12 == NULL) {
      tls_log("error reading TLSPKCS12File '%s': %s", tls_pkcs12_file,
        tls_get_errors()); 
      fclose(fp);
      return -1;
    }

    fclose(fp);

    /* XXX Need to add support for any CA certs contained in the PKCS12 file. */
    if (PKCS12_parse(p12, passwd, &pkey, &cert, NULL) != 1) {
      tls_log("error parsing info in TLSPKCS12File '%s': %s", tls_pkcs12_file,
        tls_get_errors());

      PKCS12_free(p12);

      if (cert)
        X509_free(cert);

      if (pkey)
        EVP_PKEY_free(pkey);

      return -1;
    }

    /* Note: You MIGHT be tempted to call SSL_CTX_check_private_key(ssl_ctx)
     * here, to make sure that the private key can be used.  But doing so
     * will not work as expected, and will error out with:
     *
     *  SSL_CTX_check_private_key:no certificate assigned
     *
     * To make PKCS#12 support work, do not make that call here.
     */

    res = SSL_CTX_use_certificate(ssl_ctx, cert);
    if (res <= 0) {
      PRIVS_RELINQUISH

      tls_log("error loading certificate from TLSPKCS12File '%s' %s",
        tls_pkcs12_file, tls_get_errors());
      PKCS12_free(p12);

      if (cert)
        X509_free(cert);

      if (pkey)
        EVP_PKEY_free(pkey);

      return -1;
    }

    if (pkey &&
        tls_pkey) {
      switch (EVP_PKEY_type(pkey->type)) {
        case EVP_PKEY_RSA:
          tls_pkey->flags |= TLS_PKEY_USE_RSA;
          tls_pkey->flags &= ~(TLS_PKEY_USE_DSA|TLS_PKEY_USE_EC);
          break;

        case EVP_PKEY_DSA:
          tls_pkey->flags |= TLS_PKEY_USE_DSA;
          tls_pkey->flags &= ~(TLS_PKEY_USE_RSA|TLS_PKEY_USE_EC);
          break;

#ifdef PR_USE_OPENSSL_ECC
        case EVP_PKEY_EC:
          tls_pkey->flags |= TLS_PKEY_USE_EC;
          tls_pkey->flags &= ~(TLS_PKEY_USE_RSA|TLS_PKEY_USE_DSA);
          break;
#endif /* PR_USE_OPENSSL_ECC */
      }
    }

    res = SSL_CTX_use_PrivateKey(ssl_ctx, pkey);
    if (res <= 0) {
      PRIVS_RELINQUISH

      tls_log("error loading key from TLSPKCS12File '%s' %s", tls_pkcs12_file,
        tls_get_errors());

      PKCS12_free(p12);

      if (cert)
        X509_free(cert);

      if (pkey)
        EVP_PKEY_free(pkey);

      return -1;
    }

    res = SSL_CTX_check_private_key(ssl_ctx);
    if (res != 1) {
      PRIVS_RELINQUISH

      tls_log("error checking key from TLSPKCS12File '%s': %s", tls_pkcs12_file,
        tls_get_errors());

      PKCS12_free(p12);

      if (cert)
        X509_free(cert);

      if (pkey)
        EVP_PKEY_free(pkey);

      return -1;
    }

    /* SSL_CTX_use_certificate() will increment the refcount on cert, so we
     * can safely call X509_free() on it.  However, we need to keep that
     * pointer around until after the handling of a cert chain file.
     */
    if (pkey != NULL) {
      switch (EVP_PKEY_type(pkey->type)) {
        case EVP_PKEY_RSA:
          server_rsa_cert = cert;
          break;

        case EVP_PKEY_DSA:
          server_dsa_cert = cert;
          break;

#ifdef PR_USE_OPENSSL_ECC
        case EVP_PKEY_EC:
          server_ec_cert = cert;
          break;
#endif /* PR_USE_OPENSSL_ECC */
      }
    }

    if (pkey)
      EVP_PKEY_free(pkey);

    if (p12)
      PKCS12_free(p12);
  }

  /* Log a warning if the server was badly misconfigured, and has no server
   * certs at all.  The client will probably see this situation as something
   * like:
   *
   *  error:14094410:SSL routines:SSL3_READ_BYTES:sslv3 alert handshake failure
   *
   * And the TLSLog will show the error as:
   *
   *  error:1408A0C1:SSL routines:SSL3_GET_CLIENT_HELLO:no shared cipher
   */
  if (tls_rsa_cert_file == NULL &&
      tls_dsa_cert_file == NULL &&
      tls_ec_cert_file == NULL &&
      tls_pkcs12_file == NULL) {
    tls_log("no TLSRSACertificateFile, TLSDSACertificateFile, "
      "TLSECCertificateFile, or TLSPKCS12File configured; "
      "unable to handle SSL/TLS connections");
    pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION
      ": no TLSRSACertificateFile, TLSDSACertificateFile, "
      "TLSECCertificateFile or TLSPKCS12File configured; "
      "unable to handle SSL/TLS connections");
  }

  /* Handle a CertificateChainFile.  We need to do this here, after the
   * server cert has been loaded, so that we can decide whether the
   * CertificateChainFile contains another copy of the server cert (or not).
   */

  tls_ca_chain = get_param_ptr(main_server->conf, "TLSCertificateChainFile",
    FALSE);
  if (tls_ca_chain) {
    BIO *bio;
    X509 *cert;

    /* Ideally we would use OpenSSL's SSL_CTX_use_certificate_chain()
     * function.  However, that function automatically assumes that the
     * first cert contained in the chain file is to be used as the server
     * cert.  This may or may not be the case.  So instead, we read through
     * the chain and add the extra certs ourselves.
     */

    bio = BIO_new_file(tls_ca_chain, "r");
    if (bio) {
      unsigned int count = 0;
      int res;

      cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
      while (cert != NULL) {
        pr_signals_handle();

        if (server_rsa_cert != NULL) {
          /* Skip this cert if it is the same as the configured RSA
           * server cert.
           */
          if (X509_cmp(server_rsa_cert, cert) == 0) {
            cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
            continue;
          }
        }

        if (server_dsa_cert != NULL) {
          /* Skip this cert if it is the same as the configured RSA
           * server cert.
           */
          if (X509_cmp(server_dsa_cert, cert) == 0) {
            cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
            continue;
          }
        }

        res = SSL_CTX_add_extra_chain_cert(ssl_ctx, cert);
        if (res != 1) {
          tls_log("error adding cert to certificate chain: %s",
            tls_get_errors());
          X509_free(cert);
          break;
        }

        count++;
        cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
      }

      BIO_free(bio);

      tls_log("added %u certs from '%s' to certificate chain", count,
        tls_ca_chain);

    } else {
      tls_log("unable to read certificate chain '%s': %s", tls_ca_chain,
        tls_get_errors());
    }
  } 

  /* Done with the server cert pointers now. */
  if (server_rsa_cert != NULL) {
    X509_free(server_rsa_cert);
    server_rsa_cert = NULL;
  }

  if (server_dsa_cert != NULL) {
    X509_free(server_dsa_cert);
    server_dsa_cert = NULL;
  }

#ifdef PR_USE_OPENSSL_ECC
  if (server_ec_cert != NULL) {
    X509_free(server_ec_cert);
    server_ec_cert = NULL;
  }
#endif /* PR_USE_OPENSSL_ECC */

  /* Set up the CRL. */
  if (tls_crl_file || tls_crl_path) {
    tls_crl_store = X509_STORE_new();
    if (tls_crl_store == NULL) {
      tls_log("error creating CRL store: %s", tls_get_errors());
      return -1;
    }

    if (X509_STORE_load_locations(tls_crl_store, tls_crl_file,
        tls_crl_path) == 0) {

      if (tls_crl_file && !tls_crl_path) {
        tls_log("error loading TLSCARevocationFile '%s': %s",
          tls_crl_file, tls_get_errors());

      } else if (!tls_crl_file && tls_crl_path) {
        tls_log("error loading TLSCARevocationPath '%s': %s",
          tls_crl_path, tls_get_errors());

      } else {
        tls_log("error loading TLSCARevocationFile '%s', "
          "TLSCARevocationPath '%s': %s", tls_crl_file, tls_crl_path,
          tls_get_errors());
      }
    }
  }

  PRIVS_RELINQUISH

  SSL_CTX_set_cipher_list(ssl_ctx, tls_cipher_suite);

#if OPENSSL_VERSION_NUMBER > 0x000907000L
  /* Lookup/process any configured TLSRenegotiate parameters. */
  c = find_config(main_server->conf, CONF_PARAM, "TLSRenegotiate", FALSE);
  if (c) {
    if (c->argc == 0) {
      /* Disable all server-side requested renegotiations; clients can
       * still request renegotiations.
       */
      tls_ctrl_renegotiate_timeout = 0;
      tls_data_renegotiate_limit = 0;
      tls_renegotiate_timeout = 0;
      tls_renegotiate_required = FALSE;

    } else {
      int ctrl_timeout = *((int *) c->argv[0]);
      off_t data_limit = *((off_t *) c->argv[1]);
      int renegotiate_timeout = *((int *) c->argv[2]);
      unsigned char renegotiate_required = *((unsigned char *) c->argv[3]);

      if (data_limit)
        tls_data_renegotiate_limit = data_limit;
    
      if (renegotiate_timeout)
        tls_renegotiate_timeout = renegotiate_timeout;

      tls_renegotiate_required = renegotiate_required;
  
      /* Set any control channel renegotiation timers, if need be. */
      pr_timer_add(ctrl_timeout ? ctrl_timeout : tls_ctrl_renegotiate_timeout,
        -1, &tls_module, tls_ctrl_renegotiate_cb, "SSL/TLS renegotiation");
    }
  }
#endif

  return 0;
}

static int tls_get_block(conn_t *conn) {
  int flags;

  flags = fcntl(conn->rfd, F_GETFL);
  if (flags & O_NONBLOCK) {
    return FALSE;
  }

  return TRUE;
}

static int tls_accept(conn_t *conn, unsigned char on_data) {
  int blocking, res = 0, xerrno = 0;
  long cache_mode = 0;
  char *subj = NULL;
  static unsigned char logged_data = FALSE;
  SSL *ssl = NULL;
  BIO *rbio = NULL, *wbio = NULL;

  if (!ssl_ctx) {
    tls_log("%s", "unable to start session: null SSL_CTX");
    return -1;
  }

  ssl = SSL_new(ssl_ctx);
  if (ssl == NULL) {
    tls_log("error: unable to start session: %s",
      ERR_error_string(ERR_get_error(), NULL));
    return -2;
  }

  /* This works with either rfd or wfd (I hope). */
  rbio = BIO_new_socket(conn->rfd, FALSE);
  wbio = BIO_new_socket(conn->wfd, FALSE);
  SSL_set_bio(ssl, rbio, wbio);

  /* If configured, set a timer for the handshake. */
  if (tls_handshake_timeout) {
    tls_handshake_timer_id = pr_timer_add(tls_handshake_timeout, -1,
      &tls_module, tls_handshake_timeout_cb, "SSL/TLS handshake");
  }

  if (on_data) {

    /* Make sure that TCP_NODELAY is enabled for the handshake. */
    pr_inet_set_proto_nodelay(conn->pool, conn, 1);

    /* Make sure that TCP_CORK (aka TCP_NOPUSH) is DISABLED for the handshake.
     * This socket option is set via the pr_inet_set_proto_opts() call made
     * in mod_core, upon handling the PASV/EPSV command.
     */
    (void) pr_inet_set_proto_cork(conn->wfd, 0);

    cache_mode = SSL_CTX_get_session_cache_mode(ssl_ctx);
    if (!(cache_mode & SSL_SESS_CACHE_OFF)) {
      /* Disable STORING of any new session IDs in the session cache. We DO
       * want to allow LOOKUP of session IDs in the session cache, however.
       */
      long data_cache_mode;
      data_cache_mode = SSL_SESS_CACHE_SERVER|SSL_SESS_CACHE_NO_INTERNAL_STORE;
      SSL_CTX_set_session_cache_mode(ssl_ctx, data_cache_mode);
    }
  }

  retry:

  blocking = tls_get_block(conn);
  if (blocking) {
    /* Put the connection in non-blocking mode for the duration of the
     * SSL handshake.  This lets us handle EAGAIN/retries better (i.e.
     * without spinning in a tight loop and consuming the CPU).
     */
    pr_inet_set_nonblock(conn->pool, conn);
  }

  pr_signals_handle();
  res = SSL_accept(ssl);
  if (res == -1) {
    xerrno = errno;
  }

  if (blocking) {
    /* Return the connection to blocking mode. */
    pr_inet_set_block(conn->pool, conn);
  }

  if (res < 1) {
    const char *msg = "unable to accept TLS connection";
    int errcode = SSL_get_error(ssl, res);

    pr_signals_handle();

    if (tls_handshake_timed_out) {
      tls_log("TLS negotiation timed out (%u seconds)", tls_handshake_timeout);
      tls_end_sess(ssl, on_data ? PR_NETIO_STRM_DATA : PR_NETIO_STRM_CTRL, 0);
      return -4;
    }

    switch (errcode) {
      case SSL_ERROR_WANT_READ:
        pr_trace_msg(trace_channel, 17,
          "SSL_accept() returned WANT_READ, waiting for more to "
          "read on fd %d", conn->rfd);
        tls_readmore(conn->rfd);
        goto retry;

      case SSL_ERROR_WANT_WRITE:
        pr_trace_msg(trace_channel, 17,
          "SSL_accept() returned WANT_WRITE, waiting for more to "
          "write on fd %d", conn->rfd);
        tls_writemore(conn->rfd);
        goto retry;

      case SSL_ERROR_ZERO_RETURN:
        tls_log("%s: TLS connection closed", msg);
        break;

      case SSL_ERROR_WANT_X509_LOOKUP:
        tls_log("%s: needs X509 lookup", msg);
        break;

      case SSL_ERROR_SYSCALL: {
        /* Check to see if the OpenSSL error queue has info about this. */
        int xerrcode = ERR_get_error();
    
        if (xerrcode == 0) {
          /* The OpenSSL error queue doesn't have any more info, so we'll
           * examine the SSL_accept() return value itself.
           */

          if (res == 0) {
            /* EOF */
            tls_log("%s: received EOF that violates protocol", msg);

          } else if (res == -1) {
            /* Check errno */
            tls_log("%s: system call error: [%d] %s", msg, xerrno,
              strerror(xerrno));
          }

        } else {
          tls_log("%s: system call error: %s", msg, tls_get_errors());
        }

        break;
      }

      case SSL_ERROR_SSL:
        tls_log("%s: protocol error: %s", msg, tls_get_errors());
        break;
    }

    if (on_data) {
      pr_event_generate("mod_tls.data-handshake-failed", &errcode);

    } else {
      pr_event_generate("mod_tls.ctrl-handshake-failed", &errcode);
    }

    tls_end_sess(ssl, on_data ? PR_NETIO_STRM_DATA : PR_NETIO_STRM_CTRL, 0);
    return -3;
  }

  if (on_data) {
    /* Disable TCP_NODELAY, now that the handshake is done. */
    pr_inet_set_proto_nodelay(conn->pool, conn, 0);

    /* Reenable TCP_CORK (aka TCP_NOPUSH), now that the handshake is done. */
    (void) pr_inet_set_proto_cork(conn->wfd, 1);

    if (!(cache_mode & SSL_SESS_CACHE_OFF)) {
      /* Restore the previous session cache mode. */
      SSL_CTX_set_session_cache_mode(ssl_ctx, cache_mode);
    }
  }
 
  /* Disable the handshake timer. */
  pr_timer_remove(tls_handshake_timer_id, &tls_module);

  /* Manually update the raw bytes counters with the network IO from the
   * SSL handshake.
   */
  session.total_raw_in += (BIO_number_read(rbio) +
    BIO_number_read(wbio));
  session.total_raw_out += (BIO_number_written(rbio) +
    BIO_number_written(wbio));

  /* Stash the SSL object in the pointers of the correct NetIO streams. */
  if (conn == session.c) {
    pr_buffer_t *strm_buf;

    ctrl_ssl = ssl;

    pr_table_add(tls_ctrl_rd_nstrm->notes,
      pstrdup(tls_ctrl_rd_nstrm->strm_pool, TLS_NETIO_NOTE),
      ssl, sizeof(SSL *));
    pr_table_add(tls_ctrl_wr_nstrm->notes,
      pstrdup(tls_ctrl_wr_nstrm->strm_pool, TLS_NETIO_NOTE),
      ssl, sizeof(SSL *));

#if OPENSSL_VERSION_NUMBER >= 0x009080dfL
    if (SSL_get_secure_renegotiation_support(ssl) == 1) {
      /* If the peer indicates that it can support secure renegotiations, log
       * this fact for reference.
       *
       * Note that while we could use this fact to automatically enable the
       * AllowClientRenegotiations TLSOption, in light of CVE-2011-1473
       * (where malicious SSL/TLS clients can abuse the fact that session
       * renegotiations are more computationally intensive for servers than
       * for clients and repeatedly request renegotiations to create a
       * denial of service attach), we won't enable AllowClientRenegotiations
       * programmatically.  The admin will still need to explicitly configure
       * that.
       */
      tls_log("%s", "client supports secure renegotiations");
    }
#endif /* OpenSSL 0.9.8m and later */

    /* Clear any data from the NetIO stream buffers which may have been read
     * in before the SSL/TLS handshake occurred (Bug#3624).
     */
    strm_buf = tls_ctrl_rd_nstrm->strm_buf;
    if (strm_buf != NULL) {
      strm_buf->current = NULL;
      strm_buf->remaining = strm_buf->buflen;
    }

  } else if (conn == session.d) {
    pr_buffer_t *strm_buf;

    pr_table_add(tls_data_rd_nstrm->notes,
      pstrdup(tls_data_rd_nstrm->strm_pool, TLS_NETIO_NOTE),
      ssl, sizeof(SSL *));
    pr_table_add(tls_data_wr_nstrm->notes,
      pstrdup(tls_data_wr_nstrm->strm_pool, TLS_NETIO_NOTE),
      ssl, sizeof(SSL *));

    /* Clear any data from the NetIO stream buffers which may have been read
     * in before the SSL/TLS handshake occurred (Bug#3624).
     */
    strm_buf = tls_data_rd_nstrm->strm_buf;
    if (strm_buf != NULL) {
      strm_buf->current = NULL;
      strm_buf->remaining = strm_buf->buflen;
    }
  }

#if OPENSSL_VERSION_NUMBER == 0x009080cfL
  /* In OpenSSL-0.9.8l, SSL session renegotiations are automatically
   * disabled.  Thus if the admin explicitly configured support for
   * client-initiated renegotations via the AllowClientRenegotiations
   * TLSOption, then we need to do some hackery to enable renegotiations.
   */
  if (tls_opts & TLS_OPT_ALLOW_CLIENT_RENEGOTIATIONS) {
    ssl->s3->flags |= SSL3_FLAGS_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
  }
#endif

  /* TLS handshake on the control channel... */
  if (!on_data) {
    int reused;

    subj = tls_get_subj_name(ctrl_ssl);
    if (subj)
      tls_log("Client: %s", subj);

    if (!(tls_opts & TLS_OPT_NO_CERT_REQUEST)) {

      /* Now we can go on with our post-handshake, application level
       * requirement checks.
       */
      if (tls_check_client_cert(ssl, conn) < 0) {
        tls_end_sess(ssl, PR_NETIO_STRM_CTRL, 0);
        ctrl_ssl = NULL;
        return -1;
      }
    }

    reused = SSL_session_reused(ssl);

    tls_log("%s connection accepted, using cipher %s (%d bits%s)",
      SSL_get_version(ssl), SSL_get_cipher_name(ssl),
      SSL_get_cipher_bits(ssl, NULL),
      reused > 0 ? ", resumed session" : "");

    /* Setup the TLS environment variables, if requested. */
    tls_setup_environ(ssl);

    if (reused > 0) {
      pr_log_writefile(tls_logfd, MOD_TLS_VERSION, "%s",
        "client reused previous SSL session for control connection");
    }

  /* TLS handshake on the data channel... */
  } else {

    /* We won't check for session reuse for data connections when either
     * a) the NoSessionReuseRequired TLSOption has been configured, or
     * b) the CCC command has been used (Bug#3465).
     */
    if (!(tls_opts & TLS_OPT_NO_SESSION_REUSE_REQUIRED) &&
        !(tls_flags & TLS_SESS_HAVE_CCC)) {
      int reused;
      SSL_SESSION *ctrl_sess;

      /* Ensure that the following conditions are met:
       *
       *   1. The client reused an existing SSL session
       *   2. The reused SSL session matches the SSL session from the control
       *      connection.
       *
       * Shutdown the SSL session unless the conditions are met.  By
       * requiring these conditions, we make sure that the client which is
       * talking to us on the control connection is indeed the same client
       * that is using this data connection.  Without these checks, a
       * malicious client might be able to hijack/steal the data transfer.
       */

      reused = SSL_session_reused(ssl);
      if (reused != 1) {
        tls_log("client did not reuse SSL session, rejecting data connection "
          "(see the NoSessionReuseRequired TLSOptions parameter)");
        tls_end_sess(ssl, PR_NETIO_STRM_DATA, 0);
        pr_table_remove(tls_data_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
        pr_table_remove(tls_data_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
        return -1;

      } else {
        tls_log("%s", "client reused SSL session for data connection");
      }

      ctrl_sess = SSL_get_session(ctrl_ssl);
      if (ctrl_sess != NULL) {
        SSL_SESSION *data_sess;

        data_sess = SSL_get_session(ssl);
        if (data_sess != NULL) {
          int matching_sess_id = -1;

#if OPENSSL_VERSION_NUMBER < 0x000907000L
          /* In the OpenSSL source code, SSL_SESSION_cmp() ultimately uses
           * memcmp(3) to check, and thus returns memcmp(3)'s return value.
           */
          matching_sess_id = SSL_SESSION_cmp(ctrl_sess, data_sess);
          if (matching_sess_id != 0) {
#else
          unsigned char *sess_id;
          unsigned int sess_id_len;

# if OPENSSL_VERSION_NUMBER > 0x000908000L
          sess_id = (unsigned char *) SSL_SESSION_get_id(data_sess,
            &sess_id_len);
# else
          /* XXX Directly accessing these fields cannot be a Good Thing. */
          sess_id = data_sess->session_id;
          sess_id_len = data_sess->session_id_length;
# endif
 
          matching_sess_id = SSL_has_matching_session_id(ctrl_ssl, sess_id,
            sess_id_len);
          if (matching_sess_id == 0) {
#endif
            tls_log("Client did not reuse SSL session from control channel, "
              "rejecting data connection (see the NoSessionReuseRequired "
              "TLSOptions parameter)");
            tls_end_sess(ssl, PR_NETIO_STRM_DATA, 0);
            pr_table_remove(tls_data_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
            pr_table_remove(tls_data_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
            return -1;

          } else {
            long sess_created, sess_expires;
            time_t now;

            /* The SSL session ID for data and control channels matches.
             *
             * Many sites are using mod_tls such that OpenSSL's internal
             * session caching is being used.  And by default, that
             * cache expires sessions after 300 secs (5 min).  It's possible
             * that the control channel SSL session will expire soon;
             * unless the client renegotiates that session, the internal
             * cache will expire the cached session, and the next data
             * transfer could fail (since mod_tls won't allow that session ID
             * to be reused again, as it will no longer be in the session
             * cache).
             *
             * Try to warn if this is about to happen.
             */

            sess_created = SSL_SESSION_get_time(ctrl_sess);
            sess_expires = SSL_SESSION_get_timeout(ctrl_sess);
            now = time(NULL);

            if ((sess_created + sess_expires) >= now) {
              unsigned long remaining;

              remaining = (unsigned long) ((sess_created + sess_expires) - now);

              if (remaining <= 60) {
                tls_log("control channel SSL session expires in %lu secs (%lu session cache expiration)", remaining, sess_expires);
                tls_log("%s","Consider using 'TLSSessionCache internal:' to increase the session cache expiration if necessary, or renegotiate the control channel SSL session");
              }
            }
          }

        } else {
          /* This should never happen, so log if it does. */
          tls_log("BUG: unable to determine whether client reused SSL session: SSL_get_session() for control connection return NULL");
          tls_log("rejecting data connection (see TLSOption NoSessionReuseRequired)");
          tls_end_sess(ssl, PR_NETIO_STRM_DATA, 0);
          pr_table_remove(tls_data_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
          pr_table_remove(tls_data_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
          return -1;
        }

      } else {
        /* This should never happen, so log if it does. */
        tls_log("BUG: unable to determine whether client reused SSL session: SSL_get_session() for control connection return NULL");
        tls_log("rejecting data connection (see TLSOption NoSessionReuseRequired)");
        tls_end_sess(ssl, PR_NETIO_STRM_DATA, 0);
        pr_table_remove(tls_data_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
        pr_table_remove(tls_data_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
        return -1;
      }
    }

    /* Only be verbose with the first TLS data connection, otherwise there
     * might be too much noise.
     */
    if (!logged_data) {
      tls_log("%s data connection accepted, using cipher %s (%d bits)",
        SSL_get_version(ssl), SSL_get_cipher_name(ssl),
        SSL_get_cipher_bits(ssl, NULL));
      logged_data = TRUE;
    }
  }

  return 0;
}

static int tls_connect(conn_t *conn) {
  int blocking, res = 0, xerrno = 0;
  char *subj = NULL;
  SSL *ssl = NULL;
  BIO *rbio = NULL, *wbio = NULL;

  if (!ssl_ctx) {
    tls_log("%s", "unable to start session: null SSL_CTX");
    return -1;
  }

  ssl = SSL_new(ssl_ctx);
  if (ssl == NULL) {
    tls_log("error: unable to start session: %s",
      ERR_error_string(ERR_get_error(), NULL));
    return -2;
  }

  /* We deliberately set SSL_VERIFY_NONE here, so that we get to
   * determine how to handle the server cert verification result ourselves.
   */
  SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);

  /* This works with either rfd or wfd (I hope). */
  rbio = BIO_new_socket(conn->rfd, FALSE);
  wbio = BIO_new_socket(conn->rfd, FALSE);
  SSL_set_bio(ssl, rbio, wbio);

  /* If configured, set a timer for the handshake. */
  if (tls_handshake_timeout) {
    tls_handshake_timer_id = pr_timer_add(tls_handshake_timeout, -1,
      &tls_module, tls_handshake_timeout_cb, "SSL/TLS handshake");
  }

  /* Make sure that TCP_NODELAY is enabled for the handshake. */
  pr_inet_set_proto_nodelay(conn->pool, conn, 1);

  retry:

  blocking = tls_get_block(conn);
  if (blocking) {
    /* Put the connection in non-blocking mode for the duration of the
     * SSL handshake.  This lets us handle EAGAIN/retries better (i.e.
     * without spinning in a tight loop and consuming the CPU).
     */
    pr_inet_set_nonblock(conn->pool, conn);
  }

  pr_signals_handle();
  res = SSL_connect(ssl);
  if (res == -1) {
    xerrno = errno;
  }

  if (blocking) {
    /* Return the connection to blocking mode. */
    pr_inet_set_block(conn->pool, conn);
  }

  if (res < 1) {
    const char *msg = "unable to connect using TLS connection";
    int errcode = SSL_get_error(ssl, res);

    pr_signals_handle();

    if (tls_handshake_timed_out) {
      tls_log("TLS negotiation timed out (%u seconds)", tls_handshake_timeout);
      tls_end_sess(ssl, PR_NETIO_STRM_DATA, 0);
      return -4;
    }

    switch (errcode) {
      case SSL_ERROR_WANT_READ:
        pr_trace_msg(trace_channel, 17,
          "SSL_connect() returned WANT_READ, waiting for more to "
          "read on fd %d", conn->rfd);
        tls_readmore(conn->rfd);
        goto retry;

      case SSL_ERROR_WANT_WRITE:
        pr_trace_msg(trace_channel, 17,
          "SSL_connect() returned WANT_READ, waiting for more to "
          "read on fd %d", conn->rfd);
        tls_writemore(conn->rfd);
        goto retry;

      case SSL_ERROR_ZERO_RETURN:
        tls_log("%s: TLS connection closed", msg);
        break;

      case SSL_ERROR_WANT_X509_LOOKUP:
        tls_log("%s: needs X509 lookup", msg);
        break;

      case SSL_ERROR_SYSCALL: {
        /* Check to see if the OpenSSL error queue has info about this. */
        int xerrcode = ERR_get_error();
    
        if (xerrcode == 0) {
          /* The OpenSSL error queue doesn't have any more info, so we'll
           * examine the SSL_connect() return value itself.
           */

          if (res == 0) {
            /* EOF */
            tls_log("%s: received EOF that violates protocol", msg);

          } else if (res == -1) {
            /* Check errno */
            tls_log("%s: system call error: [%d] %s", msg, xerrno,
              strerror(xerrno));
          }

        } else {
          tls_log("%s: system call error: %s", msg, tls_get_errors());
        }

        break;
      }

      case SSL_ERROR_SSL:
        tls_log("%s: protocol error: %s", msg, tls_get_errors());
        break;
    }

    pr_event_generate("mod_tls.data-handshake-failed", &errcode);

    tls_end_sess(ssl, PR_NETIO_STRM_DATA, 0);
    return -3;
  }

  /* Disable TCP_NODELAY, now that the handshake is done. */
  pr_inet_set_proto_nodelay(conn->pool, conn, 0);
 
  /* Disable the handshake timer. */
  pr_timer_remove(tls_handshake_timer_id, &tls_module);

  /* Manually update the raw bytes counters with the network IO from the
   * SSL handshake.
   */
  session.total_raw_in += (BIO_number_read(rbio) +
    BIO_number_read(wbio));
  session.total_raw_out += (BIO_number_written(rbio) +
    BIO_number_written(wbio));

  /* Stash the SSL object in the pointers of the correct NetIO streams. */
  if (conn == session.d) {
    pr_buffer_t *strm_buf;

    pr_table_add(tls_data_rd_nstrm->notes,
      pstrdup(tls_data_rd_nstrm->strm_pool, TLS_NETIO_NOTE),
      ssl, sizeof(SSL *));
    pr_table_add(tls_data_wr_nstrm->notes,
      pstrdup(tls_data_wr_nstrm->strm_pool, TLS_NETIO_NOTE),
      ssl, sizeof(SSL *));

    /* Clear any data from the NetIO stream buffers which may have been read
     * in before the SSL/TLS handshake occurred (Bug#3624).
     */
    strm_buf = tls_data_rd_nstrm->strm_buf;
    if (strm_buf != NULL) {
      strm_buf->current = NULL;
      strm_buf->remaining = strm_buf->buflen;
    }
  }

#if OPENSSL_VERSION_NUMBER == 0x009080cfL
  /* In OpenSSL-0.9.8l, SSL session renegotiations are automatically
   * disabled.  Thus if the admin explicitly configured support for
   * client-initiated renegotations via the AllowClientRenegotiations
   * TLSOption, then we need to do some hackery to enable renegotiations.
   */
  if (tls_opts & TLS_OPT_ALLOW_CLIENT_RENEGOTIATIONS) {
    ssl->s3->flags |= SSL3_FLAGS_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
  }
#endif

  subj = tls_get_subj_name(ssl);
  if (subj) {
    tls_log("Server: %s", subj);
  }

  if (tls_check_server_cert(ssl, conn) < 0) {
    tls_end_sess(ssl, PR_NETIO_STRM_DATA, 0);
    return -1;
  }

  tls_log("%s connection created, using cipher %s (%d bits)",
    SSL_get_version(ssl), SSL_get_cipher_name(ssl),
    SSL_get_cipher_bits(ssl, NULL));

  return 0;
}

static void tls_cleanup(int flags) {

  tls_sess_cache_close();

#if OPENSSL_VERSION_NUMBER > 0x000907000L
  if (tls_crypto_device) {
    ENGINE_cleanup();
    tls_crypto_device = NULL;
  }
#endif

  if (tls_crl_store) {
    X509_STORE_free(tls_crl_store);
    tls_crl_store = NULL;
  }

  if (ssl_ctx) {
    SSL_CTX_free(ssl_ctx);
    ssl_ctx = NULL;
  }

  if (tls_tmp_dhs) {
    register unsigned int i;
    DH **dhs;

    dhs = tls_tmp_dhs->elts;
    for (i = 0; i < tls_tmp_dhs->nelts; i++) {
      DH_free(dhs[i]);
    }

    tls_tmp_dhs = NULL;
  }

  if (tls_tmp_rsa) {
    RSA_free(tls_tmp_rsa);
    tls_tmp_rsa = NULL;
  }

  if (!(flags & TLS_CLEANUP_FL_SESS_INIT)) {
    ERR_free_strings();

#if OPENSSL_VERSION_NUMBER >= 0x10000001L
    /* The ERR_remove_state(0) usage is deprecated due to thread ID
     * differences among platforms; see the OpenSSL-1.0.0 CHANGES file
     * for details.  So for new enough OpenSSL installations, use the
     * proper way to clear the error queue state.
     */
    ERR_remove_thread_state(NULL);
#else
    ERR_remove_state(0);
#endif /* OpenSSL prior to 1.0.0-beta1 */

    EVP_cleanup();

  } else {
    /* Only call EVP_cleanup() et al if other OpenSSL-using modules are not
     * present.  If we called EVP_cleanup() here during session
     * initialization, and other modules want to use OpenSSL, we may
     * be depriving those modules of OpenSSL functionality.
     *
     * At the moment, the modules known to use OpenSSL are mod_ldap,
     * mod_sftp, mod_sql, and mod_sql_passwd.
     */
    if (pr_module_get("mod_ldap.c") == NULL &&
        pr_module_get("mod_sftp.c") == NULL &&
        pr_module_get("mod_sql.c") == NULL &&
        pr_module_get("mod_sql_passwd.c") == NULL) {
      ERR_free_strings();

#if OPENSSL_VERSION_NUMBER >= 0x10000001L
      /* The ERR_remove_state(0) usage is deprecated due to thread ID
       * differences among platforms; see the OpenSSL-1.0.0c CHANGES file
       * for details.  So for new enough OpenSSL installations, use the
       * proper way to clear the error queue state.
       */
      ERR_remove_thread_state(NULL);
#else
      ERR_remove_state(0);
#endif /* OpenSSL prior to 1.0.0-beta1 */

      EVP_cleanup();
    }
  }
}

static void tls_end_sess(SSL *ssl, int strms, int flags) {
  int res = 0;
  int shutdown_state;
  BIO *rbio, *wbio;
  int bread, bwritten;
  unsigned long rbio_rbytes, rbio_wbytes, wbio_rbytes, wbio_wbytes; 

  if (!ssl)
    return;

  rbio = SSL_get_rbio(ssl);
  rbio_rbytes = BIO_number_read(rbio);
  rbio_wbytes = BIO_number_written(rbio);

  wbio = SSL_get_wbio(ssl);
  wbio_rbytes = BIO_number_read(wbio);
  wbio_wbytes = BIO_number_written(wbio);

  /* A 'close_notify' alert (SSL shutdown message) may have been previously
   * sent to the client via tls_netio_shutdown_cb().
   */

  shutdown_state = SSL_get_shutdown(ssl);
  if (!(shutdown_state & SSL_SENT_SHUTDOWN)) {
    errno = 0;

    /* 'close_notify' not already sent; send it now. */
    res = SSL_shutdown(ssl);
  }

  if (res == 0) {
    /* Now call SSL_shutdown() again, but only if necessary. */
    if (flags & TLS_SHUTDOWN_BIDIRECTIONAL) {
      shutdown_state = SSL_get_shutdown(ssl);

      res = 1;
      if (!(shutdown_state & SSL_RECEIVED_SHUTDOWN)) {
        errno = 0;
        res = SSL_shutdown(ssl);
      }
    }

    /* If SSL_shutdown() returned -1 here, an error occurred during the
     * shutdown.
     */
    if (res < 0) {
      long err_code;

      err_code = SSL_get_error(ssl, res);
      switch (err_code) {
        case SSL_ERROR_WANT_READ:
          tls_log("SSL_shutdown error: WANT_READ");
          pr_log_debug(DEBUG0, MOD_TLS_VERSION
            ": SSL_shutdown error: WANT_READ");
          break;

        case SSL_ERROR_WANT_WRITE:
          tls_log("SSL_shutdown error: WANT_WRITE");
          pr_log_debug(DEBUG0, MOD_TLS_VERSION
            ": SSL_shutdown error: WANT_WRITE");
          break;

        case SSL_ERROR_ZERO_RETURN:
          /* Clean shutdown, nothing we need to do. */
          break;

        case SSL_ERROR_SYSCALL:
          if (errno != 0 &&
              errno != EOF &&
              errno != EBADF &&
              errno != EPIPE &&
              errno != EPERM &&
              errno != ENOSYS) {
            tls_log("SSL_shutdown syscall error: %s", strerror(errno));
          }
          break;

        default:
          tls_log("SSL_shutdown error [%ld]: %s", err_code, tls_get_errors());
          pr_log_debug(DEBUG0, MOD_TLS_VERSION
            ": SSL_shutdown error [%ld]: %s", err_code, tls_get_errors());
          break;
      }
    }

  } else if (res < 0) {
    long err_code;

    err_code = SSL_get_error(ssl, res);
    switch (err_code) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
      case SSL_ERROR_ZERO_RETURN:
        /* Clean shutdown, nothing we need to do.  The WANT_READ/WANT_WRITE
         * error codes crept into OpenSSL 0.9.8m, with changes to make
         * SSL_shutdown() work properly for non-blocking sockets.  And
         * handling these error codes for older OpenSSL versions won't break
         * things.
         */
        break;

      case SSL_ERROR_SYSCALL:
        if (errno != 0 &&
            errno != EOF &&
            errno != EBADF &&
            errno != EPIPE &&
            errno != EPERM &&
            errno != ENOSYS) {
          tls_log("SSL_shutdown syscall error: %s", strerror(errno));
        }
        break;

      default:
        tls_fatal_error(err_code, __LINE__);
        break;
    }
  }

  bread = (BIO_number_read(rbio) - rbio_rbytes) +
    (BIO_number_read(wbio) - wbio_rbytes);
  bwritten = (BIO_number_written(rbio) - rbio_wbytes) +
    (BIO_number_written(wbio) - wbio_wbytes);

  /* Manually update session.total_raw_in/out, in order to have %I/%O be
   * accurately represented for the raw traffic.
   */
  if (bread > 0) {
    session.total_raw_in += bread;
  }

  if (bwritten > 0) {
    session.total_raw_out += bwritten;
  }

  SSL_free(ssl);
}

static const char *tls_get_errors(void) {
  unsigned int count = 0;
  unsigned long e = ERR_get_error();
  BIO *bio = NULL;
  char *data = NULL;
  long datalen;
  const char *str = "(unknown)";

  /* Use ERR_print_errors() and a memory BIO to build up a string with
   * all of the error messages from the error queue.
   */

  if (e)
    bio = BIO_new(BIO_s_mem());

  while (e) {
    pr_signals_handle();
    BIO_printf(bio, "\n  (%u) %s", ++count, ERR_error_string(e, NULL));
    e = ERR_get_error(); 
  }

  datalen = BIO_get_mem_data(bio, &data);
  if (data) {
    data[datalen] = '\0';
    str = pstrdup(session.pool, data);
  }

  if (bio)
    BIO_free(bio);

  return str;
}

/* Return a page-aligned pointer to memory of at least the given size.
 */
static char *tls_get_page(size_t sz, void **ptr) {
  void *d;
  long pagesz = tls_get_pagesz(), p;

  d = calloc(1, sz + (pagesz-1));
  if (d == NULL) {
    pr_log_pri(PR_LOG_ALERT, MOD_TLS_VERSION ": Out of memory!");
    pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_NOMEM, NULL);
  }

  *ptr = d;

  p = ((long) d + (pagesz-1)) &~ (pagesz-1);

  return ((char *) p);
}

/* Return the size of a page on this architecture.
 */
static size_t tls_get_pagesz(void) {
  long pagesz;

#if defined(_SC_PAGESIZE)
  pagesz = sysconf(_SC_PAGESIZE);
#elif defined(_SC_PAGE_SIZE)
  pagesz = sysconf(_SC_PAGE_SIZE);
#else
  /* Default to using OpenSSL's defined buffer size for PEM files. */
  pagesz = PEM_BUFSIZE;
#endif /* !_SC_PAGESIZE and !_SC_PAGE_SIZE */

  return pagesz;
}

static char *tls_get_subj_name(SSL *ssl) {
  X509 *cert = SSL_get_peer_certificate(ssl);

  if (cert) {
    char *name = tls_x509_name_oneline(X509_get_subject_name(cert));
    X509_free(cert);
    return name;
  }

  return NULL;
}

static void tls_fatal_error(long error, int lineno) {

  switch (error) {
    case SSL_ERROR_NONE:
      return;

    case SSL_ERROR_SSL:
      tls_log("panic: SSL_ERROR_SSL, line %d: %s", lineno, tls_get_errors());
      break;

    case SSL_ERROR_WANT_READ:
      tls_log("panic: SSL_ERROR_WANT_READ, line %d", lineno);
      break;

    case SSL_ERROR_WANT_WRITE:
      tls_log("panic: SSL_ERROR_WANT_WRITE, line %d", lineno);
      break;

    case SSL_ERROR_WANT_X509_LOOKUP:
      tls_log("panic: SSL_ERROR_WANT_X509_LOOKUP, line %d", lineno);
      break;

    case SSL_ERROR_SYSCALL: {
      long xerrcode = ERR_get_error();

      if (errno == ECONNRESET)
        return;

      /* Check to see if the OpenSSL error queue has info about this. */
      if (xerrcode == 0) {
        /* The OpenSSL error queue doesn't have any more info, so we'll
         * examine the error value itself.
         */

        if (errno == EOF) {
          tls_log("panic: SSL_ERROR_SYSCALL, line %d: "
            "EOF that violates protocol", lineno);

        } else {
          /* Check errno */
          tls_log("panic: SSL_ERROR_SYSCALL, line %d: system error: %s", lineno,
            strerror(errno));
        }

      } else {
        tls_log("panic: SSL_ERROR_SYSCALL, line %d: %s", lineno,
          tls_get_errors());
      }

      break;
    }

    case SSL_ERROR_ZERO_RETURN:
      tls_log("panic: SSL_ERROR_ZERO_RETURN, line %d", lineno);
      break;

    case SSL_ERROR_WANT_CONNECT:
      tls_log("panic: SSL_ERROR_WANT_CONNECT, line %d", lineno);
      break;

    default:
      tls_log("panic: SSL_ERROR %ld, line %d", error, lineno);
      break;
  }

  tls_log("%s", "unexpected OpenSSL error, disconnecting");
  pr_log_pri(PR_LOG_WARNING, "%s", MOD_TLS_VERSION
    ": unexpected OpenSSL error, disconnecting");

  pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_BY_APPLICATION, NULL);
}

/* This function checks if the client's cert is in the ~/.tlslogin file
 * of the "user".
 */
static int tls_dotlogin_allow(const char *user) {
  char buf[512] = {'\0'}, *home = NULL;
  FILE *fp = NULL;
  X509 *client_cert = NULL, *file_cert = NULL;
  struct passwd *pwd = NULL;
  pool *tmp_pool = NULL;
  unsigned char allow_user = FALSE;
  int xerrno;

  if (!(tls_flags & TLS_SESS_ON_CTRL) ||
      ctrl_ssl == NULL ||
      user == NULL) {
    return FALSE;
  }

  /* If the client did not provide a cert, we cannot do the .tlslogin check. */
  client_cert = SSL_get_peer_certificate(ctrl_ssl);
  if (client_cert == NULL) {
    return FALSE;
  }

  tmp_pool = make_sub_pool(permanent_pool);

  PRIVS_ROOT
  pwd = pr_auth_getpwnam(tmp_pool, user);
  PRIVS_RELINQUISH

  if (pwd == NULL) {
    X509_free(client_cert);
    destroy_pool(tmp_pool);
    return FALSE;
  }

  /* Handle the case where the user's home directory is a symlink. */
  PRIVS_USER
  home = dir_realpath(tmp_pool, pwd->pw_dir);
  PRIVS_RELINQUISH

  snprintf(buf, sizeof(buf), "%s/.tlslogin", home ? home : pwd->pw_dir);
  buf[sizeof(buf)-1] = '\0';

  /* No need for the temporary pool any more. */
  destroy_pool(tmp_pool);
  tmp_pool = NULL;

  PRIVS_ROOT
  fp = fopen(buf, "r");
  xerrno = errno;
  PRIVS_RELINQUISH

  if (fp == NULL) {
    X509_free(client_cert);
    tls_log(".tlslogin check: unable to open '%s': %s", buf, strerror(xerrno));
    return FALSE;
  }

  while ((file_cert = PEM_read_X509(fp, NULL, NULL, NULL))) {
    pr_signals_handle();

    if (!M_ASN1_BIT_STRING_cmp(client_cert->signature, file_cert->signature)) {
      allow_user = TRUE;
    }

    X509_free(file_cert);
    if (allow_user) {
      break;
    }
  }

  X509_free(client_cert);
  fclose(fp);

  return allow_user;
}

static int tls_cert_to_user(const char *user_name, const char *field_name) {
  X509 *client_cert = NULL;
  unsigned char allow_user = FALSE;
  unsigned char *field_value = NULL;

  if (!(tls_flags & TLS_SESS_ON_CTRL) ||
      ctrl_ssl == NULL ||
      user_name == NULL ||
      field_name == NULL) {
    return FALSE;
  }

  /* If the client did not provide a cert, we cannot do the TLSUserName
   * check.
   */
  client_cert = SSL_get_peer_certificate(ctrl_ssl);
  if (client_cert == NULL) {
    return FALSE;
  }

  if (strcmp(field_name, "CommonName") == 0) {
    X509_NAME *name;
    int pos = -1;
 
    name = X509_get_subject_name(client_cert);

    while (TRUE) {
      X509_NAME_ENTRY *entry;
      ASN1_STRING *data;
      int data_len;
      unsigned char *data_str = NULL;

      pr_signals_handle();

      pos = X509_NAME_get_index_by_NID(name, NID_commonName, pos);
      if (pos == -1) {
        break;
      }

      entry = X509_NAME_get_entry(name, pos);
      data = X509_NAME_ENTRY_get_data(entry);
      data_len = ASN1_STRING_length(data);
      data_str = ASN1_STRING_data(data);

      /* Watch for any embedded NULs, which can cause verification
       * problems via spoofing.
       */
      if (data_len != strlen((char *) data_str)) {
        tls_log("%s", "client cert CommonName contains embedded NULs, "
          "ignoring as possible spoof attempt");
        tls_log("suspicious CommonName value: '%s'", data_str);

      } else {

        /* There can be multiple CommonNames... */
        if (strcmp((char *) data_str, user_name) == 0) {
          field_value = data_str;
          allow_user = TRUE;

          tls_log("matched client cert CommonName '%s' to user '%s'",
            field_value, user_name);
          break;
        }
      }
    }

  } else if (strcmp(field_name, "EmailSubjAltName") == 0) {
    STACK_OF(GENERAL_NAME) *sk_alt_names;

    sk_alt_names = X509_get_ext_d2i(client_cert, NID_subject_alt_name, NULL,
      NULL);
    if (sk_alt_names != NULL) {
      register unsigned int i;
      int nnames = sk_GENERAL_NAME_num(sk_alt_names);

      for (i = 0; i < nnames; i++) {
        GENERAL_NAME *name = sk_GENERAL_NAME_value(sk_alt_names, i);

        /* We're only looking for the Email type. */
        if (name->type == GEN_EMAIL) {
          int data_len;
          unsigned char *data_str = NULL;

          data_len = ASN1_STRING_length(name->d.ia5);
          data_str = ASN1_STRING_data(name->d.ia5);

          /* Watch for any embedded NULs, which can cause verification
           * problems via spoofing.
           */
          if (data_len != strlen((char *) data_str)) {
            tls_log("%s", "client cert Email SAN contains embedded NULs, "
              "ignoring as possible spoof attempt");
            tls_log("suspicious Email SubjAltName value: '%s'", data_str);

          } else {

            /* There can be multiple Email SANs... */
            if (strcmp((char *) data_str, user_name) == 0) {
              field_value = data_str;
              allow_user = TRUE;

              tls_log("matched client cert Email SubjAltName '%s' to user '%s'",
                field_value, user_name);
              GENERAL_NAME_free(name);
              break;
            }
          }
        }

        GENERAL_NAME_free(name);
      }

      sk_GENERAL_NAME_free(sk_alt_names);
    }

  } else {
    /* Custom OID. */
    int nexts;

    nexts = X509_get_ext_count(client_cert);
    if (nexts > 0) {
      register unsigned int i;

      for (i = 0; i < nexts; i++) {
        X509_EXTENSION *ext = NULL;
        ASN1_OBJECT *asn_object = NULL;
        char oid[PR_TUNABLE_PATH_MAX];

        ext = X509_get_ext(client_cert, i);
        asn_object = X509_EXTENSION_get_object(ext);

        /* Get the OID of this extension, as a string. */
        memset(oid, '\0', sizeof(oid));
        if (OBJ_obj2txt(oid, sizeof(oid)-1, asn_object, 1) > 0) {
          if (strcmp(oid, field_name) == 0) {
            ASN1_OCTET_STRING *asn_data = NULL;
            unsigned char *asn_datastr = NULL;
            int asn_datalen;

            asn_data = X509_EXTENSION_get_data(ext);
            asn_datalen = ASN1_STRING_length(asn_data);
            asn_datastr = ASN1_STRING_data(asn_data);

            /* Watch for any embedded NULs, which can cause verification
             * problems via spoofing.
             */
            if (asn_datalen != strlen((char *) asn_datastr)) {
              tls_log("client cert %s extension contains embedded NULs, "
                "ignoring as possible spoof attempt", field_name);
              tls_log("suspicious %s extension value: '%s'", field_name,
                asn_datastr);

            } else {

              /* There might be multiple matching extensions? */
              if (strcmp((char *) asn_datastr, user_name) == 0) {
                field_value = asn_datastr;
                allow_user = TRUE;

                tls_log("matched client cert %s extension '%s' to user '%s'",
                  field_name, field_value, user_name);
                break;
              }
            }
          }
        }
      }
    }
  }

  X509_free(client_cert);
  return allow_user;
}

static int tls_readmore(int rfd) {
  fd_set rfds;
  struct timeval tv;

  FD_ZERO(&rfds);
  FD_SET(rfd, &rfds);

  /* Use a timeout of 15 seconds */
  tv.tv_sec = 15;
  tv.tv_usec = 0;

  return select(rfd + 1, &rfds, NULL, NULL, &tv);
}

static int tls_writemore(int wfd) {
  fd_set wfds;
  struct timeval tv;

  FD_ZERO(&wfds);
  FD_SET(wfd, &wfds);

  /* Use a timeout of 15 seconds */
  tv.tv_sec = 15;
  tv.tv_usec = 0;

  return select(wfd + 1, NULL, &wfds, NULL, &tv);
}

static ssize_t tls_read(SSL *ssl, void *buf, size_t len) {
  ssize_t count;

  retry:
  pr_signals_handle();
  count = SSL_read(ssl, buf, len);
  if (count < 0) {
    long err;
    int fd;

    err = SSL_get_error(ssl, count);
    fd = SSL_get_fd(ssl);

    /* read(2) returns only the generic error number -1 */
    count = -1;

    switch (err) {
      case SSL_ERROR_WANT_READ:
        /* OpenSSL needs more data from the wire to finish the current block,
         * so we wait a little while for it.
         */
        pr_trace_msg(trace_channel, 17,
          "SSL_read() returned WANT_READ, waiting for more to "
          "read on fd %d", fd);
        err = tls_readmore(fd);
        if (err > 0) {
          goto retry;

        } else if (err == 0) {
          /* Still missing data after timeout. Simulate an EINTR and return.
           */
          errno = EINTR;

          /* If err < 0, i.e. some error from the select(), everything is
           * already in place; errno is properly set and this function
           * returns -1.
           */
          break;
        }

      case SSL_ERROR_WANT_WRITE:
        /* OpenSSL needs to write more data to the wire to finish the current
         * block, so we wait a little while for it.
         */
        pr_trace_msg(trace_channel, 17,
          "SSL_read() returned WANT_WRITE, waiting for more to "
          "write on fd %d", fd);
        err = tls_writemore(fd);
        if (err > 0) {
          goto retry;

        } else if (err == 0) {
          /* Still missing data after timeout. Simulate an EINTR and return.
           */
          errno = EINTR;

          /* If err < 0, i.e. some error from the select(), everything is
           * already in place; errno is properly set and this function
           * returns -1.
           */
          break;
        }

      case SSL_ERROR_ZERO_RETURN:
        tls_log("read EOF from client");
        break;

      default:
        tls_fatal_error(err, __LINE__);
        break;
    }
  }

  return count;
}

static RSA *tls_rsa_cb(SSL *ssl, int is_export, int keylen) {
  BIGNUM *e = NULL;

  if (tls_tmp_rsa) {
    return tls_tmp_rsa;
  }

#if OPENSSL_VERSION_NUMBER > 0x000908000L
  e = BN_new();
  if (e == NULL) {
    return NULL;
  }

  if (BN_set_word(e, RSA_F4) != 1) {
    BN_free(e);
    return NULL;
  }

  if (RSA_generate_key_ex(tls_tmp_rsa, keylen, e, NULL) != 1) {
    BN_free(e);
    return NULL;
  }

#else
  tls_tmp_rsa = RSA_generate_key(keylen, RSA_F4, NULL, NULL);
#endif /* OpenSSL version 0.9.8 and later */

  if (e != NULL) {
    BN_free(e);
  }

  return tls_tmp_rsa;
}

static int tls_seed_prng(void) {
  char *heapdata, stackdata[1024];
  static char rand_file[300];
  FILE *fp = NULL;
  
#if OPENSSL_VERSION_NUMBER >= 0x00905100L
  if (RAND_status() == 1)

    /* PRNG already well-seeded. */
    return 0;
#endif

  tls_log("PRNG not seeded with enough data, looking for entropy sources");

  /* If the device '/dev/urandom' is present, OpenSSL uses it by default.
   * Check if it's present, else we have to make random data ourselves.
   */
  fp = fopen("/dev/urandom", "r");
  if (fp) {
    fclose(fp);

    tls_log("device /dev/urandom is present, assuming OpenSSL will use that "
      "for PRNG data");
    return 0;
  }

  /* Lookup any configured TLSRandomSeed. */
  tls_rand_file = get_param_ptr(main_server->conf, "TLSRandomSeed", FALSE);

  if (!tls_rand_file) {
    /* The ftpd's random file is (openssl-dir)/.rnd */
    memset(rand_file, '\0', sizeof(rand_file));
    snprintf(rand_file, sizeof(rand_file)-1, "%s/.rnd",
      X509_get_default_cert_area());
    tls_rand_file = rand_file;
  }

#if OPENSSL_VERSION_NUMBER >= 0x00905100L
  /* In OpenSSL 0.9.5 and later, specifying -1 here means "read the entire
   * file", which is exactly what we want.
   */
  if (RAND_load_file(tls_rand_file, -1) == 0) {
#else

  /* In versions of OpenSSL prior to 0.9.5, we have to specify the amount of
   * bytes to read in.  Since RAND_write_file(3) typically writes 1K of data
   * out, we will read 1K bytes in.
   */
  if (RAND_load_file(tls_rand_file, 1024) != 1024) {
#endif
    struct timeval tv;
    pid_t pid;
 
#if OPENSSL_VERSION_NUMBER >= 0x00905100L
    tls_log("unable to load PRNG seed data from '%s': %s", tls_rand_file,
      tls_get_errors());
#else
    tls_log("unable to load 1024 bytes of PRNG seed data from '%s': %s",
      tls_rand_file, tls_get_errors());
#endif
 
    /* No random file found, create new seed. */
    gettimeofday(&tv, NULL);
    RAND_seed(&(tv.tv_sec), sizeof(tv.tv_sec));
    RAND_seed(&(tv.tv_usec), sizeof(tv.tv_usec));

    pid = getpid();
    RAND_seed(&pid, sizeof(pid_t));
    RAND_seed(stackdata, sizeof(stackdata));

    heapdata = malloc(sizeof(stackdata));
    if (heapdata != NULL) {
      RAND_seed(heapdata, sizeof(stackdata));
      free(heapdata);
    }

  } else {
    tls_log("loaded PRNG seed data from '%s'", tls_rand_file);
  }

#if OPENSSL_VERSION_NUMBER >= 0x00905100L
  if (RAND_status() == 0) {
     /* PRNG still badly seeded. */
     return -1;
  }
#endif

  return 0;
}

/* Note: these mappings should probably be added to the mod_tls docs.
 */

static void tls_setup_cert_ext_environ(const char *env_prefix, X509 *cert) {

  /* NOTE: in the future, add ways of adding subjectAltName (and other
   * extensions?) to the environment.
   */

#if 0
  int nexts = 0;

  nexts = X509_get_ext_count(cert);
  if (nexts > 0) {
    register unsigned int i = 0;

    for (i = 0; i < nexts; i++) {
      X509_EXTENSION *ext = X509_get_ext(cert, i);
      const char *extstr = OBJ_nid2sn(OBJ_obj2nid(
        X509_EXTENSION_get_object(ext)));
    }
  }
#endif

  return;
}

/* Note: these mappings should probably be added to the mod_tls docs.
 *
 *   Name                    Short Name    NID
 *   ----                    ----------    ---
 *   countryName             C             NID_countryName
 *   commonName              CN            NID_commonName
 *   description             D             NID_description
 *   givenName               G             NID_givenName
 *   initials                I             NID_initials
 *   localityName            L             NID_localityName
 *   organizationName        O             NID_organizationName
 *   organizationalUnitName  OU            NID_organizationalUnitName
 *   stateOrProvinceName     ST            NID_stateOrProvinceName
 *   surname                 S             NID_surname
 *   title                   T             NID_title
 *   uniqueIdentifer         UID           NID_x500UniqueIdentifier
 *                                         (or NID_uniqueIdentifier, depending
 *                                         on OpenSSL version)
 *   email                   Email         NID_pkcs9_emailAddress
 */

static void tls_setup_cert_dn_environ(const char *env_prefix, X509_NAME *name) {
  register unsigned int i = 0;
  char *k, *v;

  for (i = 0; i < sk_X509_NAME_ENTRY_num(name->entries); i++) {
    X509_NAME_ENTRY *entry = sk_X509_NAME_ENTRY_value(name->entries, i);
    int nid = OBJ_obj2nid(entry->object);

    switch (nid) {
      case NID_countryName:
        k = pstrcat(session.pool, env_prefix, "C", NULL);
        v = pstrndup(session.pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(session.pool, k, v);
        break;

      case NID_commonName:
        k = pstrcat(session.pool, env_prefix, "CN", NULL);
        v = pstrndup(session.pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(session.pool, k, v);
        break;

      case NID_description:
        k = pstrcat(main_server->pool, env_prefix, "D", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      case NID_givenName:
        k = pstrcat(main_server->pool, env_prefix, "G", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      case NID_initials:
        k = pstrcat(main_server->pool, env_prefix, "I", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      case NID_localityName:
        k = pstrcat(main_server->pool, env_prefix, "L", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      case NID_organizationName:
        k = pstrcat(main_server->pool, env_prefix, "O", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      case NID_organizationalUnitName:
        k = pstrcat(main_server->pool, env_prefix, "OU", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      case NID_stateOrProvinceName:
        k = pstrcat(main_server->pool, env_prefix, "ST", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      case NID_surname:
        k = pstrcat(main_server->pool, env_prefix, "S", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      case NID_title:
        k = pstrcat(main_server->pool, env_prefix, "T", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

#if OPENSSL_VERSION_NUMBER >= 0x00907000L
      case NID_x500UniqueIdentifier:
#else
      case NID_uniqueIdentifier:
#endif
        k = pstrcat(main_server->pool, env_prefix, "UID", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      case NID_pkcs9_emailAddress:
        k = pstrcat(main_server->pool, env_prefix, "Email", NULL);
        v = pstrndup(main_server->pool, (const char *) entry->value->data,
          entry->value->length);
        pr_env_set(main_server->pool, k, v);
        break;

      default:
        break;
    }
  }
}

static void tls_setup_cert_environ(const char *env_prefix, X509 *cert) {
  char *data = NULL, *k, *v;
  long datalen = 0;
  BIO *bio = NULL;

  if (tls_opts & TLS_OPT_STD_ENV_VARS) {
    char buf[80] = {'\0'};
    ASN1_INTEGER *serial = X509_get_serialNumber(cert);

    memset(buf, '\0', sizeof(buf));
    snprintf(buf, sizeof(buf) - 1, "%lu", X509_get_version(cert) + 1);
    buf[sizeof(buf)-1] = '\0';

    k = pstrcat(main_server->pool, env_prefix, "M_VERSION", NULL);
    v = pstrdup(main_server->pool, buf);
    pr_env_set(main_server->pool, k, v);

    if (serial->length < 4) {
      memset(buf, '\0', sizeof(buf));
      snprintf(buf, sizeof(buf) - 1, "%lu", ASN1_INTEGER_get(serial));
      buf[sizeof(buf)-1] = '\0';

      k = pstrcat(main_server->pool, env_prefix, "M_SERIAL", NULL);
      v = pstrdup(main_server->pool, buf);
      pr_env_set(main_server->pool, k, v);

    } else {

      /* NOTE: actually, the number is printable, I'm just being lazy. This
       * case is much harder to deal with, and not really worth the effort.
       */
      tls_log("%s", "certificate serial number not printable");
    }

    k = pstrcat(main_server->pool, env_prefix, "S_DN", NULL);
    v = pstrdup(main_server->pool,
      tls_x509_name_oneline(X509_get_subject_name(cert)));
    pr_env_set(main_server->pool, k, v);

    tls_setup_cert_dn_environ(pstrcat(main_server->pool, env_prefix, "S_DN_",
      NULL), X509_get_subject_name(cert));

    k = pstrcat(main_server->pool, env_prefix, "I_DN", NULL);
    v = pstrdup(main_server->pool,
      tls_x509_name_oneline(X509_get_issuer_name(cert)));
    pr_env_set(main_server->pool, k, v);

    tls_setup_cert_dn_environ(pstrcat(main_server->pool, env_prefix, "I_DN_",
      NULL), X509_get_issuer_name(cert));

    tls_setup_cert_ext_environ(pstrcat(main_server->pool, env_prefix, "EXT_",
      NULL), cert);

    bio = BIO_new(BIO_s_mem());
    ASN1_TIME_print(bio, X509_get_notBefore(cert));
    datalen = BIO_get_mem_data(bio, &data);
    data[datalen] = '\0';

    k = pstrcat(main_server->pool, env_prefix, "V_START", NULL);
    v = pstrdup(main_server->pool, data);
    pr_env_set(main_server->pool, k, v);

    BIO_free(bio);

    bio = BIO_new(BIO_s_mem());
    ASN1_TIME_print(bio, X509_get_notAfter(cert));
    datalen = BIO_get_mem_data(bio, &data);
    data[datalen] = '\0';

    k = pstrcat(main_server->pool, env_prefix, "V_END", NULL);
    v = pstrdup(main_server->pool, data);
    pr_env_set(main_server->pool, k, v);

    BIO_free(bio);

    bio = BIO_new(BIO_s_mem());
    i2a_ASN1_OBJECT(bio, cert->cert_info->signature->algorithm);
    datalen = BIO_get_mem_data(bio, &data);
    data[datalen] = '\0';

    k = pstrcat(main_server->pool, env_prefix, "A_SIG", NULL);
    v = pstrdup(main_server->pool, data);
    pr_env_set(main_server->pool, k, v);

    BIO_free(bio);

    bio = BIO_new(BIO_s_mem());
    i2a_ASN1_OBJECT(bio, cert->cert_info->key->algor->algorithm);
    datalen = BIO_get_mem_data(bio, &data);
    data[datalen] = '\0';

    k = pstrcat(main_server->pool, env_prefix, "A_KEY", NULL);
    v = pstrdup(main_server->pool, data);
    pr_env_set(main_server->pool, k, v);

    BIO_free(bio);
  }

  bio = BIO_new(BIO_s_mem());
  PEM_write_bio_X509(bio, cert);
  datalen = BIO_get_mem_data(bio, &data);
  data[datalen] = '\0';

  k = pstrcat(main_server->pool, env_prefix, "CERT", NULL);
  v = pstrdup(main_server->pool, data);
  pr_env_set(main_server->pool, k, v);

  BIO_free(bio);
}

static void tls_setup_environ(SSL *ssl) {
  X509 *cert = NULL;
  STACK_OF(X509) *sk_cert_chain = NULL;
  char *k, *v;

  if (!(tls_opts & TLS_OPT_EXPORT_CERT_DATA) &&
      !(tls_opts & TLS_OPT_STD_ENV_VARS))
    return;

  if (tls_opts & TLS_OPT_STD_ENV_VARS) {
    SSL_CIPHER *cipher = NULL;
    SSL_SESSION *ssl_session = NULL;

    k = pstrdup(main_server->pool, "FTPS");
    v = pstrdup(main_server->pool, "1");
    pr_env_set(main_server->pool, k, v);

    k = pstrdup(main_server->pool, "TLS_PROTOCOL");
    v = pstrdup(main_server->pool, SSL_get_version(ssl));
    pr_env_set(main_server->pool, k, v);

    /* Process the SSL session-related environ variable. */
    ssl_session = SSL_get_session(ssl);
    if (ssl_session) {
      char buf[SSL_MAX_SSL_SESSION_ID_LENGTH*2+1];
      register unsigned int i = 0;

      /* Have to obtain a stringified session ID the hard way. */
      memset(buf, '\0', sizeof(buf));
      for (i = 0; i < ssl_session->session_id_length; i++) {
        snprintf(&(buf[i*2]), sizeof(buf) - (i*2) - 1, "%02X",
          ssl_session->session_id[i]);
      }
      buf[sizeof(buf)-1] = '\0';

      k = pstrdup(main_server->pool, "TLS_SESSION_ID");
      v = pstrdup(main_server->pool, buf);
      pr_env_set(main_server->pool, k, v);
    }

    /* Process the SSL cipher-related environ variables. */
    cipher = (SSL_CIPHER *) SSL_get_current_cipher(ssl);
    if (cipher) {
      char buf[10] = {'\0'};
      int cipher_bits_used = 0, cipher_bits_possible = 0;

      k = pstrdup(main_server->pool, "TLS_CIPHER");
      v = pstrdup(main_server->pool, SSL_CIPHER_get_name(cipher));
      pr_env_set(main_server->pool, k, v);

      cipher_bits_used = SSL_CIPHER_get_bits(cipher, &cipher_bits_possible);

      if (cipher_bits_used < 56) {
        k = pstrdup(main_server->pool, "TLS_CIPHER_EXPORT");
        v = pstrdup(main_server->pool, "1");
        pr_env_set(main_server->pool, k, v);
      }

      memset(buf, '\0', sizeof(buf));
      snprintf(buf, sizeof(buf), "%d", cipher_bits_possible);
      buf[sizeof(buf)-1] = '\0';

      k = pstrdup(main_server->pool, "TLS_CIPHER_KEYSIZE_POSSIBLE");
      v = pstrdup(main_server->pool, buf);
      pr_env_set(main_server->pool, k, v);

      memset(buf, '\0', sizeof(buf));
      snprintf(buf, sizeof(buf), "%d", cipher_bits_used);
      buf[sizeof(buf)-1] = '\0';

      k = pstrdup(main_server->pool, "TLS_CIPHER_KEYSIZE_USED");
      v = pstrdup(main_server->pool, buf);
      pr_env_set(main_server->pool, k, v);
    }

    k = pstrdup(main_server->pool, "TLS_LIBRARY_VERSION");
    v = pstrdup(main_server->pool, OPENSSL_VERSION_TEXT);
    pr_env_set(main_server->pool, k, v);
  }

  sk_cert_chain = SSL_get_peer_cert_chain(ssl);
  if (sk_cert_chain) {
    char *data = NULL;
    long datalen = 0;
    register unsigned int i = 0;
    BIO *bio = NULL;

    /* Adding TLS_CLIENT_CERT_CHAIN environ variables. */
    for (i = 0; i < sk_X509_num(sk_cert_chain); i++) {
      size_t klen = 256;
      k = pcalloc(main_server->pool, klen);
      snprintf(k, klen - 1, "%s%u", "TLS_CLIENT_CERT_CHAIN", i + 1);

      bio = BIO_new(BIO_s_mem());
      PEM_write_bio_X509(bio, sk_X509_value(sk_cert_chain, i));
      datalen = BIO_get_mem_data(bio, &data);
      data[datalen] = '\0';

      v = pstrdup(main_server->pool, data);

      pr_env_set(main_server->pool, k, v);

      BIO_free(bio);
    } 
  }

  /* Note: SSL_get_certificate() does NOT increment a reference counter,
   * so we do not call X509_free() on it.
   */
  cert = SSL_get_certificate(ssl);
  if (cert) {
    tls_setup_cert_environ("TLS_SERVER_", cert);

  } else {
    tls_log("unable to set server certificate environ variables: "
      "Server certificate unavailable");
  }

  cert = SSL_get_peer_certificate(ssl);
  if (cert) {
    tls_setup_cert_environ("TLS_CLIENT_", cert);
    X509_free(cert);

  } else {
    tls_log("unable to set client certificate environ variables: "
      "Client certificate unavailable");
  }

  return;
}

static int tls_verify_cb(int ok, X509_STORE_CTX *ctx) {
  config_rec *c;
  int verify_err = 0;

  /* We can configure the server to skip the peer's cert verification */
  if (!(tls_flags & TLS_SESS_VERIFY_CLIENT))
     return 1;

  c = find_config(main_server->conf, CONF_PARAM, "TLSVerifyOrder", FALSE);
  if (c) {
    register unsigned int i;

    for (i = 0; i < c->argc; i++) {
      char *mech = c->argv[i];

      if (strncasecmp(mech, "crl", 4) == 0) {
        ok = tls_verify_crl(ok, ctx);
        if (!ok) {
          break;
        }

      } else if (strncasecmp(mech, "ocsp", 5) == 0) {
        ok = tls_verify_ocsp(ok, ctx);
        if (!ok) {
          break;
        }
      }
    }

  } else {
    /* If no TLSVerifyOrder was specified, default to the old behavior of
     * always checking CRLs, if configured, and not paying attention to
     * any AIA attributes (i.e. no use of OCSP).
     */
    ok = tls_verify_crl(ok, ctx);
  }

  if (!ok) {
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
    int depth = X509_STORE_CTX_get_error_depth(ctx);
    verify_err = ctx->error;

    tls_log("error: unable to verify certificate at depth %d", depth);
    tls_log("error: cert subject: %s", tls_x509_name_oneline(
      X509_get_subject_name(cert)));
    tls_log("error: cert issuer: %s", tls_x509_name_oneline(
      X509_get_issuer_name(cert)));

    /* Catch a too long certificate chain here. */
    if (depth > tls_verify_depth)
      X509_STORE_CTX_set_error(ctx, X509_V_ERR_CERT_CHAIN_TOO_LONG);

    switch (ctx->error) {
      case X509_V_ERR_CERT_CHAIN_TOO_LONG:
      case X509_V_ERR_CERT_HAS_EXPIRED:
      case X509_V_ERR_CERT_REVOKED:
      case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
      case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
      case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
      case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
      case X509_V_ERR_APPLICATION_VERIFICATION:
        tls_log("client certificate failed verification: %s",
          X509_verify_cert_error_string(ctx->error));
        ok = 0;
        break;

      case X509_V_ERR_INVALID_PURPOSE: {
        register unsigned int i;
        int count = X509_PURPOSE_get_count();

        tls_log("client certificate failed verification: %s",
          X509_verify_cert_error_string(ctx->error));

        for (i = 0; i < count; i++) {
          X509_PURPOSE *purp = X509_PURPOSE_get0(i);
          tls_log("  purpose #%d: %s", i+1, X509_PURPOSE_get0_name(purp));
        }

        ok = 0;
        break;
      }

      default:
        tls_log("error verifying client certificate: [%d] %s",
          ctx->error, X509_verify_cert_error_string(ctx->error));
        ok = 0;
        break;
    }
  }

  if (ok) {
    pr_event_generate("mod_tls.verify-client", NULL);

  } else {
    pr_event_generate("mod_tls.verify-client-failed", &verify_err);
  }

  return ok;
}

/* This routine is (very much!) based on the work by Ralf S. Engelschall
 * <rse@engelshall.com>.  Comments by Ralf.
 */
static int tls_verify_crl(int ok, X509_STORE_CTX *ctx) {
  X509_OBJECT obj;
  X509_NAME *subject = NULL, *issuer = NULL;
  X509 *xs = NULL;
  X509_CRL *crl = NULL;
  X509_STORE_CTX store_ctx;
  int n, res;
  register int i = 0;

  /* Unless a revocation store for CRLs was created we cannot do any
   * CRL-based verification, of course.
   */
  if (!tls_crl_store) {
    return ok;
  }

  tls_log("CRL store present, checking client certificate against configured "
    "CRLs");

  /* Determine certificate ingredients in advance.
   */
  xs = X509_STORE_CTX_get_current_cert(ctx);

  subject = X509_get_subject_name(xs);
  pr_trace_msg(trace_channel, 15,
    "verifying cert: subject = '%s'", tls_x509_name_oneline(subject));

  issuer = X509_get_issuer_name(xs);
  pr_trace_msg(trace_channel, 15,
    "verifying cert: issuer = '%s'", tls_x509_name_oneline(issuer));

  /* OpenSSL provides the general mechanism to deal with CRLs but does not
   * use them automatically when verifying certificates, so we do it
   * explicitly here. We will check the CRL for the currently checked
   * certificate, if there is such a CRL in the store.
   *
   * We come through this procedure for each certificate in the certificate
   * chain, starting with the root-CA's certificate. At each step we've to
   * both verify the signature on the CRL (to make sure it's a valid CRL)
   * and its revocation list (to make sure the current certificate isn't
   * revoked).  But because to check the signature on the CRL we need the
   * public key of the issuing CA certificate (which was already processed
   * one round before), we've a little problem. But we can both solve it and
   * at the same time optimize the processing by using the following
   * verification scheme (idea and code snippets borrowed from the GLOBUS
   * project):
   *
   * 1. We'll check the signature of a CRL in each step when we find a CRL
   *    through the _subject_ name of the current certificate. This CRL
   *    itself will be needed the first time in the next round, of course.
   *    But we do the signature processing one round before this where the
   *    public key of the CA is available.
   *
   * 2. We'll check the revocation list of a CRL in each step when
   *    we find a CRL through the _issuer_ name of the current certificate.
   *    This CRLs signature was then already verified one round before.
   *
   * This verification scheme allows a CA to revoke its own certificate as
   * well, of course.
   */

  /* Try to retrieve a CRL corresponding to the _subject_ of
   * the current certificate in order to verify its integrity.
   */
  memset(&obj, 0, sizeof(obj));
#if OPENSSL_VERSION_NUMBER > 0x000907000L
  if (X509_STORE_CTX_init(&store_ctx, tls_crl_store, NULL, NULL) <= 0) {
    tls_log("error initializing CRL store context: %s", tls_get_errors());
    return ok;
  }
#else
  X509_STORE_CTX_init(&store_ctx, tls_crl_store, NULL, NULL);
#endif

  res = X509_STORE_get_by_subject(&store_ctx, X509_LU_CRL, subject, &obj);
  crl = obj.data.crl;

  if (res > 0 &&
      crl != NULL) {
    EVP_PKEY *pubkey;
    char buf[512];
    int len;
    BIO *b = BIO_new(BIO_s_mem());

    BIO_printf(b, "CA CRL: Issuer: ");
    X509_NAME_print(b, issuer, 0);

    BIO_printf(b, ", lastUpdate: ");
    ASN1_UTCTIME_print(b, crl->crl->lastUpdate);

    BIO_printf(b, ", nextUpdate: ");
    ASN1_UTCTIME_print(b, crl->crl->nextUpdate);

    len = BIO_read(b, buf, sizeof(buf) - 1);
    if (len >= sizeof(buf)) {
      len = sizeof(buf)-1;
    }
    buf[len] = '\0';

    BIO_free(b);

    tls_log("%s", buf);

    pubkey = X509_get_pubkey(xs);

    /* Verify the signature on this CRL */
    res = X509_CRL_verify(crl, pubkey);

    if (pubkey) {
      EVP_PKEY_free(pubkey);
    }

    if (res <= 0) {
      tls_log("invalid signature on CRL: %s", tls_get_errors());

      X509_STORE_CTX_set_error(ctx, X509_V_ERR_CRL_SIGNATURE_FAILURE);
      X509_OBJECT_free_contents(&obj);
      X509_STORE_CTX_cleanup(&store_ctx);
      return FALSE;
    }

    /* Check date of CRL to make sure it's not expired */
    i = X509_cmp_current_time(X509_CRL_get_nextUpdate(crl));
    if (i == 0) {
      tls_log("CRL has invalid nextUpdate field: %s", tls_get_errors());
      X509_STORE_CTX_set_error(ctx, X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD);
      X509_OBJECT_free_contents(&obj);
      X509_STORE_CTX_cleanup(&store_ctx);
      return FALSE;
    }

    if (i < 0) {
      /* XXX This is a bit draconian, rejecting all certificates if the CRL
       * has expired.  See also Bug#3216, about automatically reloading
       * the CRL file when it has expired.
       */
      tls_log("%s", "CRL is expired, revoking all certificates until an "
        "updated CRL is obtained");
      X509_STORE_CTX_set_error(ctx, X509_V_ERR_CRL_HAS_EXPIRED);
      X509_OBJECT_free_contents(&obj);
      X509_STORE_CTX_cleanup(&store_ctx);
      return FALSE;
    }

    X509_OBJECT_free_contents(&obj);
  }

  /* Try to retrieve a CRL corresponding to the _issuer_ of
   * the current certificate in order to check for revocation.
   */
  memset(&obj, 0, sizeof(obj));

  res = X509_STORE_get_by_subject(&store_ctx, X509_LU_CRL, issuer, &obj);
  crl = obj.data.crl;

  if (res > 0 &&
      crl != NULL) {

    /* Check if the current certificate is revoked by this CRL */
    n = sk_X509_REVOKED_num(X509_CRL_get_REVOKED(crl));

    for (i = 0; i < n; i++) {
      X509_REVOKED *revoked;
      ASN1_INTEGER *sn;

      revoked = sk_X509_REVOKED_value(X509_CRL_get_REVOKED(crl), i);
      sn = revoked->serialNumber;

      if (ASN1_INTEGER_cmp(sn, X509_get_serialNumber(xs)) == 0) {
        long serial = ASN1_INTEGER_get(sn);
        char *cp = tls_x509_name_oneline(issuer);

        tls_log("certificate with serial number %ld (0x%lX) revoked per CRL "
          "from issuer '%s'", serial, serial, cp ? cp : "(ERROR)");

        X509_STORE_CTX_set_error(ctx, X509_V_ERR_CERT_REVOKED);
        X509_OBJECT_free_contents(&obj);
        X509_STORE_CTX_cleanup(&store_ctx);
        return FALSE;
      }
    }

    X509_OBJECT_free_contents(&obj);
  }

  X509_STORE_CTX_cleanup(&store_ctx);
  return ok;
}

#if OPENSSL_VERSION_NUMBER > 0x000907000L
static int tls_verify_ocsp_url(X509_STORE_CTX *ctx, X509 *cert,
    const char *url) {
  BIO *conn;
  X509 *issuing_cert = NULL;
  X509_NAME *subj = NULL;
  const char *subj_name;
  char *host = NULL, *port = NULL, *uri = NULL;
  int ok = FALSE, res = 0 , use_ssl = 0, ocsp_status, ocsp_cert_status,
    ocsp_reason;
  OCSP_REQUEST *req = NULL;
  OCSP_CERTID *cert_id = NULL;
  OCSP_RESPONSE *resp = NULL;
  OCSP_BASICRESP *basic_resp = NULL;
  SSL_CTX *ocsp_ssl_ctx = NULL;
  ASN1_GENERALIZEDTIME *revtime, *thisupd, *nextupd;

  if (cert == NULL ||
      url == NULL) {
    return FALSE;
  }

  subj = X509_get_subject_name(cert);
  subj_name = tls_x509_name_oneline(subj);

  tls_log("checking OCSP URL '%s' for client cert '%s'", url, subj_name);

  if (OCSP_parse_url((char *) url, &host, &port, &uri, &use_ssl) != 1) {
    tls_log("error parsing OCSP URL '%s': %s", url, tls_get_errors());
    return FALSE;
  }

  /* XXX Need to check for NULL host, uri. */

  if (port == NULL) {
    if (use_ssl == 1) {
#ifdef OPENSSL_strdup
      port = OPENSSL_strdup("443");
#else
      port = OPENSSL_malloc(4);
      sstrncpy(port, "443", 3);
#endif /* OPENSSL_strdup */

    } else {
#ifdef OPENSSL_strdup
      port = OPENSSL_strdup("80");
#else
      port = OPENSSL_malloc(3);
      sstrncpy(port, "80", 2);
#endif /* OPENSSL_strdup */
    }
  }

  tls_log("connecting to OCSP responder at host '%s', port '%s', URI '%s'%s",
    host, port, uri, use_ssl ? ", using SSL/TLS" : "");

  /* Connect to the OCSP responder indicated */
  conn = BIO_new_connect(host);
  if (conn == NULL) {
    tls_log("error creating connection BIO: %s", tls_get_errors());

    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  BIO_set_conn_port(conn, port);

  /* If use_ssl is true, we need to a) create an SSL_CTX object to use, and
   * push it onto the BIO chain so that SSL/TLS actually happens.
   * When doing so, what version of SSL/TLS should we use?
   */
  if (use_ssl == 1) {
    BIO *ocsp_ssl_bio = NULL;

    /* Note: this code used openssl/apps/ocsp.c as a model. */
    ocsp_ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_set_mode(ocsp_ssl_ctx, SSL_MODE_AUTO_RETRY);

      ocsp_ssl_bio = BIO_new_ssl(ocsp_ssl_ctx, 1);
      BIO_push(ocsp_ssl_bio, conn);

    } else {
      tls_log("error allocating SSL_CTX object for OCSP verification: %s",
        tls_get_errors());
    }
  }

  res = BIO_do_connect(conn);
  if (res != 1) {
    tls_log("error connecting to OCSP URL '%s': %s", url, tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    /* XXX Should we give the client the benefit of the doubt, and allow
     * it to connect even though we can't talk to its OCSP responder?  Or
     * do we fail-close, and penalize the client if the OCSP responder is
     * down (e.g. for maintenance)?
     */

    return FALSE;
  }

  /* XXX Why are we querying the OCSP responder about the client cert's
   * issuing CA, rather than querying about the client cert itself?
   */

  res = X509_STORE_CTX_get1_issuer(&issuing_cert, ctx, cert);
  if (res != 1) {
    tls_log("error retrieving issuing cert for client cert '%s': %s",
      subj_name, tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  /* Note that the cert_id value will be freed when the request is freed. */
  cert_id = OCSP_cert_to_id(NULL, cert, issuing_cert);
  if (cert_id == NULL) {
    const char *issuer_subj_name = tls_x509_name_oneline(
      X509_get_subject_name(issuing_cert));

    tls_log("error converting client cert '%s' and its issuing cert '%s' "
      "to an OCSP cert ID: %s", subj_name, issuer_subj_name, tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  req = OCSP_REQUEST_new();
  if (req == NULL) {
    tls_log("unable to allocate OCSP request: %s", tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  if (OCSP_request_add0_id(req, cert_id) == NULL) {
    tls_log("error adding cert ID to OCSP request: %s", tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_REQUEST_free(req);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

# if 0
  /* XXX ideally we would set the requestor name to the subject name of the
   * cert configured via TLS{DSA,RSA}CertificateFile here.
   */
  if (OCSP_request_set1_name(req, /* server cert X509_NAME subj name */) != 1) {
    tls_log("error adding requestor name '%s' to OCSP request: %s",
      requestor_name, tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_REQUEST_free(req);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }
# endif

  res = OCSP_request_add1_nonce(req, NULL, 0);
  if (res != 1) {
    tls_log("error adding nonce to OCSP request: %s", tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_REQUEST_free(req);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  if (tls_opts & TLS_OPT_ENABLE_DIAGS) {
    BIO *bio;

    bio = BIO_new(BIO_s_mem());
    if (bio != NULL) {
      if (OCSP_REQUEST_print(bio, req, 0) == 1) {
        char *data = NULL;
        long datalen;

        datalen = BIO_get_mem_data(bio, &data);
        if (data != NULL) {
          data[datalen] = '\0';
          tls_log("sending OCSP request (%ld bytes):\n%s", datalen, data);
        }
      }

      BIO_free(bio);
    }
  }

  resp = OCSP_sendreq_bio(conn, uri, req);

  if (resp == NULL) {
    tls_log("error receiving response from OCSP responder at '%s': %s", url,
      tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_REQUEST_free(req);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  if (tls_opts & TLS_OPT_ENABLE_DIAGS) {
    BIO *bio;

    bio = BIO_new(BIO_s_mem());
    if (bio != NULL) {
      if (OCSP_RESPONSE_print(bio, resp, 0) == 1) {
        char *data = NULL;
        long datalen;

        datalen = BIO_get_mem_data(bio, &data);
        if (data != NULL) {
          data[datalen] = '\0';
          tls_log("received OCSP response (%ld bytes):\n%s", datalen, data);
        }
      }

      BIO_free(bio);
    }
  }

  tls_log("checking response from OCSP responder at URL '%s' for client cert "
    "'%s'", url, subj_name);

  basic_resp = OCSP_response_get1_basic(resp);
  if (basic_resp == NULL) {
    tls_log("error retrieving basic response from OCSP responder at '%s': %s",
      url, tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_RESPONSE_free(resp);
    OCSP_REQUEST_free(req);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  res = OCSP_check_nonce(req, basic_resp);
  if (res != 1) {
    tls_log("unable to use response from OCSP responder at '%s': bad nonce",
      url);

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_BASICRESP_free(basic_resp);
    OCSP_RESPONSE_free(resp);
    OCSP_REQUEST_free(req);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  res = OCSP_basic_verify(basic_resp, NULL, ctx->ctx, 0);
  if (res != 1) {
    tls_log("error verifying basic response from OCSP responder at '%s': %s",
      url, tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_REQUEST_free(req);
    OCSP_BASICRESP_free(basic_resp);
    OCSP_RESPONSE_free(resp);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  /* Now that we have verified the response, we can check the response status.
   * If we only looked at the status first, then a malicious responder
   * could be tricking us, e.g.:
   *
   *  http://www.thoughtcrime.org/papers/ocsp-attack.pdf
   */

  ocsp_status = OCSP_response_status(resp);
  if (ocsp_status != OCSP_RESPONSE_STATUS_SUCCESSFUL) {
    tls_log("unable to verify client cert '%s' via OCSP responder at '%s': "
      "response status '%s' (%d)", subj_name, url,
      OCSP_response_status_str(ocsp_status), ocsp_status);

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_REQUEST_free(req);
    OCSP_BASICRESP_free(basic_resp);
    OCSP_RESPONSE_free(resp);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    switch (ocsp_status) {
      case OCSP_RESPONSE_STATUS_MALFORMEDREQUEST:
      case OCSP_RESPONSE_STATUS_INTERNALERROR:
      case OCSP_RESPONSE_STATUS_SIGREQUIRED:
      case OCSP_RESPONSE_STATUS_UNAUTHORIZED:
      case OCSP_RESPONSE_STATUS_TRYLATER:
        /* XXX For now, for the above OCSP response reasons, give the client
         * the benefit of the doubt, since all of the above non-success
         * response codes indicate either a) an issue with the OCSP responder
         * outside of the client's control, or b) an issue with our OCSP
         * implementation (e.g. SIGREQUIRED, UNAUTHORIZED).
         */
        ok = TRUE;
        break;

      default:
        ok = FALSE;
    }

    return ok;
  }

  res = OCSP_resp_find_status(basic_resp, cert_id, &ocsp_cert_status,
    &ocsp_reason, &revtime, &thisupd, &nextupd);
  if (res != 1) {
    tls_log("unable to retrieve cert status from OCSP response: %s",
      tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_REQUEST_free(req);
    OCSP_BASICRESP_free(basic_resp);
    OCSP_RESPONSE_free(resp);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  /* Check the validity of the timestamps on the response. */
  res = OCSP_check_validity(thisupd, nextupd, TLS_OCSP_RESP_MAX_AGE_SECS, -1);
  if (res != 1) {
    tls_log("unable validate OCSP response timestamps: %s",
      tls_get_errors());

    if (ocsp_ssl_ctx != NULL) {
      SSL_CTX_free(ocsp_ssl_ctx);
    }

    OCSP_REQUEST_free(req);
    OCSP_BASICRESP_free(basic_resp);
    OCSP_RESPONSE_free(resp);
    X509_free(issuing_cert);
    BIO_free_all(conn);
    OPENSSL_free(host);
    OPENSSL_free(port);
    OPENSSL_free(uri);

    return FALSE;
  }

  tls_log("client cert '%s' has '%s' (%d) status according to OCSP responder "
    "at '%s'", subj_name, OCSP_cert_status_str(ocsp_cert_status),
    ocsp_cert_status, url);

  switch (ocsp_cert_status) {
    case V_OCSP_CERTSTATUS_GOOD:
      ok = TRUE;
      break;

    case V_OCSP_CERTSTATUS_REVOKED:
      tls_log("client cert '%s' has '%s' status due to: %s", subj_name,
        OCSP_cert_status_str(ocsp_status), OCSP_crl_reason_str(ocsp_reason));
      ok = FALSE;
      break;

    case V_OCSP_CERTSTATUS_UNKNOWN:
      /* If the client cert points to an OCSP responder which claims not to
       * know about the client cert, then we shouldn't trust that client
       * cert.  Otherwise, a client could present a cert pointing to an
       * OCSP responder which they KNOW won't know about the client cert,
       * and could then slip through the verification process.
       */
      ok = FALSE;
      break;

    default:
      ok = FALSE;
  }

  if (ocsp_ssl_ctx != NULL) {
    SSL_CTX_free(ocsp_ssl_ctx);
  }

  OCSP_REQUEST_free(req);
  OCSP_BASICRESP_free(basic_resp);
  OCSP_RESPONSE_free(resp);
  X509_free(issuing_cert);
  BIO_free_all(conn);
  OPENSSL_free(host);
  OPENSSL_free(port);
  OPENSSL_free(uri);

  return ok;
}
#endif

static int tls_verify_ocsp(int ok, X509_STORE_CTX *ctx) {
#if OPENSSL_VERSION_NUMBER > 0x000907000L
  register unsigned int i;
  X509 *cert;
  const char *subj;
  STACK_OF(ACCESS_DESCRIPTION) *descs;
  pool *tmp_pool = NULL;
  array_header *ocsp_urls = NULL;

  /* Set a default verification error here; it will be superceded as needed
   * later during the verification process.
   */
  X509_STORE_CTX_set_error(ctx, X509_V_ERR_APPLICATION_VERIFICATION);

  cert = X509_STORE_CTX_get_current_cert(ctx);
  if (cert == NULL) {
    return ok;
  }

  subj = tls_x509_name_oneline(X509_get_subject_name(cert));

  descs = X509_get_ext_d2i(cert, NID_info_access, NULL, NULL);
  if (descs == NULL) {
    tls_log("Client cert '%s' contained no AuthorityInfoAccess attribute, "
      "unable to verify via OCSP", subj);
    return ok;
  }

  for (i = 0; i < sk_ACCESS_DESCRIPTION_num(descs); i++) {
    ACCESS_DESCRIPTION *desc = sk_ACCESS_DESCRIPTION_value(descs, i);

    if (OBJ_obj2nid(desc->method) == NID_ad_OCSP) {
      /* Found an OCSP AuthorityInfoAccess attribute */

      if (desc->location->type != GEN_URI) {
        /* Not a valid URI, ignore it. */
        continue;
      }

      /* Add this URL to the list of OCSP URLs to check. */
      if (ocsp_urls == NULL) {
        tmp_pool = make_sub_pool(session.pool);
        ocsp_urls = make_array(tmp_pool, 1, sizeof(char *));
      }

      *((char **) push_array(ocsp_urls)) = pstrdup(tmp_pool,
        (char *) desc->location->d.uniformResourceIdentifier->data);
    }
  }

  if (ocsp_urls == NULL) {
    tls_log("found no OCSP URLs in AuthorityInfoAccess attribute for client "
      "cert '%s', unable to verify via OCSP", subj);
    AUTHORITY_INFO_ACCESS_free(descs);
    return ok;
  }

  tls_log("found %u OCSP %s in AuthorityInfoAccess attribute for client cert "
    "'%s'", ocsp_urls->nelts, ocsp_urls->nelts != 1 ? "URLs" : "URL", subj);

  /* Check each of the URLs. */
  for (i = 0; i < ocsp_urls->nelts; i++) {
    char *url = ((char **) ocsp_urls->elts)[i];

    ok = tls_verify_ocsp_url(ctx, cert, url);
    if (ok)
      break;
  }

  destroy_pool(tmp_pool);
  AUTHORITY_INFO_ACCESS_free(descs);

  return ok;
#else
  return ok;
#endif
}

static ssize_t tls_write(SSL *ssl, const void *buf, size_t len) {
  ssize_t count;

  count = SSL_write(ssl, buf, len);

  if (count < 0) {
    long err = SSL_get_error(ssl, count);

    /* write(2) returns only the generic error number -1 */
    count = -1;

    switch (err) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        /* Simulate an EINTR in case OpenSSL wants to write more. */
        errno = EINTR;
        break;

      default:
        tls_fatal_error(err, __LINE__);
        break;
    }
  }

  return count;
}

static char *tls_x509_name_oneline(X509_NAME *x509_name) {
  static char buf[1024] = {'\0'};

  /* If we are using OpenSSL 0.9.6 or newer, we want to use
   * X509_NAME_print_ex() instead of X509_NAME_oneline().
   */

#if OPENSSL_VERSION_NUMBER < 0x000906000L
  memset(&buf, '\0', sizeof(buf));
  return X509_NAME_oneline(x509_name, buf, sizeof(buf)-1);
#else

  /* Sigh...do it the hard way. */
  BIO *mem = BIO_new(BIO_s_mem());
  char *data = NULL;
  long datalen = 0;
  int ok;
   
  ok = X509_NAME_print_ex(mem, x509_name, 0, XN_FLAG_ONELINE);
  if (ok) {
    datalen = BIO_get_mem_data(mem, &data);

    if (data) {
      memset(&buf, '\0', sizeof(buf));

      if (datalen >= sizeof(buf)) {
        datalen = sizeof(buf)-1;
      }

      memcpy(buf, data, datalen);

      buf[datalen] = '\0';
      buf[sizeof(buf)-1] = '\0';

      BIO_free(mem);
      return buf;
    }
  }

  BIO_free(mem);
  return NULL;
#endif /* OPENSSL_VERSION_NUMBER >= 0x000906000 */
}

/* Session cache API */

struct tls_scache {
  struct tls_scache *next, *prev;

  const char *name;
  tls_sess_cache_t *cache;
};

static pool *tls_sess_cache_pool = NULL;
static struct tls_scache *tls_sess_caches = NULL;
static unsigned int tls_sess_ncaches = 0;

int tls_sess_cache_register(const char *name, tls_sess_cache_t *cache) {
  struct tls_scache *sc;

  if (name == NULL ||
      cache == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (tls_sess_cache_pool == NULL) {
    tls_sess_cache_pool = make_sub_pool(permanent_pool);
    pr_pool_tag(tls_sess_cache_pool, "TLS Session Cache API Pool");
  }

  /* Make sure this cache has not already been registered. */
  if (tls_sess_cache_get_cache(name) != NULL) {
    errno = EEXIST;
    return -1;
  }

  sc = pcalloc(tls_sess_cache_pool, sizeof(struct tls_scache)); 

  /* XXX Should this name string be dup'd from the tls_sess_cache_pool? */
  sc->name = name;
  cache->cache_name = pstrdup(tls_sess_cache_pool, name); 
  sc->cache = cache;

  if (tls_sess_caches) {
    sc->next = tls_sess_caches;

  } else {
    sc->next = NULL;
  }

  tls_sess_caches = sc;
  tls_sess_ncaches++;

  return 0;
}

int tls_sess_cache_unregister(const char *name) {
  struct tls_scache *sc;

  if (name == NULL) {
    errno = EINVAL;
    return -1;
  }

  for (sc = tls_sess_caches; sc; sc = sc->next) {
    if (strcmp(sc->name, name) == 0) {

      if (sc->prev) {
        sc->prev->next = sc->next;

      } else {
        /* If prev is NULL, this is the head of the list. */
        tls_sess_caches = sc->next;
      }

      if (sc->next) {
        sc->next->prev = sc->prev;
      }

      sc->next = sc->prev = NULL;
      tls_sess_ncaches--;

      /* If the session cache being unregistered is in use, update the
       * session-cache-in-use pointer.
       */
      if (sc->cache == tls_sess_cache) {
        tls_sess_cache_close();
        tls_sess_cache = NULL;
      }

      /* NOTE: a counter should be kept of the number of unregistrations,
       * as the memory for a registration is not freed on unregistration.
       */

      return 0;
    }
  }

  errno = ENOENT;
  return -1;
}

static tls_sess_cache_t *tls_sess_cache_get_cache(const char *name) {
  struct tls_scache *sc;

  if (name == NULL) {
    errno = EINVAL;
    return NULL;
  }

  for (sc = tls_sess_caches; sc; sc = sc->next) {
    if (strcmp(sc->name, name) == 0) {
      return sc->cache;
    }
  }

  errno = ENOENT;
  return NULL;
}

static long tls_sess_cache_get_cache_mode(void) {
  if (tls_sess_cache == NULL) {
    return 0;
  }

  return tls_sess_cache->cache_mode;
}

static int tls_sess_cache_open(char *info, long timeout) {
  int res;

  if (tls_sess_cache == NULL) {
    errno = ENOSYS;
    return -1;
  }

  res = (tls_sess_cache->open)(tls_sess_cache, info, timeout);
  return res;
}

static int tls_sess_cache_close(void) {
  int res;

  if (tls_sess_cache == NULL) {
    errno = ENOSYS;
    return -1;
  }

  res = (tls_sess_cache->close)(tls_sess_cache);
  return res;
}

#ifdef PR_USE_CTRLS
static int tls_sess_cache_clear(void) {
  int res;

  if (tls_sess_cache == NULL) {
    errno = ENOSYS;
    return -1;
  }

  res = (tls_sess_cache->clear)(tls_sess_cache);
  return res;
}

static int tls_sess_cache_remove(void) {
  int res;

  if (tls_sess_cache == NULL) {
    errno = ENOSYS;
    return -1;
  }

  res = (tls_sess_cache->remove)(tls_sess_cache);
  return res;
}

static void sess_cache_printf(void *ctrl, const char *fmt, ...) {
  char buf[PR_TUNABLE_BUFFER_SIZE];
  va_list msg;

  memset(buf, '\0', sizeof(buf));

  va_start(msg, fmt);
  vsnprintf(buf, sizeof(buf), fmt, msg);
  va_end(msg);

  buf[sizeof(buf)-1] = '\0';
  pr_ctrls_add_response(ctrl, "%s", buf);
}

static int tls_sess_cache_status(pr_ctrls_t *ctrl, int flags) {
  int res = 0;

  if (tls_sess_cache != NULL) {
    res = (tls_sess_cache->status)(tls_sess_cache, sess_cache_printf, ctrl,
      flags);
    return res;
  }

  pr_ctrls_add_response(ctrl, "No TLSSessionCache configured");
  return res;
}

static int tls_handle_clear(pr_ctrls_t *ctrl, int reqargc, char **reqargv) {
  int res;

  res = tls_sess_cache_clear();
  if (res < 0) {
    pr_ctrls_add_response(ctrl,
      "tls sesscache: error clearing session cache: %s", strerror(errno));

  } else {
    pr_ctrls_add_response(ctrl, "tls sesscache: cleared %d %s from '%s' "
      "session cache", res, res != 1 ? "sessions" : "session",
      tls_sess_cache->cache_name);
    res = 0;
  }

  return res;
}

static int tls_handle_info(pr_ctrls_t *ctrl, int reqargc, char **reqargv) {
  int flags = 0, optc, res;
  const char *opts = "v";

  pr_getopt_reset();

  while ((optc = getopt(reqargc, reqargv, opts)) != -1) {
    switch (optc) {
      case 'v':
        flags = TLS_SESS_CACHE_STATUS_FL_SHOW_SESSIONS;
        break;

      case '?':
        pr_ctrls_add_response(ctrl,
          "tls sesscache: unsupported parameter: '%s'", reqargv[1]);
        return -1;
    }
  }

  res = tls_sess_cache_status(ctrl, flags);
  if (res < 0) {
    pr_ctrls_add_response(ctrl,
      "tls sesscache: error obtaining session cache status: %s",
      strerror(errno));

  } else {
    res = 0;
  }

  return res;
}

static int tls_handle_remove(pr_ctrls_t *ctrl, int reqargc, char **reqargv) {
  int res;

  res = tls_sess_cache_remove();
  if (res < 0) {
    pr_ctrls_add_response(ctrl,
      "tls sesscache: error removing session cache: %s", strerror(errno));

  } else {
    pr_ctrls_add_response(ctrl, "tls sesscache: removed '%s' session cache",
      tls_sess_cache->cache_name);
    res = 0;
  }

  return res;
}

static int tls_handle_sesscache(pr_ctrls_t *ctrl, int reqargc, char **reqargv) {

  /* Sanity check */
  if (reqargc == 0 ||
      reqargv == NULL) {
    pr_ctrls_add_response(ctrl, "tls sesscache: missing required parameters");
    return -1;
  }

  if (strncmp(reqargv[0], "info", 5) == 0) {

    /* Check the ACLs. */
    if (!pr_ctrls_check_acl(ctrl, tls_acttab, "info")) {
      pr_ctrls_add_response(ctrl, "access denied");
      return -1;
    }

    return tls_handle_info(ctrl, reqargc, reqargv);

  } else if (strncmp(reqargv[0], "clear", 6) == 0) {

    /* Check the ACLs. */
    if (!pr_ctrls_check_acl(ctrl, tls_acttab, "clear")) {
      pr_ctrls_add_response(ctrl, "access denied");
      return -1;
    }

    return tls_handle_clear(ctrl, reqargc, reqargv);

  } else if (strncmp(reqargv[0], "remove", 7) == 0) {

    /* Check the ACLs. */
    if (!pr_ctrls_check_acl(ctrl, tls_acttab, "remove")) {
      pr_ctrls_add_response(ctrl, "access denied");
      return -1;
    }

    return tls_handle_remove(ctrl, reqargc, reqargv);
  }

  pr_ctrls_add_response(ctrl, "tls sesscache: unknown sesscache action: '%s'",
    reqargv[0]);
  return -1;
}

/* Our main ftpdctl action handler */
static int tls_handle_tls(pr_ctrls_t *ctrl, int reqargc, char **reqargv) {

  /* Sanity check */
  if (reqargc == 0 ||
      reqargv == NULL) {
    pr_ctrls_add_response(ctrl, "tls: missing required parameters");
    return -1;
  }

  if (strncmp(reqargv[0], "sesscache", 10) == 0) {

    /* Check the ACLs. */
    if (!pr_ctrls_check_acl(ctrl, tls_acttab, "sesscache")) {
      pr_ctrls_add_response(ctrl, "access denied");
      return -1;
    }

    return tls_handle_sesscache(ctrl, --reqargc, ++reqargv);
  }

  pr_ctrls_add_response(ctrl, "tls: unknown tls action: '%s'", reqargv[0]);
  return -1;
}
#endif

static int tls_sess_cache_add_sess_cb(SSL *ssl, SSL_SESSION *sess) {
  unsigned char *sess_id;
  unsigned int sess_id_len;
  int res;
  long expires;

  if (tls_sess_cache == NULL) {
    tls_log("unable to add session to session cache: %s", strerror(ENOSYS));

    SSL_SESSION_free(sess);
    return 1;
  }

  SSL_set_timeout(sess, tls_sess_cache->cache_timeout);

#if OPENSSL_VERSION_NUMBER > 0x000908000L
  sess_id = (unsigned char *) SSL_SESSION_get_id(sess, &sess_id_len);
#else
  /* XXX Directly accessing these fields cannot be a Good Thing. */
  sess_id = sess->session_id;
  sess_id_len = sess->session_id_length;
#endif

  /* The expiration timestamp stored in the session cache is the
   * Unix epoch time, not an interval.
   */
  expires = SSL_SESSION_get_time(sess) + tls_sess_cache->cache_timeout;

  res = (tls_sess_cache->add)(tls_sess_cache, sess_id, sess_id_len, expires,
    sess);
  if (res < 0) {
    long cache_mode;

    tls_log("error adding session to '%s' cache: %s",
      tls_sess_cache->cache_name, strerror(errno));

    cache_mode = tls_sess_cache_get_cache_mode();
#ifdef SSL_SESS_CACHE_NO_INTERNAL
    if (cache_mode & SSL_SESS_CACHE_NO_INTERNAL) {
      /* Call SSL_SESSION_free() here, and return 1.  We told OpenSSL that we
       * are the only cache, so failing to call SSL_SESSION_free() could
       * result in a memory leak.
       */ 
      SSL_SESSION_free(sess);
      return 1;
    }
#endif /* !SSL_SESS_CACHE_NO_INTERNAL */
  }

  /* Return zero to indicate to OpenSSL that we have not called
   * SSL_SESSION_free().
   */
  return 0;
}

static SSL_SESSION *tls_sess_cache_get_sess_cb(SSL *ssl,
    unsigned char *sess_id, int sess_id_len, int *do_copy) {
  SSL_SESSION *sess;

  /* Indicate to OpenSSL that the ref count should not be incremented
   * by setting the do_copy pointer to zero.
   */
  *do_copy = 0;

  /* The actual session_id_length field in the OpenSSL SSL_SESSION struct
   * is unsigned, not signed.  But for some reason, the expected callback
   * signature uses 'int', not 'unsigned int'.  Hopefully the implicit
   * cast below (our callback uses 'unsigned int') won't cause problems.
   * Just to be sure, check if OpenSSL is giving us a negative ID length.
   */
  if (sess_id_len <= 0) {
    tls_log("OpenSSL invoked SSL session cache 'get' callback with session "
      "ID length %d, returning null", sess_id_len);
    return NULL;
  }

  if (tls_sess_cache == NULL) {
    tls_log("unable to get session from session cache: %s", strerror(ENOSYS));
    return NULL;
  }

  sess = (tls_sess_cache->get)(tls_sess_cache, sess_id, sess_id_len);
  if (sess == NULL) {
    tls_log("error retrieving session from '%s' cache: %s",
      tls_sess_cache->cache_name, strerror(errno));
  }

  return sess;
}

static void tls_sess_cache_delete_sess_cb(SSL_CTX *ctx, SSL_SESSION *sess) {
  unsigned char *sess_id;
  unsigned int sess_id_len;
  int res;

  if (tls_sess_cache == NULL) {
    tls_log("unable to remove session from session cache: %s",
      strerror(ENOSYS));
    return;
  }

#if OPENSSL_VERSION_NUMBER > 0x000908000L
  sess_id = (unsigned char *) SSL_SESSION_get_id(sess, &sess_id_len);
#else
  /* XXX Directly accessing these fields cannot be a Good Thing. */
  sess_id = sess->session_id;
  sess_id_len = sess->session_id_length;
#endif

  res = (tls_sess_cache->delete)(tls_sess_cache, sess_id, sess_id_len);
  if (res < 0) {
    tls_log("error removing session from '%s' cache: %s",
      tls_sess_cache->cache_name, strerror(errno));
  }

  return;
}

/* NetIO callbacks
 */

static void tls_netio_abort_cb(pr_netio_stream_t *nstrm) {
  nstrm->strm_flags |= PR_NETIO_SESS_ABORT;
}

static int tls_netio_close_cb(pr_netio_stream_t *nstrm) {
  int res = 0;
  SSL *ssl = NULL;

  ssl = pr_table_get(nstrm->notes, TLS_NETIO_NOTE, NULL);
  if (ssl != NULL) {
    if (nstrm->strm_type == PR_NETIO_STRM_CTRL &&
        nstrm->strm_mode == PR_NETIO_IO_WR) {
      tls_end_sess(ssl, nstrm->strm_type, 0);
      pr_table_remove(tls_ctrl_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
      pr_table_remove(tls_ctrl_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
      tls_ctrl_netio = NULL;
      tls_flags &= ~TLS_SESS_ON_CTRL;
    }

    if (nstrm->strm_type == PR_NETIO_STRM_DATA &&
        nstrm->strm_mode == PR_NETIO_IO_WR) {
      pr_table_remove(tls_data_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
      pr_table_remove(tls_data_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
      tls_data_netio = NULL;
      tls_flags &= ~TLS_SESS_ON_DATA;
    }
  }

  res = close(nstrm->strm_fd);
  nstrm->strm_fd = -1;

  return res;
}

static pr_netio_stream_t *tls_netio_open_cb(pr_netio_stream_t *nstrm, int fd,
    int mode) {
  nstrm->strm_fd = fd;
  nstrm->strm_mode = mode;

  /* Cache a pointer to this stream. */
  if (nstrm->strm_type == PR_NETIO_STRM_CTRL) {
    /* Note: We need to make this more generalized, so that mod_tls can be
     * used to open multiple different read/write control streams.  To do
     * so, we need a small (e.g. 4) array of pr_netio_stream_t pointers
     * for both read and write streams, and to iterate through them.  Need
     * iterate through and set strm_data appropriately in tls_accept(), too. 
     *
     * This will needed to support FTPS connections to backend servers from
     * mod_proxy, for example.
     */
    if (nstrm->strm_mode == PR_NETIO_IO_RD) {
      if (tls_ctrl_rd_nstrm == NULL) {
        tls_ctrl_rd_nstrm = nstrm;
      }
    }

    if (nstrm->strm_mode == PR_NETIO_IO_WR) {
      if (tls_ctrl_wr_nstrm == NULL) {
        tls_ctrl_wr_nstrm = nstrm;
      }
    }

  } else if (nstrm->strm_type == PR_NETIO_STRM_DATA) {
    if (nstrm->strm_mode == PR_NETIO_IO_RD) {
      tls_data_rd_nstrm = nstrm;
    }

    if (nstrm->strm_mode == PR_NETIO_IO_WR) {
      tls_data_wr_nstrm = nstrm;
    }

    /* Note: from the FTP-TLS Draft 9.2:
     * 
     *  It is quite reasonable for the server to insist that the data
     *  connection uses a TLS cached session.  This might be a cache of a
     *  previous data connection or of the control connection.  If this is
     *  the reason for the refusal to allow the data transfer then the
     *  '522' reply should indicate this.
     * 
     * and, from 10.4:
     *   
     *   If a server needs to have the connection protected then it will
     *   reply to the STOR/RETR/NLST/... command with a '522' indicating
     *   that the current state of the data connection protection level is
     *   not sufficient for that data transfer at that time.
     *
     * This points out the need for a module to be able to influence
     * command response codes in a more flexible manner...
     */
  }

  return nstrm;
}

static int tls_netio_poll_cb(pr_netio_stream_t *nstrm) {
  fd_set rfds, wfds;
  struct timeval tval;

  FD_ZERO(&rfds);
  FD_ZERO(&wfds);

  if (nstrm->strm_mode == PR_NETIO_IO_RD) {
    FD_SET(nstrm->strm_fd, &rfds);

  } else {
    FD_SET(nstrm->strm_fd, &wfds);
  }

  tval.tv_sec = (nstrm->strm_flags & PR_NETIO_SESS_INTR) ?
    nstrm->strm_interval : 10;
  tval.tv_usec = 0;

  return select(nstrm->strm_fd + 1, &rfds, &wfds, NULL, &tval);
}

static int tls_netio_postopen_cb(pr_netio_stream_t *nstrm) {

  /* If this is a data stream, and it's for writing, and TLS is required,
   * then do a TLS handshake.
   */

  if (nstrm->strm_type == PR_NETIO_STRM_DATA &&
      nstrm->strm_mode == PR_NETIO_IO_WR) {

    /* Enforce the "data" part of TLSRequired, if configured. */
    if (tls_required_on_data == 1 ||
        (tls_flags & TLS_SESS_NEED_DATA_PROT)) {
      SSL *ssl = NULL;

      ssl = pr_table_get(nstrm->notes, TLS_NETIO_NOTE, NULL);

      /* XXX How to force 421 response code for failed secure FXP/SSCN? */

      /* Directory listings (LIST, MLSD, NLST) are ALWAYS handled in server
       * mode, regardless of SSCN mode.
       */
      if (session.curr_cmd_id == PR_CMD_LIST_ID ||
          session.curr_cmd_id == PR_CMD_MLSD_ID ||
          session.curr_cmd_id == PR_CMD_NLST_ID ||
          tls_sscn_mode == TLS_SSCN_MODE_SERVER) {
        X509 *ctrl_cert = NULL, *data_cert = NULL;

        tls_log("%s", "starting TLS negotiation on data connection");
        tls_data_need_init_handshake = TRUE;
        if (tls_accept(session.d, TRUE) < 0) {
          tls_log("%s",
            "unable to open data connection: TLS negotiation failed");
          session.d->xerrno = EPERM;
          return -1;
        }

        /* Make sure that the certificate used, if any, for this data channel
         * handshake is the same as that used for the control channel handshake.
         * This may be too strict of a requirement, though.
         */
        ctrl_cert = SSL_get_peer_certificate(ctrl_ssl);
        data_cert = SSL_get_peer_certificate(ssl);

        if (ctrl_cert != NULL &&
            data_cert != NULL) {
          if (X509_cmp(ctrl_cert, data_cert)) {
            X509_free(ctrl_cert);
            X509_free(data_cert);

            /* Properly shutdown the SSL session. */
            tls_end_sess(ssl, nstrm->strm_type, 0);
            pr_table_remove(tls_data_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
            pr_table_remove(tls_data_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);

            tls_log("%s", "unable to open data connection: control/data "
              "certificate mismatch");

            session.d->xerrno = EPERM;
            return -1;
          }

          if (ctrl_cert) {
            X509_free(ctrl_cert);
          }

          if (data_cert) {
            X509_free(data_cert);
          }
        }

      } else if (tls_sscn_mode == TLS_SSCN_MODE_CLIENT) {
        tls_log("%s", "making TLS connection for data connection");
        if (tls_connect(session.d) < 0) {
          tls_log("%s",
            "unable to open data connection: TLS connection failed");
          session.d->xerrno = EPERM;
          return -1;
        }
     }

#if OPENSSL_VERSION_NUMBER < 0x0090702fL
      /* Make sure blinding is turned on. (For some reason, this only seems
       * to be allowed on SSL objects, not on SSL_CTX objects.  Bummer).
       */
      tls_blinding_on(ssl);
#endif

      tls_flags |= TLS_SESS_ON_DATA;
    }
  }

  return 0;
}

static int tls_netio_read_cb(pr_netio_stream_t *nstrm, char *buf,
    size_t buflen) {
  SSL *ssl;

  ssl = pr_table_get(nstrm->notes, TLS_NETIO_NOTE, NULL);
  if (ssl != NULL) {
    BIO *rbio, *wbio;
    int bread = 0, bwritten = 0;
    ssize_t res = 0;
    unsigned long rbio_rbytes, rbio_wbytes, wbio_rbytes, wbio_wbytes;

    rbio = SSL_get_rbio(ssl);
    rbio_rbytes = BIO_number_read(rbio);
    rbio_wbytes = BIO_number_written(rbio);

    wbio = SSL_get_wbio(ssl);
    wbio_rbytes = BIO_number_read(wbio);
    wbio_wbytes = BIO_number_written(wbio);

    res = tls_read(ssl, buf, buflen);

    bread = (BIO_number_read(rbio) - rbio_rbytes) +
      (BIO_number_read(wbio) - wbio_rbytes);
    bwritten = (BIO_number_written(rbio) - rbio_wbytes) +
      (BIO_number_written(wbio) - wbio_wbytes);

    /* Manually update session.total_raw_in with the difference between
     * the raw bytes read in versus the non-SSL bytes read in, in order to
     * have %I be accurately represented for the raw traffic.
     */
    session.total_raw_in += (bread - res);

    /* Manually update session.total_raw_out, in order to have %O be
     * accurately represented for the raw traffic.
     */
    if (bwritten > 0) {
      session.total_raw_out += bwritten;
    }

    return res;
  }

  return read(nstrm->strm_fd, buf, buflen);
}

static pr_netio_stream_t *tls_netio_reopen_cb(pr_netio_stream_t *nstrm, int fd,
    int mode) {

  if (nstrm->strm_fd != -1) {
    close(nstrm->strm_fd);
  }

  nstrm->strm_fd = fd;
  nstrm->strm_mode = mode;

  /* NOTE: a no-op? */
  return nstrm;
}

static int tls_netio_shutdown_cb(pr_netio_stream_t *nstrm, int how) {

  if (how == 1 ||
      how == 2) {
    /* Closing this stream for writing; we need to send the 'close_notify'
     * alert first, so that the client knows, at the application layer,
     * that the SSL/TLS session is shutting down.
     */

    if (nstrm->strm_mode == PR_NETIO_IO_WR &&
        (nstrm->strm_type == PR_NETIO_STRM_CTRL ||
         nstrm->strm_type == PR_NETIO_STRM_DATA)) {
      SSL *ssl;

      ssl = pr_table_get(nstrm->notes, TLS_NETIO_NOTE, NULL);
      if (ssl != NULL) {
        BIO *rbio, *wbio;
        int bread = 0, bwritten = 0;
        unsigned long rbio_rbytes, rbio_wbytes, wbio_rbytes, wbio_wbytes;

        rbio = SSL_get_rbio(ssl);
        rbio_rbytes = BIO_number_read(rbio);
        rbio_wbytes = BIO_number_written(rbio);

        wbio = SSL_get_wbio(ssl);
        wbio_rbytes = BIO_number_read(wbio);
        wbio_wbytes = BIO_number_written(wbio);

        if (!(SSL_get_shutdown(ssl) & SSL_SENT_SHUTDOWN)) {
          /* We haven't sent a 'close_notify' alert yet; do so now. */
          SSL_shutdown(ssl);
        }

        bread = (BIO_number_read(rbio) - rbio_rbytes) +
          (BIO_number_read(wbio) - wbio_rbytes);
        bwritten = (BIO_number_written(rbio) - rbio_wbytes) +
          (BIO_number_written(wbio) - wbio_wbytes);

        /* Manually update session.total_raw_in/out, in order to have %I/%O be
         * accurately represented for the raw traffic.
         */
        if (bread > 0) {
          session.total_raw_in += bread;
        }

        if (bwritten > 0) {
          session.total_raw_out += bwritten;
        }
      }
    }
  }

  return shutdown(nstrm->strm_fd, how);
}

static int tls_netio_write_cb(pr_netio_stream_t *nstrm, char *buf,
    size_t buflen) {
  SSL *ssl;

  ssl = pr_table_get(nstrm->notes, TLS_NETIO_NOTE, NULL);
  if (ssl != NULL) {
    BIO *rbio, *wbio;
    int bread = 0, bwritten = 0;
    ssize_t res = 0;
    unsigned long rbio_rbytes, rbio_wbytes, wbio_rbytes, wbio_wbytes;

    rbio = SSL_get_rbio(ssl);
    rbio_rbytes = BIO_number_read(rbio);
    rbio_wbytes = BIO_number_written(rbio);

    wbio = SSL_get_wbio(ssl);
    wbio_rbytes = BIO_number_read(wbio);
    wbio_wbytes = BIO_number_written(wbio);

#if OPENSSL_VERSION_NUMBER > 0x000907000L
    if (tls_data_renegotiate_limit &&
        session.xfer.total_bytes >= tls_data_renegotiate_limit

#if OPENSSL_VERSION_NUMBER >= 0x009080cfL
        /* In OpenSSL-0.9.8l and later, SSL session renegotiations
         * (both client- and server-initiated) are automatically disabled.
         * Unless the admin explicitly configured support for
         * client-initiated renegotations via the AllowClientRenegotiations
         * TLSOption, we can't request renegotiations ourselves.
         */
        && (tls_opts & TLS_OPT_ALLOW_CLIENT_RENEGOTIATIONS)
#endif
      ) {

      tls_flags |= TLS_SESS_DATA_RENEGOTIATING;

      tls_log("requesting TLS renegotiation on data channel "
        "(%" PR_LU " KB data limit)",
        (pr_off_t) (tls_data_renegotiate_limit / 1024));
      SSL_renegotiate(ssl);
      /* SSL_do_handshake(ssl); */

      pr_timer_add(tls_renegotiate_timeout, -1, &tls_module,
        tls_renegotiate_timeout_cb, "SSL/TLS renegotiation");
    }
#endif

    res = tls_write(ssl, buf, buflen);

    bread = (BIO_number_read(rbio) - rbio_rbytes) +
      (BIO_number_read(wbio) - wbio_rbytes);
    bwritten = (BIO_number_written(rbio) - rbio_wbytes) +
      (BIO_number_written(wbio) - wbio_wbytes);

    /* Manually update session.total_raw_in, in order to have %I be
     * accurately represented for the raw traffic.
     */
    if (bread > 0) {
      session.total_raw_in += bread;
    }

    /* Manually update session.total_raw_out with the difference between
     * the raw bytes written out versus the non-SSL bytes written out,
     * in order to have %) be accurately represented for the raw traffic.
     */
    session.total_raw_out += (bwritten - res);

    return res;
  }

  return write(nstrm->strm_fd, buf, buflen);
}

static void tls_netio_install_ctrl(void) {
  pr_netio_t *netio;

  if (tls_ctrl_netio) {
    /* If we already have our ctrl netio, then it's been registered, and
     * we don't need to do anything more.
     */
    return;
  }

  tls_ctrl_netio = netio = pr_alloc_netio2(permanent_pool, &tls_module);

  netio->abort = tls_netio_abort_cb;
  netio->close = tls_netio_close_cb;
  netio->open = tls_netio_open_cb;
  netio->poll = tls_netio_poll_cb;
  netio->postopen = tls_netio_postopen_cb;
  netio->read = tls_netio_read_cb;
  netio->reopen = tls_netio_reopen_cb;
  netio->shutdown = tls_netio_shutdown_cb;
  netio->write = tls_netio_write_cb;

  pr_unregister_netio(PR_NETIO_STRM_CTRL);

  if (pr_register_netio(netio, PR_NETIO_STRM_CTRL) < 0) {
    pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION ": error registering netio: %s",
      strerror(errno));
  }
}

static void tls_netio_install_data(void) {
  pr_netio_t *netio = tls_data_netio ? tls_data_netio :
    (tls_data_netio = pr_alloc_netio2(session.pool ? session.pool :
    permanent_pool, &tls_module));

  netio->abort = tls_netio_abort_cb;
  netio->close = tls_netio_close_cb;
  netio->open = tls_netio_open_cb;
  netio->poll = tls_netio_poll_cb;
  netio->postopen = tls_netio_postopen_cb;
  netio->read = tls_netio_read_cb;
  netio->reopen = tls_netio_reopen_cb;
  netio->shutdown = tls_netio_shutdown_cb;
  netio->write = tls_netio_write_cb;

  pr_unregister_netio(PR_NETIO_STRM_DATA);

  if (pr_register_netio(netio, PR_NETIO_STRM_DATA) < 0) {
    pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION ": error registering netio: %s",
      strerror(errno));
  }
}

/* Logging functions
 */

static void tls_closelog(void) {

  /* Sanity check */
  if (tls_logfd != -1) {
    close(tls_logfd);
    tls_logfd = -1;
    tls_logname = NULL;
  }

  return;
}

int tls_log(const char *fmt, ...) {
  va_list msg;
  int res;

  /* Sanity check */
  if (!tls_logname)
    return 0;

  va_start(msg, fmt);
  res = pr_log_vwritefile(tls_logfd, MOD_TLS_VERSION, fmt, msg);
  va_end(msg);

  return res;
}

static int tls_openlog(void) {
  int res = 0, xerrno;

  /* Sanity checks */
  tls_logname = get_param_ptr(main_server->conf, "TLSLog", FALSE);
  if (tls_logname == NULL)
    return 0;

  if (strncasecmp(tls_logname, "none", 5) == 0) {
    tls_logname = NULL;
    return 0;
  }

  pr_signals_block();
  PRIVS_ROOT
  res = pr_log_openfile(tls_logname, &tls_logfd, PR_LOG_SYSTEM_MODE);
  xerrno = errno;
  PRIVS_RELINQUISH
  pr_signals_unblock();

  errno = xerrno;
  return res;
}

/* Authentication handlers
 */

/* This function does the main authentication work, and is called in the
 * normal course of events:
 *
 *   cmd->argv[0]: user name
 *   cmd->argv[1]: cleartext password
 */
MODRET tls_authenticate(cmd_rec *cmd) {
  if (!tls_engine) {
    return PR_DECLINED(cmd);
  }

  /* Possible authentication combinations:
   *
   *  TLS handshake + passwd (default)
   *  TLS handshake + .tlslogin (passwd ignored)
   */

  if (tls_flags & TLS_SESS_ON_CTRL) {
    config_rec *c;

    if (tls_opts & TLS_OPT_ALLOW_DOT_LOGIN) {
      if (tls_dotlogin_allow(cmd->argv[0])) {
        tls_log("TLS/X509 .tlslogin check successful for user '%s'",
          cmd->argv[0]);
        pr_log_auth(PR_LOG_NOTICE, "USER %s: TLS/X509 .tlslogin authentication "
          "successful", cmd->argv[0]);
        session.auth_mech = "mod_tls.c";
        return mod_create_data(cmd, (void *) PR_AUTH_RFC2228_OK);

      } else {
        tls_log("TLS/X509 .tlslogin check failed for user '%s'", cmd->argv[0]);
      }
    }

    c = find_config(main_server->conf, CONF_PARAM, "TLSUserName", FALSE);
    if (c != NULL) {
      if (tls_cert_to_user(cmd->argv[0], c->argv[0])) {
        tls_log("TLS/X509 TLSUserName '%s' check successful for user '%s'",
          (char *) c->argv[0], (char *) cmd->argv[0]);
        pr_log_auth(PR_LOG_NOTICE,
          "USER %s: TLS/X509 TLSUserName authentication successful",
          cmd->argv[0]);
        session.auth_mech = "mod_tls.c";
        return mod_create_data(cmd, (void *) PR_AUTH_RFC2228_OK);

      } else {
        tls_log("TLS/X509 TLSUserName '%s' check failed for user '%s'",
          (char *) c->argv[0], (char *) cmd->argv[0]);
      }
    }
  }

  return PR_DECLINED(cmd);
}

/* This function is called only when UserPassword is involved, used to
 * override the configured password for a user.
 *
 *  cmd->argv[0]: hashed password (from proftpd.conf)
 *  cmd->argv[1]: user name
 *  cmd->argv[2]: cleartext password
 */
MODRET tls_auth_check(cmd_rec *cmd) {
  if (!tls_engine)
    return PR_DECLINED(cmd);

  /* Possible authentication combinations:
   *
   *  TLS handshake + passwd (default)
   *  TLS handshake + .tlslogin (passwd ignored)
   */

  if (tls_flags & TLS_SESS_ON_CTRL) {
    config_rec *c;

    if (tls_opts & TLS_OPT_ALLOW_DOT_LOGIN) {
      if (tls_dotlogin_allow(cmd->argv[1])) {
        tls_log("TLS/X509 .tlslogin check successful for user '%s'",
          (char *) cmd->argv[0]);
        pr_log_auth(PR_LOG_NOTICE, "USER %s: TLS/X509 .tlslogin authentication "
          "successful", cmd->argv[1]);
        session.auth_mech = "mod_tls.c";
        return mod_create_data(cmd, (void *) PR_AUTH_RFC2228_OK);

      } else {
        tls_log("TLS/X509 .tlslogin check failed for user '%s'",
          (char *) cmd->argv[1]);
      }
    }

    c = find_config(main_server->conf, CONF_PARAM, "TLSUserName", FALSE);
    if (c != NULL) {
      if (tls_cert_to_user(cmd->argv[0], c->argv[0])) {
        tls_log("TLS/X509 TLSUserName '%s' check successful for user '%s'",
          (char *) c->argv[0], (char *) cmd->argv[0]);
        pr_log_auth(PR_LOG_NOTICE,
          "USER %s: TLS/X509 TLSUserName authentication successful",
          cmd->argv[0]);
        session.auth_mech = "mod_tls.c";
        return mod_create_data(cmd, (void *) PR_AUTH_RFC2228_OK);

      } else {
        tls_log("TLS/X509 TLSUserName '%s' check failed for user '%s'",
          (char *) c->argv[0], (char *) cmd->argv[0]);
      }
    }
  }

  return PR_DECLINED(cmd);
}

/* Command handlers
 */

MODRET tls_any(cmd_rec *cmd) {
  if (!tls_engine)
    return PR_DECLINED(cmd);

  /* Some commands need not be hindered. */
  if (pr_cmd_cmp(cmd, PR_CMD_SYST_ID) == 0 ||
      pr_cmd_cmp(cmd, PR_CMD_AUTH_ID) == 0 ||
      pr_cmd_cmp(cmd, PR_CMD_FEAT_ID) == 0 ||
      pr_cmd_cmp(cmd, PR_CMD_QUIT_ID) == 0) {
    return PR_DECLINED(cmd);
  }

  if (tls_required_on_auth == 1 &&
      !(tls_flags & TLS_SESS_ON_CTRL)) {

    if (!(tls_opts & TLS_OPT_ALLOW_PER_USER)) {
      if (pr_cmd_cmp(cmd, PR_CMD_USER_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_PASS_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_ACCT_ID) == 0) {
        tls_log("SSL/TLS required but absent for authentication, "
          "denying %s command", cmd->argv[0]);
        pr_response_add_err(R_550,
          _("SSL/TLS required on the control channel"));
        return PR_ERROR(cmd);
      }
    }
  }

  if (tls_required_on_ctrl == 1 &&
      !(tls_flags & TLS_SESS_ON_CTRL)) {

    if (!(tls_opts & TLS_OPT_ALLOW_PER_USER)) {
      tls_log("SSL/TLS required but absent on control channel, "
        "denying %s command", cmd->argv[0]);
      pr_response_add_err(R_550, _("SSL/TLS required on the control channel"));
      return PR_ERROR(cmd);

    } else {

      if (tls_authenticated &&
          *tls_authenticated == TRUE) {
        tls_log("SSL/TLS required but absent on control channel, "
          "denying %s command", cmd->argv[0]);
        pr_response_add_err(R_550,
          _("SSL/TLS required on the control channel"));
        return PR_ERROR(cmd);
      }
    }
  }

  /* TLSRequired checks */

  if (tls_required_on_data == 1) {
    /* TLSRequired encompasses all data transfers for this session, the
     * client did not specify an appropriate PROT, and the command is one
     * which will trigger a data transfer...
     */

    if (!(tls_flags & TLS_SESS_NEED_DATA_PROT)) {
      if (pr_cmd_cmp(cmd, PR_CMD_APPE_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_LIST_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_MLSD_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_NLST_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_RETR_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_STOR_ID) == 0 ||
          pr_cmd_cmp(cmd, PR_CMD_STOU_ID) == 0) {
        tls_log("SSL/TLS required but absent on data channel, "
          "denying %s command", cmd->argv[0]);
        pr_response_add_err(R_522, _("SSL/TLS required on the data channel"));
        return PR_ERROR(cmd);
      }
    }

  } else {

    /* TLSRequired is not in effect for all data transfers for this session.
     * If this command will trigger a data transfer, check the current
     * context to see if there's a directory-level TLSRequired for data
     * transfers.
     *
     * XXX ideally, rather than using the current directory location, we'd
     * do the lookup based on the target location.
     */

    if (pr_cmd_cmp(cmd, PR_CMD_APPE_ID) == 0 ||
        pr_cmd_cmp(cmd, PR_CMD_LIST_ID) == 0 ||
        pr_cmd_cmp(cmd, PR_CMD_MLSD_ID) == 0 ||
        pr_cmd_cmp(cmd, PR_CMD_NLST_ID) == 0 ||
        pr_cmd_cmp(cmd, PR_CMD_RETR_ID) == 0 ||
        pr_cmd_cmp(cmd, PR_CMD_STOR_ID) == 0 ||
        pr_cmd_cmp(cmd, PR_CMD_STOU_ID) == 0) {
      config_rec *c;

      c = find_config(CURRENT_CONF, CONF_PARAM, "TLSRequired", FALSE);
      if (c) {
        int tls_required;

        tls_required = *((int *) c->argv[1]);

        if (tls_required == TRUE &&
            !(tls_flags & TLS_SESS_NEED_DATA_PROT)) {
          tls_log("%s command denied by TLSRequired in directory '%s'",
            cmd->argv[0], session.dir_config ? session.dir_config->name :
            session.anon_config ? session.anon_config->name :
            main_server->ServerName);
          pr_response_add_err(R_522, _("SSL/TLS required on the data channel"));
          return PR_ERROR(cmd);
        }
      }
    }
  }

  return PR_DECLINED(cmd);
}

MODRET tls_auth(cmd_rec *cmd) {
  register unsigned int i = 0;

  if (!tls_engine)
    return PR_DECLINED(cmd);

  /* If we already have protection on the control channel (i.e. AUTH has
   * already been sent by the client and handled), then reject this second
   * AUTH.  Clients that want to renegotiate can either use SSL/TLS's
   * renegotiation facilities, or disconnect and start over.
   */
  if (tls_flags & TLS_SESS_ON_CTRL) {
    tls_log("Unwilling to accept AUTH after AUTH for this session");
    pr_response_add_err(R_503, _("Unwilling to accept second AUTH"));
    return PR_ERROR(cmd);
  }

  if (cmd->argc < 2) {
    pr_response_add_err(R_504, _("AUTH requires at least one argument"));
    return PR_ERROR(cmd);
  }

  if (tls_flags & TLS_SESS_HAVE_CCC) {
    tls_log("Unwilling to accept AUTH after CCC for this session");
    pr_response_add_err(R_534, _("Unwilling to accept security parameters"));
    return PR_ERROR(cmd);
  }

  /* Convert the parameter to upper case */
  for (i = 0; i < strlen(cmd->argv[1]); i++)
    (cmd->argv[1])[i] = toupper((cmd->argv[1])[i]);

  if (strncmp(cmd->argv[1], "TLS", 4) == 0 ||
      strncmp(cmd->argv[1], "TLS-C", 6) == 0) {
    pr_response_send(R_234, _("AUTH %s successful"), cmd->argv[1]);

    tls_log("%s", "TLS/TLS-C requested, starting TLS handshake");
    pr_event_generate("mod_tls.ctrl-handshake", session.c);
    if (tls_accept(session.c, FALSE) < 0) {
      tls_log("%s", "TLS/TLS-C negotiation failed on control channel");

      if (tls_required_on_ctrl == 1) {
        pr_response_send(R_550, _("TLS handshake failed"));
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_CONFIG_ACL,
          "TLSRequired");
      }

      /* If we reach this point, the debug logging may show gibberish
       * commands from the client.  In reality, this gibberish is probably
       * more encrypted data from the client.
       */
      pr_response_send(R_550, _("TLS handshake failed"));
      pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_BY_APPLICATION,
        NULL);
    }

#if OPENSSL_VERSION_NUMBER < 0x0090702fL
    /* Make sure blinding is turned on. (For some reason, this only seems
     * to be allowed on SSL objects, not on SSL_CTX objects.  Bummer).
     */
    tls_blinding_on(ctrl_ssl);
#endif

    tls_flags |= TLS_SESS_ON_CTRL;

  } else if (strncmp(cmd->argv[1], "SSL", 4) == 0 ||
             strncmp(cmd->argv[1], "TLS-P", 6) == 0) {
    pr_response_send(R_234, _("AUTH %s successful"), cmd->argv[1]);

    tls_log("%s", "SSL/TLS-P requested, starting TLS handshake");
    if (tls_accept(session.c, FALSE) < 0) {
      tls_log("%s", "SSL/TLS-P negotiation failed on control channel");

      if (tls_required_on_ctrl == 1) {
        pr_response_send(R_550, _("TLS handshake failed"));
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_CONFIG_ACL,
          "TLSRequired");
      }

      /* If we reach this point, the debug logging may show gibberish
       * commands from the client.  In reality, this gibberish is probably
       * more encrypted data from the client.
       */
      pr_response_send(R_550, _("TLS handshake failed"));
      pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_BY_APPLICATION,
        NULL);
    }

#if OPENSSL_VERSION_NUMBER < 0x0090702fL
    /* Make sure blinding is turned on. (For some reason, this only seems
     * to be allowed on SSL objects, not on SSL_CTX objects.  Bummer).
     */
    tls_blinding_on(ctrl_ssl);
#endif

    tls_flags |= TLS_SESS_ON_CTRL;
    tls_flags |= TLS_SESS_NEED_DATA_PROT;

  } else {
    tls_log("AUTH %s unsupported, declining", cmd->argv[1]);

    /* Allow other RFC2228 modules a chance a handling this command. */
    return PR_DECLINED(cmd);
  }

  pr_session_set_protocol("ftps");
  session.rfc2228_mech = "TLS";

  return PR_HANDLED(cmd);
}

MODRET tls_ccc(cmd_rec *cmd) {

  if (!tls_engine ||
      !session.rfc2228_mech ||
      strncmp(session.rfc2228_mech, "TLS", 4) != 0)
    return PR_DECLINED(cmd);

  if (!(tls_flags & TLS_SESS_ON_CTRL)) {
    pr_response_add_err(R_533,
      _("CCC not allowed on insecure control connection"));
    return PR_ERROR(cmd);
  }

  if (tls_required_on_ctrl == 1) {
    pr_response_add_err(R_534, _("Unwilling to accept security parameters"));
    tls_log("%s: unwilling to accept security parameters: "
      "TLSRequired setting does not allow for unprotected control channel",
      cmd->argv[0]);
    return PR_ERROR(cmd);
  }

  /* Check for <Limit> restrictions. */
  if (!dir_check(cmd->tmp_pool, cmd, G_NONE, session.cwd, NULL)) {
    pr_response_add_err(R_534, _("Unwilling to accept security parameters"));
    tls_log("%s: unwilling to accept security parameters", cmd->argv[0]);
    return PR_ERROR(cmd);
  }

  tls_log("received CCC, clearing control channel protection");

  /* Send the OK response asynchronously; the spec dictates that the
   * response be sent prior to performing the SSL session shutdown.
   */
  pr_response_send_async(R_200, _("Clearing control channel protection"));

  /* Close the SSL session, but only one the control channel.
   * The data channel, if protected, should remain so.
   */

  tls_end_sess(ctrl_ssl, PR_NETIO_STRM_CTRL, TLS_SHUTDOWN_BIDIRECTIONAL);
  pr_table_remove(tls_ctrl_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
  pr_table_remove(tls_ctrl_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
  ctrl_ssl = NULL;

  /* Remove our NetIO for the control channel. */
  pr_unregister_netio(PR_NETIO_STRM_CTRL);

  tls_flags &= ~TLS_SESS_ON_CTRL;
  tls_flags |= TLS_SESS_HAVE_CCC;

  return PR_HANDLED(cmd);
}

MODRET tls_pbsz(cmd_rec *cmd) {

  if (!tls_engine ||
      !session.rfc2228_mech ||
      strncmp(session.rfc2228_mech, "TLS", 4) != 0)
    return PR_DECLINED(cmd);

  CHECK_CMD_ARGS(cmd, 2);

  if (!(tls_flags & TLS_SESS_ON_CTRL)) {
    pr_response_add_err(R_503,
      _("PBSZ not allowed on insecure control connection"));
    return PR_ERROR(cmd);
  }

  /* We expect "PBSZ 0" */
  if (strncmp(cmd->argv[1], "0", 2) == 0) {
    pr_response_add(R_200, _("PBSZ 0 successful"));

  } else {
    pr_response_add(R_200, _("PBSZ=0 successful"));
  }

  tls_flags |= TLS_SESS_PBSZ_OK;
  return PR_HANDLED(cmd);
}

MODRET tls_post_host(cmd_rec *cmd) {

  /* If the HOST command changed the main_server pointer, reinitialize
   * ourselves.
   */
  if (session.prev_server != NULL) {

    /* HOST after AUTH?  Make the SNI check, close the connection if failed. */

    /* HOST before AUTH?  Re-init mod_tls. */

    /* XXX Has a TLS handshake already been performed?  If so, do some stuff. */
    /* XXX Perform SNI check */

#if 0
    int res;

    res = tls_sess_init();
    if (res < 0) {
      pr_session_disconnect(&tls_module,
        PR_SESS_DISCONNECT_SESSION_INIT_FAILED, NULL);
    }
#endif
  }

  return PR_DECLINED(cmd);
}

MODRET tls_post_pass(cmd_rec *cmd) {
  config_rec *protocols_config;

  if (!tls_engine)
    return PR_DECLINED(cmd);

  /* At this point, we can look up the Protocols config if the client has been
   * authenticated, which may have been tweaked via mod_ifsession's 
   * user/group/class-specific sections.
   */
  protocols_config = find_config(main_server->conf, CONF_PARAM, "Protocols",
    FALSE);
  
  if (!(tls_opts & TLS_OPT_ALLOW_PER_USER) &&
      protocols_config == NULL) {
    return PR_DECLINED(cmd);
  }

  tls_authenticated = get_param_ptr(cmd->server->conf, "authenticated", FALSE);

  if (tls_authenticated &&
      *tls_authenticated == TRUE) {
    config_rec *c;

    c = find_config(TOPLEVEL_CONF, CONF_PARAM, "TLSRequired", FALSE);
    if (c) {

      /* Lookup the TLSRequired directive again in this context (which could be
       * <Anonymous>, for example, or modified by mod_ifsession).
       */

      tls_required_on_ctrl = *((int *) c->argv[0]);
      tls_required_on_data = *((int *) c->argv[1]);
      tls_required_on_auth = *((int *) c->argv[2]);

      /* We cannot return PR_ERROR for the PASS command at this point, since
       * this is a POST_CMD handler.  Instead, we will simply check the
       * TLSRequired policy, and if the current session does not make the
       * cut, well, then the session gets cut.
       */
      if ((tls_required_on_ctrl == 1 ||
           tls_required_on_auth == 1) &&
          (!(tls_flags & TLS_SESS_ON_CTRL))) {
        tls_log("SSL/TLS required but absent on control channel, "
          "disconnecting");
        pr_response_send(R_530, "%s", _("Login incorrect."));
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_CONFIG_ACL,
          "TLSRequired");
      }
    }

    if (protocols_config) {
      register unsigned int i;
      int allow_ftps = FALSE;
      array_header *protocols;
      char **elts;

      protocols = protocols_config->argv[0];
      elts = protocols->elts;

      /* We only want to check for 'ftps' in the configured Protocols list
       * if the RFC2228 mechanism is "TLS".
       */
      if (session.rfc2228_mech != NULL &&
          strncmp(session.rfc2228_mech, "TLS", 4) == 0) {
        for (i = 0; i < protocols->nelts; i++) {
          char *proto;

          proto = elts[i];
          if (proto != NULL) {
            if (strncasecmp(proto, "ftps", 5) == 0) {
              allow_ftps = TRUE;
              break;
            }
          }
        }
      }

      if (!allow_ftps) {
        tls_log("ftps protocol denied by Protocols config");
        pr_response_send(R_530, "%s", _("Login incorrect."));
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_CONFIG_ACL,
          "Denied by Protocols setting");
      }
    }
  }

  return PR_DECLINED(cmd);
}

MODRET tls_prot(cmd_rec *cmd) {

  if (!tls_engine ||
      !session.rfc2228_mech ||
      strncmp(session.rfc2228_mech, "TLS", 4) != 0)
    return PR_DECLINED(cmd);

  CHECK_CMD_ARGS(cmd, 2);

  if (!(tls_flags & TLS_SESS_ON_CTRL) &&
      !(tls_flags & TLS_SESS_HAVE_CCC)) {
    pr_response_add_err(R_503,
      _("PROT not allowed on insecure control connection"));
    return PR_ERROR(cmd);
  }

  if (!(tls_flags & TLS_SESS_PBSZ_OK)) {
    pr_response_add_err(R_503,
      _("You must issue the PBSZ command prior to PROT"));
    return PR_ERROR(cmd);
  }

  /* Check for <Limit> restrictions. */
  if (!dir_check(cmd->tmp_pool, cmd, G_NONE, session.cwd, NULL)) {
    pr_response_add_err(R_534, _("Unwilling to accept security parameters"));
    tls_log("%s: denied by <Limit> configuration", cmd->argv[0]);
    return PR_ERROR(cmd);
  }

  /* Only PROT C or PROT P is valid with respect to SSL/TLS. */
  if (strncmp(cmd->argv[1], "C", 2) == 0) {
    char *mesg = "Protection set to Clear";

    if (tls_required_on_data != 1) {
      /* Only accept this if SSL/TLS is not required, by policy, on data
       * connections.
       */
      tls_flags &= ~TLS_SESS_NEED_DATA_PROT;
      pr_response_add(R_200, "%s", mesg);
      tls_log("%s", mesg);

    } else {
      pr_response_add_err(R_534, _("Unwilling to accept security parameters"));
      tls_log("%s: TLSRequired requires protection for data transfers",
        cmd->argv[0]);
      tls_log("%s: unwilling to accept security parameter (%s)", cmd->argv[0],
        cmd->argv[1]);
      return PR_ERROR(cmd);
    }

  } else if (strncmp(cmd->argv[1], "P", 2) == 0) {
    char *mesg = "Protection set to Private";

    if (tls_required_on_data != -1) {
      /* Only accept this if SSL/TLS is allowed, by policy, on data
       * connections.
       */
      tls_flags |= TLS_SESS_NEED_DATA_PROT;
      pr_response_add(R_200, "%s", mesg);
      tls_log("%s", mesg);

    } else {
      pr_response_add_err(R_534, _("Unwilling to accept security parameters"));
      tls_log("%s: TLSRequired does not allow protection for data transfers",
        cmd->argv[0]);
      tls_log("%s: unwilling to accept security parameter (%s)", cmd->argv[0],
        cmd->argv[1]);
      return PR_ERROR(cmd);
    }

  } else if (strncmp(cmd->argv[1], "S", 2) == 0 ||
             strncmp(cmd->argv[1], "E", 2) == 0) {
    pr_response_add_err(R_536, _("PROT %s unsupported"), cmd->argv[1]);

    /* By the time the logic reaches this point, there must have been
     * an SSL/TLS session negotiated; other AUTH mechanisms will handle
     * things differently, and when they do, the logic of this handler
     * would not reach this point.  This means that it would not be impolite
     * to return ERROR here, rather than DECLINED: it shows that mod_tls
     * is handling the security mechanism, and that this module does not
     * allow for the unsupported PROT levels.
     */
    return PR_ERROR(cmd);

  } else {
    pr_response_add_err(R_504, _("PROT %s unsupported"), cmd->argv[1]);
    return PR_ERROR(cmd);
  }

  return PR_HANDLED(cmd);
}

MODRET tls_sscn(cmd_rec *cmd) {

  if (tls_engine == FALSE ||
      session.rfc2228_mech == NULL ||
      strncmp(session.rfc2228_mech, "TLS", 4) != 0) {
    return PR_DECLINED(cmd);
  }

  if (cmd->argc > 2) {
    int xerrno = EINVAL;

    tls_log("denying malformed SSCN command: '%s %s'", cmd->argv[0], cmd->arg);
    pr_response_add_err(R_504, _("%s: %s"), cmd->argv[0], strerror(xerrno));

    errno = xerrno;
    return PR_ERROR(cmd);
  }

  if (!dir_check(cmd->tmp_pool, cmd, cmd->group, session.cwd, NULL)) {
    int xerrno = EPERM;

    pr_log_debug(DEBUG8, "%s denied by <Limit> configuration", cmd->argv[0]);
    tls_log("%s denied by <Limit> configuration", cmd->argv[0]);
    pr_response_add_err(R_550, _("%s: %s"), cmd->argv[0], strerror(xerrno));

    errno = xerrno;
    return PR_ERROR(cmd);
  }

  if (cmd->argc == 1) {
    /* Client is querying our SSCN mode. */
    pr_response_add(R_200, "%s:%s METHOD", cmd->argv[0],
      tls_sscn_mode == TLS_SSCN_MODE_SERVER ? "SERVER" : "CLIENT");

  } else {
    /* Parameter MUST be one of: "ON", "OFF. */
    if (strncmp(cmd->argv[1], "ON", 3) == 0) {
      tls_sscn_mode = TLS_SSCN_MODE_CLIENT;
      pr_response_add(R_200, "%s:CLIENT METHOD", cmd->argv[0]);

    } else if (strncmp(cmd->argv[1], "OFF", 4) == 0) {
      tls_sscn_mode = TLS_SSCN_MODE_SERVER;
      pr_response_add(R_200, "%s:SERVER METHOD", cmd->argv[0]);

    } else {
      int xerrno = EINVAL;

      tls_log("denying unsupported SSCN command: '%s %s'", cmd->argv[0],
        cmd->argv[1]);
      pr_response_add_err(R_501, _("%s: %s"), cmd->argv[0], strerror(xerrno));

      errno = xerrno;
      return PR_ERROR(cmd);
    }
  }

  return PR_HANDLED(cmd);
}

/* Configuration handlers
 */

/* usage: TLSCACertificateFile file */
MODRET set_tlscacertfile(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSCACertificatePath path */
MODRET set_tlscacertpath(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = dir_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, "parameter must be a directory path");
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }
 
  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]); 
  return PR_HANDLED(cmd);
}

/* usage: TLSCARevocationFile file */
MODRET set_tlscacrlfile(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSCARevocationPath path */
MODRET set_tlscacrlpath(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = dir_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, "parameter must be a directory path");
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSCertificateChainFile file */
MODRET set_tlscertchain(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSCipherSuite string */
MODRET set_tlsciphersuite(cmd_rec *cmd) {
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSControlsACLs actions|all allow|deny user|group list */
MODRET set_tlsctrlsacls(cmd_rec *cmd) {
#ifdef PR_USE_CTRLS
  char *bad_action = NULL, **actions = NULL;

  CHECK_ARGS(cmd, 4);
  CHECK_CONF(cmd, CONF_ROOT);

  /* We can cheat here, and use the ctrls_parse_acl() routine to
   * separate the given string...
   */
  actions = ctrls_parse_acl(cmd->tmp_pool, cmd->argv[1]);

  /* Check the second parameter to make sure it is "allow" or "deny" */
  if (strncmp(cmd->argv[2], "allow", 6) != 0 &&
      strncmp(cmd->argv[2], "deny", 5) != 0)
    CONF_ERROR(cmd, "second parameter must be 'allow' or 'deny'");

  /* Check the third parameter to make sure it is "user" or "group" */
  if (strncmp(cmd->argv[3], "user", 5) != 0 &&
      strncmp(cmd->argv[3], "group", 6) != 0)
    CONF_ERROR(cmd, "third parameter must be 'user' or 'group'");

  bad_action = pr_ctrls_set_module_acls(tls_acttab, tls_act_pool, actions,
    cmd->argv[2], cmd->argv[3], cmd->argv[4]);
  if (bad_action != NULL)
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, ": unknown action: '",
      bad_action, "'", NULL));

  return PR_HANDLED(cmd);
#else
  CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "The ", cmd->argv[0],
    " directive requires Controls support (--enable-ctrls)", NULL));
#endif /* PR_USE_CTRLS */
}

/* usage: TLSCryptoDevice driver|"ALL" */
MODRET set_tlscryptodevice(cmd_rec *cmd) {
#if OPENSSL_VERSION_NUMBER > 0x000907000L
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  (void) add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);

  return PR_HANDLED(cmd);

#else /* OpenSSL is too old for ENGINE support */
  CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "The ", cmd->argv[0],
    "directive cannot be used on the system, as the OpenSSL version is too old",
    NULL));
#endif
}

/* usage: TLSDHParamFile file */
MODRET set_tlsdhparamfile(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSDSACertificateFile file */
MODRET set_tlsdsacertfile(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSDSACertificateKeyFile file */
MODRET set_tlsdsakeyfile(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSECCertificateFile file */
MODRET set_tlseccertfile(cmd_rec *cmd) {
#ifdef PR_USE_OPENSSL_ECC
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
#else
  CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "The ", cmd->argv[0],
    " directive cannot be used on this system, as your OpenSSL version "
    "does have EC support", NULL));
#endif /* PR_USE_OPENSSL_ ECC */
}

/* usage: TLSECCertificateKeyFile file */
MODRET set_tlseckeyfile(cmd_rec *cmd) {
#ifdef PR_USE_OPENSSL_ECC
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
#else
  CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "The ", cmd->argv[0],
    " directive cannot be used on this system, as your OpenSSL version "
    "does have EC support", NULL));
#endif /* PR_USE_OPENSSL_ ECC */
}

/* usage: TLSEngine on|off */
MODRET set_tlsengine(cmd_rec *cmd) {
  int bool = -1;
  config_rec *c = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  bool = get_boolean(cmd, 1);
  if (bool == -1)
    CONF_ERROR(cmd, "expected Boolean parameter");

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(unsigned char));
  *((unsigned char *) c->argv[0]) = bool;

  return PR_HANDLED(cmd);
}

/* usage: TLSLog file */
MODRET set_tlslog(cmd_rec *cmd) {
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSMasqueradeAddress ip-addr|dns-name */
MODRET set_tlsmasqaddr(cmd_rec *cmd) {
  config_rec *c = NULL;
  pr_netaddr_t *masq_addr = NULL;
  unsigned int addr_flags = PR_NETADDR_GET_ADDR_FL_INCL_DEVICE;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL);

  /* We can only masquerade as one address, so we don't need to know if the
   * given name might map to multiple addresses.
   */
  masq_addr = pr_netaddr_get_addr2(cmd->server->pool, cmd->argv[1], NULL,
    addr_flags);
  if (masq_addr == NULL) {
    return PR_ERROR_MSG(cmd, NULL, pstrcat(cmd->tmp_pool, cmd->argv[0],
      ": unable to resolve \"", cmd->argv[1], "\"", NULL));
  }

  c = add_config_param(cmd->argv[0], 2, (void *) masq_addr, NULL);
  c->argv[1] = pstrdup(c->pool, cmd->argv[1]);

  return PR_HANDLED(cmd);
}

/* usage: TLSOptions opt1 opt2 ... */
MODRET set_tlsoptions(cmd_rec *cmd) {
  config_rec *c = NULL;
  register unsigned int i = 0;
  unsigned long opts = 0UL;

  if (cmd->argc-1 == 0)
    CONF_ERROR(cmd, "wrong number of parameters");

  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  c = add_config_param(cmd->argv[0], 1, NULL);

  for (i = 1; i < cmd->argc; i++) {
    if (strcmp(cmd->argv[i], "AllowDotLogin") == 0) {
      opts |= TLS_OPT_ALLOW_DOT_LOGIN;

    } else if (strcmp(cmd->argv[i], "AllowPerUser") == 0) {
      opts |= TLS_OPT_ALLOW_PER_USER;

    } else if (strcmp(cmd->argv[i], "AllowClientRenegotiation") == 0 ||
               strcmp(cmd->argv[i], "AllowClientRenegotiations") == 0) {
      opts |= TLS_OPT_ALLOW_CLIENT_RENEGOTIATIONS;

    } else if (strcmp(cmd->argv[i], "AllowWeakDH") == 0) {
      opts |= TLS_OPT_ALLOW_WEAK_DH;

    } else if (strcmp(cmd->argv[i], "EnableDiags") == 0) {
      opts |= TLS_OPT_ENABLE_DIAGS;

    } else if (strcmp(cmd->argv[i], "ExportCertData") == 0) {
      opts |= TLS_OPT_EXPORT_CERT_DATA;

    } else if (strcmp(cmd->argv[i], "NoCertRequest") == 0) {
      opts |= TLS_OPT_NO_CERT_REQUEST;

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
    } else if (strcmp(cmd->argv[i], "NoEmptyFragments") == 0) {
      /* Unlike the other TLSOptions, this option is handled slightly
       * differently, due to the fact that option affects the creation
       * of the SSL_CTX.
       *
       */
      tls_ssl_opts |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
#else
      pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION ": TLSOption NoEmptyFragments not supported (OpenSSL version is too old)");
#endif

    } else if (strcmp(cmd->argv[i], "NoSessionReuseRequired") == 0) {
      opts |= TLS_OPT_NO_SESSION_REUSE_REQUIRED;

    } else if (strcmp(cmd->argv[i], "StdEnvVars") == 0) {
      opts |= TLS_OPT_STD_ENV_VARS;

    } else if (strcmp(cmd->argv[i], "dNSNameRequired") == 0) {
      opts |= TLS_OPT_VERIFY_CERT_FQDN;
 
    } else if (strcmp(cmd->argv[i], "iPAddressRequired") == 0) {
      opts |= TLS_OPT_VERIFY_CERT_IP_ADDR;

    } else if (strcmp(cmd->argv[i], "UseImplicitSSL") == 0) {
      opts |= TLS_OPT_USE_IMPLICIT_SSL;

    } else if (strcmp(cmd->argv[i], "CommonNameRequired") == 0) {
      opts |= TLS_OPT_VERIFY_CERT_CN;

    } else {
      CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, ": unknown TLSOption '",
        cmd->argv[i], "'", NULL));
    }
  }

  c->argv[0] = pcalloc(c->pool, sizeof(unsigned long));
  *((unsigned long *) c->argv[0]) = opts;

  return PR_HANDLED(cmd);
}

/* usage: TLSPassPhraseProvider path */
MODRET set_tlspassphraseprovider(cmd_rec *cmd) {
  struct stat st;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT);

  if (*cmd->argv[1] != '/')
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "must be a full path: '",
      cmd->argv[1], "'", NULL));

  if (stat(cmd->argv[1], &st) < 0)
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "error checking '",
      cmd->argv[1], "': ", strerror(errno), NULL));

  if (!S_ISREG(st.st_mode))
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "unable to use '",
      cmd->argv[1], ": Not a regular file", NULL));

  tls_passphrase_provider = pstrdup(permanent_pool, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSPKCS12File file */
MODRET set_tlspkcs12file(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSProtocol version1 ... versionN */
MODRET set_tlsprotocol(cmd_rec *cmd) {
  register unsigned int i;
  config_rec *c;
  unsigned int tls_protocol = 0;

  if (cmd->argc-1 == 0) {
    CONF_ERROR(cmd, "wrong number of parameters");
  }

  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  for (i = 1; i < cmd->argc; i++) {
    if (strncasecmp(cmd->argv[i], "SSLv23", 7) == 0) {
      tls_protocol |= TLS_PROTO_SSL_V3;
      tls_protocol |= TLS_PROTO_TLS_V1;

    } else if (strncasecmp(cmd->argv[i], "SSLv3", 6) == 0) {
      tls_protocol |= TLS_PROTO_SSL_V3;

    } else if (strncasecmp(cmd->argv[i], "TLSv1", 6) == 0) {
      tls_protocol |= TLS_PROTO_TLS_V1;

    } else if (strncasecmp(cmd->argv[i], "TLSv1.1", 8) == 0) {
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
      tls_protocol |= TLS_PROTO_TLS_V1_1;
#else
      CONF_ERROR(cmd, "Your OpenSSL installation does not support TLSv1.1");
#endif /* OpenSSL 1.0.1 or later */

    } else if (strncasecmp(cmd->argv[i], "TLSv1.2", 8) == 0) {
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
      tls_protocol |= TLS_PROTO_TLS_V1_2;
#else
      CONF_ERROR(cmd, "Your OpenSSL installation does not support TLSv1.2");
#endif /* OpenSSL 1.0.1 or later */

    } else {
      CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "unknown protocol: '",
        cmd->argv[i], "'", NULL));
    }
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = palloc(c->pool, sizeof(unsigned int));
  *((unsigned int *) c->argv[0]) = tls_protocol;

  return PR_HANDLED(cmd);
}

/* usage: TLSRandomSeed file */
MODRET set_tlsrandseed(cmd_rec *cmd) {
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSRenegotiate [ctrl nsecs] [data nbytes] */
MODRET set_tlsrenegotiate(cmd_rec *cmd) {
#if OPENSSL_VERSION_NUMBER > 0x000907000L
  register unsigned int i = 0;
  config_rec *c = NULL;

  if (cmd->argc-1 < 1 || cmd->argc-1 > 8)
    CONF_ERROR(cmd, "wrong number of parameters");

  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  if (strncasecmp(cmd->argv[1], "none", 5) == 0) {
    add_config_param(cmd->argv[0], 0);
    return PR_HANDLED(cmd);
  }

  c = add_config_param(cmd->argv[0], 4, NULL, NULL, NULL, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = 0;
  c->argv[1] = pcalloc(c->pool, sizeof(off_t));
  *((off_t *) c->argv[1]) = 0;
  c->argv[2] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[2]) = 0;
  c->argv[3] = pcalloc(c->pool, sizeof(unsigned char));
  *((unsigned char *) c->argv[3]) = TRUE;

  for (i = 1; i < cmd->argc;) {
    if (strcmp(cmd->argv[i], "ctrl") == 0) {
      int secs = atoi(cmd->argv[i+1]);

      if (secs > 0) {
        *((int *) c->argv[0]) = secs;

      } else {
        CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, cmd->argv[i],
          " must be greater than zero: '", cmd->argv[i+1], "'", NULL));
      }

      i += 2;

    } else if (strcmp(cmd->argv[i], "data") == 0) {
      char *tmp = NULL;
      unsigned long kbytes = strtoul(cmd->argv[i+1], &tmp, 10);

      if (!(tmp && *tmp)) {
        *((off_t *) c->argv[1]) = (off_t) kbytes * 1024;

      } else {
        CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, cmd->argv[i],
          " must be greater than zero: '", cmd->argv[i+1], "'", NULL));
      }

      i += 2;

    } else if (strcmp(cmd->argv[i], "required") == 0) {
      int bool = get_boolean(cmd, i+1);

      if (bool != -1) {
        *((unsigned char *) c->argv[3]) = bool;

      } else {
        CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, cmd->argv[i],
          " must be a Boolean value: '", cmd->argv[i+1], "'", NULL));
      }

      i += 2;

    } else if (strcmp(cmd->argv[i], "timeout") == 0) {
      int secs = atoi(cmd->argv[i+1]);
      
      if (secs > 0) {
        *((int *) c->argv[2]) = secs;

      } else {
        CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, cmd->argv[i],
          " must be greater than zero: '", cmd->argv[i+1], "'", NULL));
      }

      i += 2;

    } else {
      CONF_ERROR(cmd, pstrcat(cmd->tmp_pool,
        ": unknown TLSRenegotiate argument '", cmd->argv[i], "'", NULL));
    }
  }

  return PR_HANDLED(cmd);
#else
  CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, " requires OpenSSL-0.9.7 or greater",
    NULL));
#endif
}

/* usage: TLSRequired on|off|both|control|ctrl|[!]data|auth|auth+data */
MODRET set_tlsrequired(cmd_rec *cmd) {
  int bool = -1;
  int on_auth = 0, on_ctrl = 0, on_data = 0;
  config_rec *c = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL|CONF_ANON|CONF_DIR|
    CONF_DYNDIR);

  bool = get_boolean(cmd, 1);
  if (bool == -1) {
    if (strcmp(cmd->argv[1], "control") == 0 ||
        strcmp(cmd->argv[1], "ctrl") == 0) {
      on_auth = 1;
      on_ctrl = 1;

    } else if (strcmp(cmd->argv[1], "data") == 0) {
      on_data = 1;

    } else if (strcmp(cmd->argv[1], "!data") == 0) {
      on_data = -1;

    } else if (strcmp(cmd->argv[1], "both") == 0 ||
               strcmp(cmd->argv[1], "ctrl+data") == 0) {
      on_auth = 1;
      on_ctrl = 1;
      on_data = 1;

    } else if (strcmp(cmd->argv[1], "ctrl+!data") == 0) {
      on_auth = 1;
      on_ctrl = 1;
      on_data = -1;

    } else if (strcmp(cmd->argv[1], "auth") == 0) {
      on_auth = 1;

    } else if (strcmp(cmd->argv[1], "auth+data") == 0) {
      on_auth = 1;
      on_data = 1;

    } else if (strcmp(cmd->argv[1], "auth+!data") == 0) {
      on_auth = 1;
      on_data = -1;

    } else
      CONF_ERROR(cmd, "bad parameter");

  } else {
    if (bool == TRUE) {
      on_auth = 1;
      on_ctrl = 1;
      on_data = 1;
    }
  }

  c = add_config_param(cmd->argv[0], 3, NULL, NULL, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = on_ctrl;
  c->argv[1] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[1]) = on_data;
  c->argv[2] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[2]) = on_auth;

  c->flags |= CF_MERGEDOWN;

  return PR_HANDLED(cmd);
}

/* usage: TLSRSACertificateFile file */
MODRET set_tlsrsacertfile(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSRSACertificateKeyFile file */
MODRET set_tlsrsakeyfile(cmd_rec *cmd) {
  int res;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  PRIVS_ROOT
  res = file_exists(cmd->argv[1]);
  PRIVS_RELINQUISH

  if (!res) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[1],
      "' does not exist", NULL));
  }

  if (*cmd->argv[1] != '/') {
    CONF_ERROR(cmd, "parameter must be an absolute path");
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSServerCipherPreference on|off */
MODRET set_tlsservercipherpreference(cmd_rec *cmd) {
  int bool = -1;
#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
  config_rec *c = NULL;
#endif

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  bool = get_boolean(cmd, 1);
  if (bool == -1) {
    CONF_ERROR(cmd, "expected Boolean parameter");
  }

#ifdef SSL_OP_CIPHER_SERVER_PREFERENCE
  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = bool;

#else
  pr_log_debug(DEBUG0,
    "%s is not supported by this version of OpenSSL, ignoring",
    cmd->argv[0]);
#endif /* SSL_OP_CIPHER_SERVER_PREFERENCE */

  return PR_HANDLED(cmd);
}

/* usage: TLSSessionCache "off"|type:/info [timeout] */
MODRET set_tlssessioncache(cmd_rec *cmd) {
  char *provider = NULL, *info = NULL;
  config_rec *c;
  long timeout = -1;
  int enabled = -1;

  if (cmd->argc < 2 ||
      cmd->argc > 3) {
    CONF_ERROR(cmd, "wrong number of parameters");
  }

  CHECK_CONF(cmd, CONF_ROOT);

  /* Has session caching been explicitly turned off? */
  enabled = get_boolean(cmd, 1);
  if (enabled != FALSE) {
    char *ptr;

    /* Separate the type/info parameter into pieces. */
    ptr = strchr(cmd->argv[1], ':');
    if (ptr == NULL) {
      CONF_ERROR(cmd, "badly formatted parameter");
    }

    *ptr = '\0';
    provider = cmd->argv[1];
    info = ptr + 1;

    /* Verify that the requested cache type has been registered. */
    if (strncmp(provider, "internal", 9) != 0) {
      if (tls_sess_cache_get_cache(provider) == NULL) {
        CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "session cache type '",
        provider, "' not available", NULL));
      }
    }
  }

  if (cmd->argc == 3) {
    char *ptr = NULL;
   
    timeout = strtol(cmd->argv[2], &ptr, 10);
    if (ptr && *ptr) {
      CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "'", cmd->argv[2],
        "' is not a valid timeout value", NULL));
    }

    if (timeout < 1) {
      CONF_ERROR(cmd, "timeout be greater than 1");
    }

  } else {
    /* Default timeout is 30 min (1800 secs). */
    timeout = 1800;
  }

  c = add_config_param(cmd->argv[0], 3, NULL, NULL, NULL);
  if (provider != NULL) {
    c->argv[0] = pstrdup(c->pool, provider);
  }

  if (info != NULL) {
    c->argv[1] = pstrdup(c->pool, info);
  }

  c->argv[2] = palloc(c->pool, sizeof(long));
  *((long *) c->argv[2]) = timeout;

  return PR_HANDLED(cmd);
}

/* usage: TLSTimeoutHandshake <secs> */
MODRET set_tlstimeouthandshake(cmd_rec *cmd) {
  int timeout = -1;
  config_rec *c = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  if (pr_str_get_duration(cmd->argv[1], &timeout) < 0) {
    CONF_ERROR(cmd, pstrcat(cmd->tmp_pool, "error parsing timeout value '",
      cmd->argv[1], "': ", strerror(errno), NULL));
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(unsigned int));
  *((unsigned int *) c->argv[0]) = timeout;

  return PR_HANDLED(cmd);
}

/* usage: TLSUserName CommonName|EmailSubjAltName|custom-oid */
MODRET set_tlsusername(cmd_rec *cmd) {
  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  /* Make sure the parameter is either "CommonName",
   * "EmailSubjAltName", or a custom OID.
   */
  if (strcmp(cmd->argv[1], "CommonName") != 0 &&
      strcmp(cmd->argv[1], "EmailSubjAltName") != 0) {
    register unsigned int i;
    char *param;
    size_t param_len;

    param = cmd->argv[1];
    param_len = strlen(param);
    for (i = 0; i < param_len; i++) {
      if (!PR_ISDIGIT(param[i]) &&
          param[i] != '.') {
        CONF_ERROR(cmd, "badly formatted OID parameter");
      }
    }
  }

  add_config_param_str(cmd->argv[0], 1, cmd->argv[1]);
  return PR_HANDLED(cmd);
}

/* usage: TLSVerifyClient on|off */
MODRET set_tlsverifyclient(cmd_rec *cmd) {
  int bool = -1;
  config_rec *c = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  bool = get_boolean(cmd, 1);
  if (bool == -1)
    CONF_ERROR(cmd, "expected Boolean parameter");

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(unsigned char));
  *((unsigned char *) c->argv[0]) = bool;

  return PR_HANDLED(cmd);
}

/* usage: TLSVerifyDepth depth */
MODRET set_tlsverifydepth(cmd_rec *cmd) {
  int depth = -1;
  config_rec *c = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  depth = atoi(cmd->argv[1]);
  if (depth < 0)
    CONF_ERROR(cmd, "depth must be zero or greater");
 
  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = depth;
 
  return PR_HANDLED(cmd);
}

/* usage: TLSVerifyOrder mech1 ... */
MODRET set_tlsverifyorder(cmd_rec *cmd) {
  register unsigned int i = 0;
  config_rec *c = NULL;

  /* We only support two client cert verification mechanisms at the moment:
   * CRLs and OCSP.
   */
  if (cmd->argc-1 < 1 ||
      cmd->argc-1 > 2) {
    CONF_ERROR(cmd, "wrong number of parameters");
  }

  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  for (i = 1; i < cmd->argc; i++) {
    char *mech = cmd->argv[i];

    if (strncasecmp(mech, "crl", 4) != 0
#if OPENSSL_VERSION_NUMBER > 0x000907000L
        && strncasecmp(mech, "ocsp", 5) != 0) {
#else
        ) {
#endif
      CONF_ERROR(cmd, pstrcat(cmd->tmp_pool,
        "unsupported verification mechanism '", mech, "' requested", NULL));
    }
  }

  c = add_config_param(cmd->argv[0], cmd->argc-1, NULL, NULL);
  for (i = 1; i < cmd->argc; i++) {
    char *mech = cmd->argv[i];

    if (strncasecmp(mech, "crl", 4) == 0) {
      c->argv[i-1] = pstrdup(c->pool, "crl");

#if OPENSSL_VERSION_NUMBER > 0x000907000L
    } else if (strncasecmp(mech, "ocsp", 5) == 0) {
      c->argv[i-1] = pstrdup(c->pool, "ocsp");
#endif
    }
  }

  return PR_HANDLED(cmd);
}

/* usage: TLSVerifyServer on|NoReverseDNS|off */
MODRET set_tlsverifyserver(cmd_rec *cmd) {
  int setting = -1;
  config_rec *c = NULL;

  CHECK_ARGS(cmd, 1);
  CHECK_CONF(cmd, CONF_ROOT|CONF_VIRTUAL|CONF_GLOBAL);

  setting = get_boolean(cmd, 1);
  if (setting == -1) {
    if (strcasecmp(cmd->argv[1], "NoReverseDNS") != 0) {
      CONF_ERROR(cmd, "expected Boolean parameter");
    }
 
    setting = 2;
  }

  c = add_config_param(cmd->argv[0], 1, NULL);
  c->argv[0] = pcalloc(c->pool, sizeof(int));
  *((int *) c->argv[0]) = setting;

  return PR_HANDLED(cmd);
}

/* Event handlers
 */

#if defined(PR_SHARED_MODULE)
static void tls_mod_unload_ev(const void *event_data, void *user_data) {
  if (strcmp("mod_tls.c", (const char *) event_data) == 0) {
    /* Unregister ourselves from all events. */
    pr_event_unregister(&tls_module, NULL, NULL);

# ifdef PR_USE_CTRLS
    /* Unregister any control actions. */
    pr_ctrls_unregister(&tls_module, "tls");

    destroy_pool(tls_act_pool);
    tls_act_pool = NULL;
# endif /* PR_USE_CTRLS */

    /* Cleanup the OpenSSL stuff. */
    tls_cleanup(0);

    /* Unregister our NetIO handler for the control channel. */
    pr_unregister_netio(PR_NETIO_STRM_CTRL);

    if (tls_ctrl_netio) {
      destroy_pool(tls_ctrl_netio->pool);
      tls_ctrl_netio = NULL;
    }

    if (tls_data_netio) {
      destroy_pool(tls_data_netio->pool);
      tls_data_netio = NULL;
    }

    close(tls_logfd);
    tls_logfd = -1;
  }
}
#endif /* PR_SHARED_MODULE */

/* Daemon PID */
extern pid_t mpid;

static void tls_shutdown_ev(const void *event_data, void *user_data) {
  if (mpid == getpid()) {
    tls_scrub_pkeys();
  }

  /* Write out a new RandomSeed file, for use later. */
  if (tls_rand_file) {
    int res;

    res = RAND_write_file(tls_rand_file);
    if (res < 0) {
      pr_log_pri(PR_LOG_WARNING, MOD_TLS_VERSION
        ": error writing PRNG seed data to '%s': %s", tls_rand_file,
        tls_get_errors());

    } else {
      pr_log_debug(DEBUG2, MOD_TLS_VERSION
        ": wrote %d bytes of PRNG seed data to '%s'", res, tls_rand_file);
    }
  }

  if (ssl_ctx != NULL) {
    SSL_CTX_free(ssl_ctx);
    ssl_ctx = NULL;
  }

  RAND_cleanup();
}

static void tls_restart_ev(const void *event_data, void *user_data) {
#ifdef PR_USE_CTRLS
  register unsigned int i;
#endif /* PR_USE_CTRLS */

  tls_scrub_pkeys();

#ifdef PR_USE_CTRLS
  if (tls_act_pool) {
    destroy_pool(tls_act_pool);
    tls_act_pool = NULL;
  }

  tls_act_pool = make_sub_pool(permanent_pool);
  pr_pool_tag(tls_act_pool, "TLS Controls Pool");

  /* Re-create the controls ACLs. */
  for (i = 0; tls_acttab[i].act_action; i++) {
    tls_acttab[i].act_acl = palloc(tls_act_pool, sizeof(ctrls_acl_t));
    pr_ctrls_init_acl(tls_acttab[i].act_acl);
  }
#endif /* PR_USE_CTRLS */

  tls_closelog();
}

static void tls_exit_ev(const void *event_data, void *user_data) {

  /* If diags are enabled, log some OpenSSL stats. */
  if (ssl_ctx != NULL && 
      (tls_opts & TLS_OPT_ENABLE_DIAGS)) {
    long res;

    res = SSL_CTX_sess_accept(ssl_ctx);
    tls_log("[stat]: SSL sessions attempted: %ld", res);

    res = SSL_CTX_sess_accept_good(ssl_ctx);
    tls_log("[stat]: SSL sessions established: %ld", res);

    res = SSL_CTX_sess_accept_renegotiate(ssl_ctx);
    tls_log("[stat]: SSL sessions renegotiated: %ld", res);

    res = SSL_CTX_sess_hits(ssl_ctx);
    tls_log("[stat]: SSL sessions resumed: %ld", res);

    res = SSL_CTX_sess_number(ssl_ctx);
    tls_log("[stat]: SSL sessions in cache: %ld", res);

    res = SSL_CTX_sess_cb_hits(ssl_ctx);
    tls_log("[stat]: SSL session cache hits: %ld", res);

    res = SSL_CTX_sess_misses(ssl_ctx);
    tls_log("[stat]: SSL session cache misses: %ld", res);

    res = SSL_CTX_sess_timeouts(ssl_ctx);
    tls_log("[stat]: SSL session cache timeouts: %ld", res);

    res = SSL_CTX_sess_cache_full(ssl_ctx);
    tls_log("[stat]: SSL session cache size exceeded: %ld", res);
  }

  /* OpenSSL cleanup */
  tls_cleanup(0);

  /* Done with the NetIO objects.  Note that we only really need to
   * destroy the data channel NetIO object; the control channel NetIO
   * object is allocated out of the permanent pool, in the daemon process,
   * and thus we have a read-only copy.
   */

  if (tls_ctrl_netio) {
    pr_unregister_netio(PR_NETIO_STRM_CTRL);
    destroy_pool(tls_ctrl_netio->pool);
    tls_ctrl_netio = NULL;
  }

  if (tls_data_netio) {
    pr_unregister_netio(PR_NETIO_STRM_DATA);
    destroy_pool(tls_data_netio->pool);
    tls_data_netio = NULL;
  }

  if (mpid != getpid())
    tls_scrub_pkeys();

  tls_closelog();
  return;
}

static void tls_timeout_ev(const void *event_data, void *user_data) {

  if (session.c &&
      ctrl_ssl != NULL &&
      (tls_flags & TLS_SESS_ON_CTRL)) {
    /* Try to properly close the SSL session down on the control channel,
     * if there is one.
     */ 
    tls_end_sess(ctrl_ssl, PR_NETIO_STRM_CTRL, 0);
    pr_table_remove(tls_ctrl_rd_nstrm->notes, TLS_NETIO_NOTE, NULL);
    pr_table_remove(tls_ctrl_wr_nstrm->notes, TLS_NETIO_NOTE, NULL);
    ctrl_ssl = NULL;
  }
}

static void tls_get_passphrases(void) {
  server_rec *s = NULL;
  char buf[256];

  for (s = (server_rec *) server_list->xas_list; s; s = s->next) {
    config_rec *rsa = NULL, *dsa = NULL, *ec = NULL, *pkcs12 = NULL;
    tls_pkey_t *k = NULL;

    /* Find any TLS*CertificateKeyFile directives.  If they aren't present,
     * look for TLS*CertificateFile directives (when appropriate).
     */
    rsa = find_config(s->conf, CONF_PARAM, "TLSRSACertificateKeyFile", FALSE);
    if (rsa == NULL) {
      rsa = find_config(s->conf, CONF_PARAM, "TLSRSACertificateFile", FALSE);
    }

    dsa = find_config(s->conf, CONF_PARAM, "TLSDSACertificateKeyFile", FALSE);
    if (dsa == NULL) {
      dsa = find_config(s->conf, CONF_PARAM, "TLSDSACertificateFile", FALSE);
    }

    ec = find_config(s->conf, CONF_PARAM, "TLSECCertificateKeyFile", FALSE);
    if (ec == NULL) {
      ec = find_config(s->conf, CONF_PARAM, "TLSECCertificateFile", FALSE);
    }

    pkcs12 = find_config(s->conf, CONF_PARAM, "TLSPKCS12File", FALSE);

    if (rsa == NULL &&
        dsa == NULL &&
        ec == NULL &&
        pkcs12 == NULL) {
      continue;
    }

    k = pcalloc(s->pool, sizeof(tls_pkey_t));
    k->pkeysz = PEM_BUFSIZE;
    k->server = s;

    if (rsa) {
      snprintf(buf, sizeof(buf)-1, "RSA key for the %s#%d (%s) server: ",
        pr_netaddr_get_ipstr(s->addr), s->ServerPort, s->ServerName);
      buf[sizeof(buf)-1] = '\0';

      k->rsa_pkey = tls_get_page(PEM_BUFSIZE, &k->rsa_pkey_ptr);
      if (k->rsa_pkey == NULL) {
        pr_log_pri(PR_LOG_ALERT, MOD_TLS_VERSION ": Out of memory!");
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_NOMEM, NULL);
      }

      if (tls_get_passphrase(s, rsa->argv[0], buf, k->rsa_pkey,
          k->pkeysz, TLS_PASSPHRASE_FL_RSA_KEY) < 0) {
        pr_log_debug(DEBUG0, MOD_TLS_VERSION
          ": error reading RSA passphrase: %s", tls_get_errors());

        pr_log_pri(PR_LOG_ERR, MOD_TLS_VERSION ": unable to use "
          "RSA certificate key in '%s', exiting", (char *) rsa->argv[0]);
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_BY_APPLICATION,
          NULL);
      }
    }

    if (dsa) {
      snprintf(buf, sizeof(buf)-1, "DSA key for the %s#%d (%s) server: ",
        pr_netaddr_get_ipstr(s->addr), s->ServerPort, s->ServerName);
      buf[sizeof(buf)-1] = '\0';

      k->dsa_pkey = tls_get_page(PEM_BUFSIZE, &k->dsa_pkey_ptr);
      if (k->dsa_pkey == NULL) {
        pr_log_pri(PR_LOG_ALERT, MOD_TLS_VERSION ": Out of memory!");
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_NOMEM, NULL);
      }

      if (tls_get_passphrase(s, dsa->argv[0], buf, k->dsa_pkey,
          k->pkeysz, TLS_PASSPHRASE_FL_DSA_KEY) < 0) {
        pr_log_debug(DEBUG0, MOD_TLS_VERSION
          ": error reading DSA passphrase: %s", tls_get_errors());

        pr_log_pri(PR_LOG_ERR, MOD_TLS_VERSION ": unable to use "
          "DSA certificate key '%s', exiting", (char *) dsa->argv[0]);
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_BY_APPLICATION,
          NULL);
      }
    }

#ifdef PR_USE_OPENSSL_ECC
    if (ec != NULL) {
      snprintf(buf, sizeof(buf)-1, "EC key for the %s#%d (%s) server: ",
        pr_netaddr_get_ipstr(s->addr), s->ServerPort, s->ServerName);
      buf[sizeof(buf)-1] = '\0';

      k->ec_pkey = tls_get_page(PEM_BUFSIZE, &k->ec_pkey_ptr);
      if (k->ec_pkey == NULL) {
        pr_log_pri(PR_LOG_ALERT, MOD_TLS_VERSION ": Out of memory!");
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_NOMEM, NULL);
      }

      if (tls_get_passphrase(s, ec->argv[0], buf, k->ec_pkey,
          k->pkeysz, TLS_PASSPHRASE_FL_EC_KEY) < 0) {
        pr_log_debug(DEBUG0, MOD_TLS_VERSION
          ": error reading EC passphrase: %s", tls_get_errors());

        pr_log_pri(PR_LOG_ERR, MOD_TLS_VERSION ": unable to use "
          "EC certificate key '%s', exiting", (char *) dsa->argv[0]);
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_BY_APPLICATION,
          NULL);
      }
    }
#endif /* PR_USE_OPENSSL_ECC */

    if (pkcs12) {
      snprintf(buf, sizeof(buf)-1,
        "PKCS12 password for the %s#%d (%s) server: ",
        pr_netaddr_get_ipstr(s->addr), s->ServerPort, s->ServerName);
      buf[sizeof(buf)-1] = '\0';

      k->pkcs12_passwd = tls_get_page(PEM_BUFSIZE, &k->pkcs12_passwd_ptr);
      if (k->pkcs12_passwd == NULL) {
        pr_log_pri(PR_LOG_ALERT, MOD_TLS_VERSION ": Out of memory!");
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_NOMEM, NULL);
      }

      if (tls_get_passphrase(s, pkcs12->argv[0], buf, k->pkcs12_passwd,
          k->pkeysz, TLS_PASSPHRASE_FL_PKCS12_PASSWD) < 0) {
        pr_log_debug(DEBUG0, MOD_TLS_VERSION
          ": error reading PKCS12 password: %s", tls_get_errors());

        pr_log_pri(PR_LOG_ERR, MOD_TLS_VERSION ": unable to use "
          "PKCS12 certificate '%s', exiting", (char *) pkcs12->argv[0]);
        pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_BY_APPLICATION,
          NULL);
      }
    }

    k->next = tls_pkey_list;
    tls_pkey_list = k;
    tls_npkeys++;
  }
}

static void tls_postparse_ev(const void *event_data, void *user_data) {
  server_rec *s = NULL;

  /* Check for incompatible configurations.  For example, configuring:
   *
   *  TLSOptions AllowPerUser
   *  TLSRequired auth
   *
   * cannot be supported; the AllowPerUser means that the requirement of
   * SSL/TLS protection during authentication cannot be enforced.
   */

  for (s = (server_rec *) server_list->xas_list; s; s = s->next) {
    unsigned long *opts;
    config_rec *toplevel_c = NULL, *other_c = NULL;
    int toplevel_auth_requires_ssl = FALSE, other_auth_requires_ssl = TRUE;

    opts = get_param_ptr(s->conf, "TLSOptions", FALSE);
    if (opts == NULL) {
      continue;
    }

    /* The purpose of this check is to watch for configurations such as:
     *
     *  <IfModule mod_tls.c>
     *    ...
     *    TLSRequired on
     *    ...
     *    TLSOptions AllowPerUser
     *    ...
     *  </IfModule>
     *
     * This policy cannot be enforced; we cannot require use of SSL/TLS
     * (specifically at authentication time, when we do NOT know the user)
     * AND also allow per-user SSL/TLS requirements.  It's a chicken-and-egg
     * problem.
     *
     * However, we DO want to allow configurations like:
     *
     *  <IfModule mod_tls.c>
     *    ...
     *    TLSRequired on
     *    ...
     *    TLSOptions AllowPerUser
     *    ...
     *  </IfModule>
     *
     *  <Anonymous ...>
     *    ... 
     *    <IfModule mod_tls.c>
     *      TLSRequired off
     *    </IfModule>
     *  </Anonymous>
     *
     * Thus this check is a bit tricky.  We look first in this server_rec's
     * config list for a top-level TLSRequired setting.  If it is 'on' AND
     * if the AllowPerUser TLSOption is set, AND we find no other TLSRequired
     * configs deeper in the server_rec whose value is 'off', then log the
     * error and quit.  Otherwise, let things proceed.
     *
     * If the mod_ifsession module is present, skip this check as well; we
     * will not be able to suss out any TLSRequired settings which are
     * lurking in mod_ifsession's grasp until authentication time.
     *
     * I still regret adding support for the AllowPerUser TLSOption.  Users
     * just cannot seem to wrap their minds around the fact that the user
     * is not known at the time when the SSL/TLS session is done.  Sigh.
     */

    if (pr_module_exists("mod_ifsession.c")) {
      continue;
    }

    toplevel_c = find_config(s->conf, CONF_PARAM, "TLSRequired", FALSE);
    if (toplevel_c) {
      toplevel_auth_requires_ssl = *((int *) toplevel_c->argv[2]);
    }

    /* If this toplevel TLSRequired value is 'off', then we need check no
     * further.
     */
    if (!toplevel_auth_requires_ssl) {
      continue;
    }

    /* This time, we recurse deeper into the server_rec's configs.
     * We need only pay attention to settings we find in the CONF_DIR or
     * CONF_ANON config contexts.  And we need only look until we find such
     * a setting does not require SSL/TLS during authentication, for at that
     * point we know it is not a misconfiguration.
     */
    find_config_set_top(NULL);
    other_c = find_config(s->conf, CONF_PARAM, "TLSRequired", TRUE);
    while (other_c) {
      int auth_requires_ssl;

      pr_signals_handle();

      if (other_c->parent == NULL ||
          (other_c->parent->config_type != CONF_ANON &&
           other_c->parent->config_type != CONF_DIR)) {
        /* Not what we're looking for; continue on. */ 
        other_c = find_config_next(other_c, other_c->next, CONF_PARAM,
          "TLSRequired", TRUE);
        continue;
      }

      auth_requires_ssl = *((int *) other_c->argv[2]);
      if (!auth_requires_ssl) {
        other_auth_requires_ssl = FALSE;
        break;
      }

      other_c = find_config_next(other_c, other_c->next, CONF_PARAM,
        "TLSRequired", TRUE);
    }

    if ((*opts & TLS_OPT_ALLOW_PER_USER) &&
        toplevel_auth_requires_ssl == TRUE &&
        other_auth_requires_ssl == TRUE) {
      pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION ": Server %s: cannot enforce "
        "both 'TLSRequired auth' and 'TLSOptions AllowPerUser' at the "
        "same time", s->ServerName);
      pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_BAD_CONFIG, NULL);
    }
  }

  /* Initialize the OpenSSL context. */
  if (tls_init_ctx() < 0) {
    pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION
      ": error initialising OpenSSL context");
    pr_session_disconnect(&tls_module, PR_SESS_DISCONNECT_BY_APPLICATION, NULL);
  }

  /* We can only get the passphrases from certs once OpenSSL has been
   * initialized.
   */
  tls_get_passphrases();

  /* Install our control channel NetIO handlers.  This is done here
   * specifically because we need to cache a pointer to the nstrm that
   * is passed to the open callback().  Ideally we'd only install our
   * custom NetIO handlers if the appropriate AUTH command was given.
   * But by then, the open() callback will have already been called, and
   * we will not have a chance to get that nstrm pointer.
   */
  tls_netio_install_ctrl();
}

/* Initialization routines
 */

static int tls_init(void) {
  unsigned long openssl_version;

  /* Check that the OpenSSL headers used match the version of the
   * OpenSSL library used.
   *
   * For now, we only log if there is a difference.
   */
  openssl_version = SSLeay();

  if (openssl_version != OPENSSL_VERSION_NUMBER) {
    int unexpected_version_mismatch = TRUE;

    if (OPENSSL_VERSION_NUMBER >= 0x1000000fL) {
      /* OpenSSL versions after 1.0.0 try to maintain ABI compatibility.
       * So we will warn about header/library version mismatches only if
       * the library is older than the headers.
       */
      if (openssl_version >= OPENSSL_VERSION_NUMBER) {
        unexpected_version_mismatch = FALSE;
      }
    }

    if (unexpected_version_mismatch == TRUE) {
      pr_log_pri(PR_LOG_WARNING, MOD_TLS_VERSION
        ": compiled using OpenSSL version '%s' headers, but linked to "
        "OpenSSL version '%s' library", OPENSSL_VERSION_TEXT,
        SSLeay_version(SSLEAY_VERSION));
      tls_log("compiled using OpenSSL version '%s' headers, but linked to "
        "OpenSSL version '%s' library", OPENSSL_VERSION_TEXT,
        SSLeay_version(SSLEAY_VERSION));
    }
  }

  pr_log_debug(DEBUG2, MOD_TLS_VERSION ": using " OPENSSL_VERSION_TEXT);

#if defined(PR_SHARED_MODULE)
  pr_event_register(&tls_module, "core.module-unload", tls_mod_unload_ev, NULL);
#endif /* PR_SHARED_MODULE */
  pr_event_register(&tls_module, "core.postparse", tls_postparse_ev, NULL);
  pr_event_register(&tls_module, "core.restart", tls_restart_ev, NULL);
  pr_event_register(&tls_module, "core.shutdown", tls_shutdown_ev, NULL);

  SSL_load_error_strings();
  SSL_library_init();

  /* It looks like calling OpenSSL_add_all_algorithms() is necessary for
   * handling some algorithms (e.g. PKCS12 files) which are NOT added by
   * just calling SSL_library_init().
   */
  OpenSSL_add_all_algorithms();

#ifdef PR_USE_CTRLS
  if (pr_ctrls_register(&tls_module, "tls", "query/tune mod_tls settings",
      tls_handle_tls) < 0) {
    pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION
      ": error registering 'tls' control: %s", strerror(errno));

  } else {
    register unsigned int i;

    tls_act_pool = make_sub_pool(permanent_pool);
    pr_pool_tag(tls_act_pool, "TLS Controls Pool");

    for (i = 0; tls_acttab[i].act_action; i++) {
      tls_acttab[i].act_acl = palloc(tls_act_pool, sizeof(ctrls_acl_t));
      pr_ctrls_init_acl(tls_acttab[i].act_acl);
    }
  }
#endif /* PR_USE_CTRLS */

  return 0;
}

static int tls_sess_init(void) {
  int res = 0;
  unsigned char *tmp = NULL;
  config_rec *c = NULL;

  /* First, check to see whether mod_tls is even enabled. */
  tmp = get_param_ptr(main_server->conf, "TLSEngine", FALSE);
  if (tmp != NULL &&
      *tmp == TRUE) {
    tls_engine = TRUE;

  } else {

    /* No need for this modules's control channel NetIO handlers
     * anymore.
     */
    pr_unregister_netio(PR_NETIO_STRM_CTRL);

    /* No need for all the OpenSSL stuff in this process space, either.
     */
    tls_cleanup(TLS_CLEANUP_FL_SESS_INIT);
    tls_scrub_pkeys();

    return 0;
  }

  tls_cipher_suite = get_param_ptr(main_server->conf, "TLSCipherSuite",
    FALSE);
  if (tls_cipher_suite == NULL)
    tls_cipher_suite = TLS_DEFAULT_CIPHER_SUITE;

  tls_crl_file = get_param_ptr(main_server->conf, "TLSCARevocationFile", FALSE);
  tls_crl_path = get_param_ptr(main_server->conf, "TLSCARevocationPath", FALSE);

  tls_dhparam_file = get_param_ptr(main_server->conf, "TLSDHParamFile", FALSE);
  if (tls_dhparam_file != NULL) {
    FILE *fp;
    int xerrno;

    /* Load the DH params from the file. */
    PRIVS_ROOT
    fp = fopen(tls_dhparam_file, "r");
    xerrno = errno;
    PRIVS_RELINQUISH

    if (fp != NULL) {
      DH *dh;

      dh = PEM_read_DHparams(fp, NULL, NULL, NULL);
      if (dh != NULL) {
        tls_tmp_dhs = make_array(session.pool, 1, sizeof(DH *));
      }

      while (dh != NULL) {
        pr_signals_handle();
        *((DH **) push_array(tls_tmp_dhs)) = dh;
        dh = PEM_read_DHparams(fp, NULL, NULL, NULL);
      }

      fclose(fp);

    } else {
      pr_log_debug(DEBUG3, MOD_TLS_VERSION
        ": unable to open TLSDHParamFile '%s': %s", tls_dhparam_file,
          strerror(xerrno));
    }
  }

  tls_dsa_cert_file = get_param_ptr(main_server->conf, "TLSDSACertificateFile",
    FALSE);
  tls_dsa_key_file = get_param_ptr(main_server->conf,
    "TLSDSACertificateKeyFile", FALSE);

  tls_ec_cert_file = get_param_ptr(main_server->conf, "TLSECCertificateFile",
    FALSE);
  tls_ec_key_file = get_param_ptr(main_server->conf,
    "TLSECCertificateKeyFile", FALSE);

  tls_pkcs12_file = get_param_ptr(main_server->conf, "TLSPKCS12File", FALSE);

  tls_rsa_cert_file = get_param_ptr(main_server->conf, "TLSRSACertificateFile",
    FALSE);
  tls_rsa_key_file = get_param_ptr(main_server->conf,
    "TLSRSACertificateKeyFile", FALSE);

  c = find_config(main_server->conf, CONF_PARAM, "TLSOptions", FALSE);
  while (c != NULL) {
    unsigned long opts = 0;

    pr_signals_handle();

    opts = *((unsigned long *) c->argv[0]);
    tls_opts |= opts;

    c = find_config_next(c, c->next, CONF_PARAM, "TLSOptions", FALSE);
  }

#if OPENSSL_VERSION_NUMBER > 0x009080cfL
  /* The OpenSSL team realized that the flag added in 0.9.8l, the
   * SSL3_FLAGS_ALLOW_UNSAFE_LEGACY_RENEGOTIATION flag, was a bad idea.
   * So in later versions, it was changed to a context flag,
   * SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION.
   */
  if (tls_opts & TLS_OPT_ALLOW_CLIENT_RENEGOTIATIONS) {
    int ssl_opts;

    ssl_opts = SSL_CTX_get_options(ssl_ctx);
    ssl_opts |= SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
    SSL_CTX_set_options(ssl_ctx, ssl_opts);
  }
#endif

  tmp = get_param_ptr(main_server->conf, "TLSVerifyClient", FALSE);
  if (tmp != NULL &&
      *tmp == TRUE) {
    tls_flags |= TLS_SESS_VERIFY_CLIENT;
  }

  c = find_config(main_server->conf, CONF_PARAM, "TLSVerifyServer", FALSE);
  if (c != NULL) {
    int setting;

    setting = *((int *) c->argv[0]);
    switch (setting) {
      case 2:
        tls_flags |= TLS_SESS_VERIFY_SERVER_NO_DNS;
        break;

      case 1:
        tls_flags |= TLS_SESS_VERIFY_SERVER;
        break;
    }

  } else {
    tls_flags |= TLS_SESS_VERIFY_SERVER;
  }

  /* If TLSVerifyClient/Server is on, look up the verification depth. */
  if (tls_flags & (TLS_SESS_VERIFY_CLIENT|TLS_SESS_VERIFY_SERVER|TLS_SESS_VERIFY_SERVER_NO_DNS)) {
    int *depth = NULL;

    depth = get_param_ptr(main_server->conf, "TLSVerifyDepth", FALSE);
    if (depth != NULL) {
      tls_verify_depth = *depth;
    }
  }

  c = find_config(main_server->conf, CONF_PARAM, "TLSRequired", FALSE);
  if (c) {
    tls_required_on_ctrl = *((int *) c->argv[0]);
    tls_required_on_data = *((int *) c->argv[1]);
    tls_required_on_auth = *((int *) c->argv[2]);
  }

  c = find_config(main_server->conf, CONF_PARAM, "TLSTimeoutHandshake", FALSE);
  if (c) {
    tls_handshake_timeout = *((unsigned int *) c->argv[0]);
  }

  /* Open the TLSLog, if configured */
  res = tls_openlog();
  if (res < 0) {
    if (res == -1) {
      pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION
        ": notice: unable to open TLSLog: %s", strerror(errno));

    } else if (res == PR_LOG_WRITABLE_DIR) {
      pr_log_pri(PR_LOG_WARNING, MOD_TLS_VERSION
        ": notice: unable to open TLSLog: parent directory is world-writable");

    } else if (res == PR_LOG_SYMLINK) {
      pr_log_pri(PR_LOG_WARNING, MOD_TLS_VERSION
        ": notice: unable to open TLSLog: cannot log to a symbolic link");
    }
  }

  /* If UseReverseDNS is set to off, disable TLS_OPT_VERIFY_CERT_FQDN. */
  if (!ServerUseReverseDNS &&
      ((tls_opts & TLS_OPT_VERIFY_CERT_FQDN) ||
       (tls_opts & TLS_OPT_VERIFY_CERT_CN))) {

    if (tls_opts & TLS_OPT_VERIFY_CERT_FQDN) {
      tls_opts &= ~TLS_OPT_VERIFY_CERT_FQDN;
      tls_log("%s", "reverse DNS off, disabling TLSOption dNSNameRequired");
    }

    if (tls_opts & TLS_OPT_VERIFY_CERT_CN) {
      tls_opts &= ~TLS_OPT_VERIFY_CERT_CN;
      tls_log("%s", "reverse DNS off, disabling TLSOption CommonNameRequired");
    }
  }

  /* We need to check for FIPS mode in the child process as well, in order
   * to re-seed the FIPS PRNG for this process ID.  Annoying, isn't it?
   */
  if (pr_define_exists("TLS_USE_FIPS") &&
      ServerType == SERVER_STANDALONE) {
#ifdef OPENSSL_FIPS
    if (!FIPS_mode()) {
      /* Make sure OpenSSL is set to use the default RNG, as per an email
       * discussion on the OpenSSL developer list:
       *
       *  "The internal FIPS logic uses the default RNG to see the FIPS RNG
       *   as part of the self test process..."
       */
      RAND_set_rand_method(NULL);

      if (!FIPS_mode_set(1)) {
        const char *errstr;

        errstr = tls_get_errors();

        tls_log("unable to use FIPS mode: %s", errstr);
        pr_log_pri(PR_LOG_ERR, MOD_TLS_VERSION ": unable to use FIPS mode: %s",
          errstr);

        errno = EPERM;
        return -1;

      } else {
        tls_log("FIPS mode enabled");
        pr_log_pri(PR_LOG_NOTICE, MOD_TLS_VERSION ": FIPS mode enabled");
      }

    } else {
      tls_log("FIPS mode already enabled");
    }
#else
    pr_log_pri(PR_LOG_WARNING, MOD_TLS_VERSION ": FIPS mode requested, but " OPENSSL_VERSION_TEXT " not built with FIPS support");
#endif /* OPENSSL_FIPS */
  }

  /* Update the session ID context to use.  This is important; it ensures
   * that the session IDs for this particular vhost will differ from those
   * for another vhost.  An external SSL session cache will possibly
   * cache sessions from all vhosts together, and we need to keep them
   * separate.
   */
  SSL_CTX_set_session_id_context(ssl_ctx,
    (unsigned char *) &(main_server->sid), sizeof(main_server->sid));

  /* Install our data channel NetIO handlers. */
  tls_netio_install_data();

  pr_event_register(&tls_module, "core.exit", tls_exit_ev, NULL);

  /* There are several timeouts which can cause the client to be disconnected;
   * register a listener for them which can politely/cleanly shut the SSL/TLS
   * session down before the connection is closed.
   */
  pr_event_register(&tls_module, "core.timeout-idle", tls_timeout_ev, NULL);
  pr_event_register(&tls_module, "core.timeout-login", tls_timeout_ev, NULL);
  pr_event_register(&tls_module, "core.timeout-no-transfer", tls_timeout_ev,
    NULL);
  pr_event_register(&tls_module, "core.timeout-session", tls_timeout_ev, NULL);
  pr_event_register(&tls_module, "core.timeout-stalled", tls_timeout_ev, NULL);

  /* Check to see if a passphrase has been entered for this server. */
  tls_pkey = tls_lookup_pkey();
  if (tls_pkey != NULL) {
    SSL_CTX_set_default_passwd_cb(ssl_ctx, tls_pkey_cb);
    SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx, (void *) tls_pkey);
  }

  /* We always install an info callback, in order to watch for
   * client-initiated session renegotiations (Bug#3324).  If EnableDiags
   * is enabled, that info callback will also log the OpenSSL diagnostic
   * information.
   */
  SSL_CTX_set_info_callback(ssl_ctx, tls_diags_cb);

#if OPENSSL_VERSION_NUMBER > 0x000907000L
  /* Install a callback for logging OpenSSL message information,
   * if requested.
   */
  if (tls_opts & TLS_OPT_ENABLE_DIAGS) {
    tls_log("%s",
      "TLSOption EnableDiags enabled, setting diagnostics callback");
    SSL_CTX_set_msg_callback(ssl_ctx, tls_msg_cb);
  }
#endif

#if OPENSSL_VERSION_NUMBER > 0x000907000L
  /* Handle any requested crypto accelerators/drivers. */
  c = find_config(main_server->conf, CONF_PARAM, "TLSCryptoDevice", FALSE);
  if (c) {
    tls_crypto_device = (const char *) c->argv[0];

    if (strncasecmp(tls_crypto_device, "ALL", 4) == 0) {
      /* Load all ENGINE implementations bundled with OpenSSL. */
      ENGINE_load_builtin_engines();
      ENGINE_register_all_complete();

      tls_log("%s", "enabled all builtin crypto devices");

    } else {
      ENGINE *e;

      /* Load all ENGINE implementations bundled with OpenSSL. */
      ENGINE_load_builtin_engines();

      e = ENGINE_by_id(tls_crypto_device);
      if (e) {
        if (ENGINE_init(e)) {
          if (ENGINE_set_default(e, ENGINE_METHOD_ALL)) {
            ENGINE_finish(e);
            ENGINE_free(e);

            tls_log("using TLSCryptoDevice '%s'", tls_crypto_device);

          } else {
            /* The requested driver could not be used as the default for
             * some odd reason.
             */
            tls_log("unable to register TLSCryptoDevice '%s' as the "
              "default: %s", tls_crypto_device, tls_get_errors());

            ENGINE_finish(e);
            ENGINE_free(e);
            e = NULL;
            tls_crypto_device = NULL;
          }

        } else {
          /* The requested driver could not be initialized. */
          tls_log("unable to initialize TLSCryptoDevice '%s': %s",
            tls_crypto_device, tls_get_errors());

          ENGINE_free(e);
          e = NULL;
          tls_crypto_device = NULL;
        }

      } else {
        /* The requested driver is not available. */
        tls_log("TLSCryptoDevice '%s' is not available", tls_crypto_device);
        tls_crypto_device = NULL;
      }
    }
  }
#endif

  /* Initialize the OpenSSL context for this server's configuration. */
  res = tls_init_server();
  if (res < 0) {
    /* NOTE: should we fail session init if TLS server init fails? */
    tls_log("%s", "error initializing OpenSSL context for this session");
  }

  /* Add the additional features implemented by this module into the
   * list, to be displayed in response to a FEAT command.
   */
  pr_feat_add("AUTH TLS");
  pr_feat_add("CCC");
  pr_feat_add("PBSZ");
  pr_feat_add("PROT");
  pr_feat_add("SSCN");

  /* Add the commands handled by this module to the HELP list. */
  pr_help_add(C_AUTH, _("<sp> base64-data"), TRUE);
  pr_help_add(C_PBSZ, _("<sp> protection buffer size"), TRUE);
  pr_help_add(C_PROT, _("<sp> protection code"), TRUE);

  if (tls_opts & TLS_OPT_USE_IMPLICIT_SSL) {
    tls_log("%s", "TLSOption UseImplicitSSL in effect, starting SSL/TLS "
      "handshake");

    if (tls_accept(session.c, FALSE) < 0) {
      tls_log("%s", "implicit SSL/TLS negotiation failed on control channel");

      errno = EACCES;
      return -1;
    }

    tls_flags |= TLS_SESS_ON_CTRL;

    if (tls_required_on_data != -1) {
      tls_flags |= TLS_SESS_NEED_DATA_PROT;
    }

    pr_session_set_protocol("ftps");
    session.rfc2228_mech = "TLS";
  }

  return 0;
}

#ifdef PR_USE_CTRLS
static ctrls_acttab_t tls_acttab[] = {
  { "clear", NULL, NULL, NULL },
  { "info", NULL, NULL, NULL },
  { "remove", NULL, NULL, NULL },
  { "sesscache", NULL, NULL, NULL },
 
  { NULL, NULL, NULL, NULL }
};

#endif /* PR_USE_CTRLS */

/* Module API tables
 */

static conftable tls_conftab[] = {
  { "TLSCACertificateFile",	set_tlscacertfile,	NULL },
  { "TLSCACertificatePath",	set_tlscacertpath,	NULL },
  { "TLSCARevocationFile",      set_tlscacrlfile,       NULL }, 
  { "TLSCARevocationPath",      set_tlscacrlpath,       NULL }, 
  { "TLSCertificateChainFile",	set_tlscertchain,	NULL },
  { "TLSCipherSuite",		set_tlsciphersuite,	NULL },
  { "TLSControlsACLs",		set_tlsctrlsacls,	NULL },
  { "TLSCryptoDevice",		set_tlscryptodevice,	NULL },
  { "TLSDHParamFile",		set_tlsdhparamfile,	NULL },
  { "TLSDSACertificateFile",	set_tlsdsacertfile,	NULL },
  { "TLSDSACertificateKeyFile",	set_tlsdsakeyfile,	NULL },
  { "TLSECCertificateFile",	set_tlseccertfile,	NULL },
  { "TLSECCertificateKeyFile",	set_tlseckeyfile,	NULL },
  { "TLSEngine",		set_tlsengine,		NULL },
  { "TLSLog",			set_tlslog,		NULL },
  { "TLSMasqueradeAddress",	set_tlsmasqaddr,	NULL },
  { "TLSOptions",		set_tlsoptions,		NULL },
  { "TLSPassPhraseProvider",	set_tlspassphraseprovider, NULL },
  { "TLSPKCS12File", 		set_tlspkcs12file,	NULL },
  { "TLSProtocol",		set_tlsprotocol,	NULL },
  { "TLSRandomSeed",		set_tlsrandseed,	NULL },
  { "TLSRenegotiate",		set_tlsrenegotiate,	NULL },
  { "TLSRequired",		set_tlsrequired,	NULL },
  { "TLSRSACertificateFile",	set_tlsrsacertfile,	NULL },
  { "TLSRSACertificateKeyFile",	set_tlsrsakeyfile,	NULL },
  { "TLSServerCipherPreference",set_tlsservercipherpreference,NULL },
  { "TLSSessionCache",		set_tlssessioncache,	NULL },
  { "TLSTimeoutHandshake",	set_tlstimeouthandshake,NULL },
  { "TLSUserName",		set_tlsusername,	NULL },
  { "TLSVerifyClient",		set_tlsverifyclient,	NULL },
  { "TLSVerifyDepth",		set_tlsverifydepth,	NULL },
  { "TLSVerifyOrder",		set_tlsverifyorder,	NULL },
  { "TLSVerifyServer",		set_tlsverifyserver,	NULL },
  { NULL , NULL, NULL}
};

static cmdtable tls_cmdtab[] = {
  { PRE_CMD,	C_ANY,	G_NONE,	tls_any,	FALSE,	FALSE },
  { CMD,	C_AUTH,	G_NONE,	tls_auth,	FALSE,	FALSE,	CL_SEC },
  { CMD,	C_CCC,	G_NONE,	tls_ccc,	TRUE,	FALSE,	CL_SEC },
  { CMD,	C_PBSZ,	G_NONE,	tls_pbsz,	FALSE,	FALSE,	CL_SEC },
  { CMD,	C_PROT,	G_NONE,	tls_prot,	FALSE,	FALSE,	CL_SEC },
  { CMD,	"SSCN",	G_NONE,	tls_sscn,	TRUE,	FALSE,	CL_SEC },
  { POST_CMD,	C_HOST,	G_NONE,	tls_post_host,	FALSE,	FALSE,	CL_SEC },
  { POST_CMD,	C_PASS,	G_NONE,	tls_post_pass,	FALSE,	FALSE,	CL_SEC },
  { 0,	NULL }
};

static authtable tls_authtab[] = {
  { 0, "auth",			tls_authenticate	},
  { 0, "check",			tls_auth_check		},
  { 0, "requires_pass",		tls_authenticate	},
  { 0, NULL }
};

module tls_module = {

  /* Always NULL */
  NULL, NULL,

  /* Module API version */
  0x20,

  /* Module name */
  "tls",

  /* Module configuration handler table */
  tls_conftab,

  /* Module command handler table */
  tls_cmdtab,

  /* Module authentication handler table */
  tls_authtab,

  /* Module initialization */
  tls_init,

  /* Session initialization */
  tls_sess_init,

  /* Module version */
  MOD_TLS_VERSION
};

