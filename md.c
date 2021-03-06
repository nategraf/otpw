/*
 * Universal wrapper API for a message digest function
 *
 * Markus Kuhn <http://www.cl.cam.ac.uk/~mgk25/>
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "md.h"
#include "rmd160.h"

void md_init(md_state * md)
{
  /* check assumptions made in rmd160.h (should produce no code) */
  assert(sizeof(dword) == 4);
  assert(sizeof(word) == 2);
  assert(sizeof(byte) == 1);

  md->length_lo = md->length_hi = 0;
  rmd160_init((dword *) md->md);
}


void md_add(md_state *md, const void *src, size_t len)
{
  int i;
  int remaining = md->length_lo & (MD_BUFLEN-1);
  unsigned chunk;
  unsigned long old_lo = md->length_lo;
  dword X[16];

  /* update 64-bit counter of bytes added so far */
  md->length_lo += len & 0xffffffff;
  if (md->length_lo < old_lo)
    md->length_hi++;
#if SIZE_MAX > 4294967295U
  md->length_hi += len >> 32;
  if (md->length_lo > 4294967295U) {
    md->length_hi += md->length_lo >> 32;
    md->length_lo &= 0xffffffff;
  }
#endif
  /* complete remaining input block of compression function */
  if (remaining > 0) {
    chunk = MD_BUFLEN - remaining;
    if (chunk > len)
      chunk = len;
    memcpy(md->buf + remaining, src, chunk);
    len -= chunk;
    src += chunk;
    if (remaining + chunk == MD_BUFLEN) {
      for (i = 0; i < 64; i += 4)
	X[i>>2] = BYTES_TO_DWORD(md->buf + i);
      rmd160_compress((dword *) md->md, X);
    }
  }
  /* feed whole input blocks to compression function */
  while (len >= MD_BUFLEN) {
    for (i = 0; i < 64; i += 4)
      X[i>>2] = BYTES_TO_DWORD((unsigned char *)src + i);
    rmd160_compress((dword *) md->md, X);
    src += MD_BUFLEN;
    len -= MD_BUFLEN;
  }
  /* partially fill buffer with remaining bytes */
  if (len > 0)
    memcpy(md->buf, src, len);
}


void md_close(md_state *md, unsigned char *result)
{
  int i;

  rmd160_finish((dword *) md->md, (byte *) md->buf,
		(dword) md->length_lo, (dword) md->length_hi);

  for (i = 0; i < MD_LEN; i++)
    result[i] = ((dword *) md->md)[i>>2] >> (8 * (i & 3));
}


