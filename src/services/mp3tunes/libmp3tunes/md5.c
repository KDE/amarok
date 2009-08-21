#include <stdio.h>

#if defined(HAVE_LIBGCRYPT)
#  include <gcrypt.h>
#elif defined(HAVE_OPENSSL)
#  include <openssl/md5.h>
#else
#  error "You need either OpenSSL or GNU Libgcrypt to build this file"
#endif

#include "md5.h"

void md5_sig_to_string(void *signature, char *str, const int str_len)
{
  unsigned char	*sig_p;
  char		*str_p, *max_p;
  unsigned int	high, low;

  str_p = str;
  max_p = str + str_len;

  for (sig_p = (unsigned char *)signature;
       sig_p < (unsigned char *)signature + MD5_SIZE;
       sig_p++) {
    high = *sig_p / 16;
    low = *sig_p % 16;
    /* account for 2 chars */
    if (str_p + 1 >= max_p) {
      break;
    }
    *str_p++ = HEX_STRING[high];
    *str_p++ = HEX_STRING[low];
  }
  /* account for 2 chars */
  if (str_p < max_p) {
    *str_p++ = '\0';
  }
}

char* md5_calc_file_signature(const char *filename)
{
  char      buffer[4096];
  char*     strsig;
  int       ret;
  FILE      *stream;
#ifdef HAVE_LIBGCRYPT
  gcry_md_hd_t md5;
  gcry_error_t err;
  unsigned char* sig;
#else
  MD5_CTX   md5;
  unsigned char sig[MD5_DIGEST_LENGTH];
#endif

  stream = fopen(filename, "r");
  if (stream == NULL) {
    perror(filename);
    exit(1);
  }
#ifdef HAVE_LIBGCRYPT
  err = gcry_md_open(&md5, GCRY_MD_MD5, 0);
  if (err) {
    fprintf(stderr, "MD5 context creation failure: %s/%s",
      gcry_strsource (err), gcry_strerror (err));
    fclose(stream);
    return NULL;
  }
#else
  MD5_Init(&md5);
#endif

  /* iterate over file */
  while (1) {
    /* read in from our file */
    ret = fread(buffer, sizeof(char), sizeof(buffer), stream);
    if (ret <= 0)
      break;

    /* process our buffer buffer */
#ifdef HAVE_LIBGCRYPT
    gcry_md_write(md5, buffer, ret);
#else
    MD5_Update(&md5, buffer, ret);
#endif
  }

#ifdef HAVE_LIBGCRYPT
  gcry_md_final(md5);
  sig = gcry_md_read(md5, GCRY_MD_MD5);
  if (!sig) {
    fprintf(stderr, "Unable to calculate MD5 signature for %s", filename);
    fclose(stream);
    return NULL;
  }
#else
  MD5_Final(sig, &md5);
#endif

  if (stream != stdin) {
    (void)fclose(stream);
  }

  /* convert to string to print */
  strsig = (char*) malloc(16*2+1);
  if (strsig) {
    md5_sig_to_string(sig, strsig, 16*2+1);
    /*(void)printf("%25s '%s'\n", "File key:", strsig);*/
    return strsig;
  } else {
    return NULL;
  }
}
