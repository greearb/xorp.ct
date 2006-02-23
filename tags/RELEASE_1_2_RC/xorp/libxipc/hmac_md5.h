/*
 * FILE:    hmac.h
 * AUTHORS: Colin Perkins
 *
 * HMAC message authentication (RFC2104)
 *
 * Copyright (c) 1998-2000 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef __cplusplus
extern "C" {
#endif
	/**
	 * Generate MD5 message digest.
	 *
	 * @param data data to be digested.
	 * @param data_bytes the amount of data to be digested.
	 * @param key to be used in making digest.
	 * @param key_bytes the amount of key data.
	 * @param digest the buffer to write the digested data.
	 */
	
	void hmac_md5(const unsigned char *data, int data_bytes,
		      const unsigned char *key,  int key_bytes,
		      unsigned char  digest[16]);

	/**
	 * Render an MD5 digest as an ascii string.
	 *
	 * @param digest digest to be rendered.
	 * @param b buffered to write rendering to.
	 * @param b_bytes number of bytes available buffer (at least 33 bytes).
	 *
	 * @return pointer to buffer on success, NULL if insufficient
	 * buffer space is provided.
	 */
	const char* hmac_md5_digest_to_ascii(unsigned char digest[16],
					     char* b, unsigned int b_bytes);
	
#ifdef __cplusplus
}
#endif