int md_selftest(void)
{
  int i, j, fail = 0;
  md_state md;
  unsigned char result[MD_LEN];

  char *pattern[8] = {
    "",
    "a",
    "abc",
    "message digest",
    "abcdefghijklmnopqrstuvwxyz",
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    "123456789012345678901234567890123456789012345678901234567890"
    "12345678901234567890"
  };

#ifdef MD_RIPEMD128
  unsigned char md_result[9][MD_LEN] = {
    { 0xcd, 0xf2, 0x62, 0x13, 0xa1, 0x50, 0xdc, 0x3e,
      0xcb, 0x61, 0x0f, 0x18, 0xf6, 0xb3, 0x8b, 0x46 },
    { 0x86, 0xbe, 0x7a, 0xfa, 0x33, 0x9d, 0x0f, 0xc7,
      0xcf, 0xc7, 0x85, 0xe7, 0x2f, 0x57, 0x8d, 0x33 },
    { 0xc1, 0x4a, 0x12, 0x19, 0x9c, 0x66, 0xe4, 0xba,
      0x84, 0x63, 0x6b, 0x0f, 0x69, 0x14, 0x4c, 0x77 },
    { 0x9e, 0x32, 0x7b, 0x3d, 0x6e, 0x52, 0x30, 0x62,
      0xaf, 0xc1, 0x13, 0x2d, 0x7d, 0xf9, 0xd1, 0xb8 },
    { 0xfd, 0x2a, 0xa6, 0x07, 0xf7, 0x1d, 0xc8, 0xf5,
      0x10, 0x71, 0x49, 0x22, 0xb3, 0x71, 0x83, 0x4e },
    { 0xa1, 0xaa, 0x06, 0x89, 0xd0, 0xfa, 0xfa, 0x2d,
      0xdc, 0x22, 0xe8, 0x8b, 0x49, 0x13, 0x3a, 0x06 },
    { 0xd1, 0xe9, 0x59, 0xeb, 0x17, 0x9c, 0x91, 0x1f,
      0xae, 0xa4, 0x62, 0x4c, 0x60, 0xc5, 0xc7, 0x02 },
    { 0x3f, 0x45, 0xef, 0x19, 0x47, 0x32, 0xc2, 0xdb,
      0xb2, 0xc4, 0xa2, 0xc7, 0x69, 0x79, 0x5f, 0xa3 },
    { 0x4a, 0x7f, 0x57, 0x23, 0xf9, 0x54, 0xeb, 0xa1,
      0x21, 0x6c, 0x9d, 0x8f, 0x63, 0x20, 0x43, 0x1f }
  };
#endif
#ifdef MD_RIPEMD160
  unsigned char md_result[9][MD_LEN] = {
    { 0x9c, 0x11, 0x85, 0xa5, 0xc5, 0xe9, 0xfc, 0x54, 0x61, 0x28,
      0x08, 0x97, 0x7e, 0xe8, 0xf5, 0x48, 0xb2, 0x25, 0x8d, 0x31 },
    { 0x0b, 0xdc, 0x9d, 0x2d, 0x25, 0x6b, 0x3e, 0xe9, 0xda, 0xae,
      0x34, 0x7b, 0xe6, 0xf4, 0xdc, 0x83, 0x5a, 0x46, 0x7f, 0xfe },
    { 0x8e, 0xb2, 0x08, 0xf7, 0xe0, 0x5d, 0x98, 0x7a, 0x9b, 0x04,
      0x4a, 0x8e, 0x98, 0xc6, 0xb0, 0x87, 0xf1, 0x5a, 0x0b, 0xfc },
    { 0x5d, 0x06, 0x89, 0xef, 0x49, 0xd2, 0xfa, 0xe5, 0x72, 0xb8,
      0x81, 0xb1, 0x23, 0xa8, 0x5f, 0xfa, 0x21, 0x59, 0x5f, 0x36 },
    { 0xf7, 0x1c, 0x27, 0x10, 0x9c, 0x69, 0x2c, 0x1b, 0x56, 0xbb,
      0xdc, 0xeb, 0x5b, 0x9d, 0x28, 0x65, 0xb3, 0x70, 0x8d, 0xbc },
    { 0x12, 0xa0, 0x53, 0x38, 0x4a, 0x9c, 0x0c, 0x88, 0xe4, 0x05,
      0xa0, 0x6c, 0x27, 0xdc, 0xf4, 0x9a, 0xda, 0x62, 0xeb, 0x2b },
    { 0xb0, 0xe2, 0x0b, 0x6e, 0x31, 0x16, 0x64, 0x02, 0x86, 0xed,
      0x3a, 0x87, 0xa5, 0x71, 0x30, 0x79, 0xb2, 0x1f, 0x51, 0x89 },
    { 0x9b, 0x75, 0x2e, 0x45, 0x57, 0x3d, 0x4b, 0x39, 0xf4, 0xdb,
      0xd3, 0x32, 0x3c, 0xab, 0x82, 0xbf, 0x63, 0x32, 0x6b, 0xfb },
    { 0x52, 0x78, 0x32, 0x43, 0xc1, 0x69, 0x7b, 0xdb, 0xe1, 0x6d,
      0x37, 0xf9, 0x7f, 0x68, 0xf0, 0x83, 0x25, 0xdc, 0x15, 0x28 }
  };
#endif

  for (i = 0; i <= 16; i++) {
    md_init(&md);
    if (i == 16)
      /* test 16: one million 'a' */
      for (j = 0; j < 1000000; j += 125)
	md_add(&md, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
	       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
	       "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 125);
    else if (i & 1)
      /* single byte feed */
      for (j = 0; pattern[i/2][j]; j++)
	md_add(&md, pattern[i/2]+j, 1);
    else
      /* single chunk feed */
      md_add(&md, pattern[i/2], strlen(pattern[i/2]));
    md_close(&md, result);
    if (memcmp(result, md_result[i/2], MD_LEN) != 0) {
      abort();
      fail++;
    }
  }

  return fail;
}
