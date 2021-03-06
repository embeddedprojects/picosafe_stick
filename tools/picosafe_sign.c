/*
 *  Create RSA/SHA-1 signature
 *
 *  This program is based on an example of the PolarSSL library.
 *
 *  Copyright (C) 2006-2011, Brainspark B.V.
 *  Copyright (c) 2012, Michael Hartmann <hartmann@embedded-projects.net>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "polarssl/config.h"

#include "polarssl/rsa.h"
#include "polarssl/sha1.h"

#define PICOSAFE_PRIV "picosafe_priv.txt"

void usage(int argc, char *argv[])
{
  printf("Usage %s: [OPTIONS] FILE\n", argv[0]);
  printf("  This program creates a signature for file FILE.\n");
  printf("  The private key must be generated by picosafe_genkey. If the flag\n");
  printf("  -p is not given asumes the private key in the file picosafe_priv.txt.\n");
  printf("  The program create the signature FILE.sig.\n\n");

  printf("OPTIONS:\n");
  printf("  -p PRIVATEKEY: path to private key\n");
  printf("  -h:            show this help\n");
}

int main(int argc, char *argv[])
{
  FILE *f;
  int ret = 1;
  rsa_context rsa;
  unsigned char hash[20];
  unsigned char buf[512];
  char *privkey      = NULL;
  char *filename     = NULL;
  char *sig_filename = NULL;
  int hflag = 0;
  int c, opterr;

  if(argc < 2)
  {
    usage(argc, argv);
    return 1;
  }

  opterr = 0;
  while((c = getopt(argc, argv, "hp:")) != -1)
  {
    switch (c) 
    {   
      case 'h': hflag   = 1;      break;
      case 'p': privkey = optarg; break;
    }
  }

  if(hflag)
  {
    usage(argc, argv);
    return 0;
  }

  filename = argv[argc-1];

  if(privkey == NULL)
    privkey = PICOSAFE_PRIV;

  if((f = fopen(privkey, "rb")) == NULL)
  {
    ret = 1;
    printf("Can't open %s for reading: %s\n", privkey, strerror(errno));
    goto exit;
  }

  /* init rsa */
  rsa_init(&rsa, RSA_PKCS_V15, 0);
  
  if((ret = mpi_read_file(&rsa.N , 16, f)) != 0 ||
     (ret = mpi_read_file(&rsa.E , 16, f)) != 0 ||
     (ret = mpi_read_file(&rsa.D , 16, f)) != 0 ||
     (ret = mpi_read_file(&rsa.P , 16, f)) != 0 ||
     (ret = mpi_read_file(&rsa.Q , 16, f)) != 0 ||
     (ret = mpi_read_file(&rsa.DP, 16, f)) != 0 ||
     (ret = mpi_read_file(&rsa.DQ, 16, f)) != 0 ||
     (ret = mpi_read_file(&rsa.QP, 16, f)) != 0)
  {
    printf("Can't read %s. mpi_read_file returned %d\n", privkey, ret);
    goto exit;
  }

  rsa.len = (mpi_msb(&rsa.N) + 7) >> 3;

  /* close private key */
  fclose(f);

  /*
   * Compute the SHA-1 hash of the input file,
   * then calculate the RSA signature of the hash.
   */
  if((ret = sha1_file(filename, hash)) != 0)
  {
    printf("Can't open or read %s. sha1_file returned %d\n", filename, ret);
    goto exit;
  }

  if((ret = rsa_pkcs1_sign(&rsa, NULL, NULL, RSA_PRIVATE, SIG_RSA_SHA1, sizeof(hash), hash, buf)) != 0)
  {
    printf("Signing failed, rsa_pkcs1_sign returned %d\n", ret);
    goto exit;
  }

  /*
   * Write the signature into <file>-sig.txt
   */
  if((sig_filename = malloc(strlen(filename)+5)) == NULL)
  {
    printf("Can't allocate memory: %s\n", strerror(errno));
    ret = 1;
    goto exit;
  }
  strcpy(sig_filename, filename);
  strcat(sig_filename, ".sig");

  if((f = fopen(sig_filename, "wb+")) == NULL)
  {
    ret = 1;
    printf("Can't create %s: %s\n", sig_filename, strerror(errno));
    goto exit;
  }

  fclose(f);

exit:
  if(sig_filename != NULL)
    free(sig_filename);

  return ret;
}
