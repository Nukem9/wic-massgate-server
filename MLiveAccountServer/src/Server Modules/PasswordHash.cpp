#include "../stdafx.h"

void MTwist::seedMT(uint32 seed)
{
	//
	// We initialize state[0..(N-1)] via the generator
	//
	//   x_new = (69069 * x_old) mod 2^32
	//
	// from Line 15 of Table 1, p. 106, Sec. 3.3.4 of Knuth's
	// _The Art of Computer Programming_, Volume 2, 3rd ed.
	//
	// Notes (SJC): I do not know what the initial state requirements
	// of the Mersenne Twister are, but it seems this seeding generator
	// could be better.  It achieves the maximum period for its modulus
	// (2^30) iff x_initial is odd (p. 20-21, Sec. 3.2.1.2, Knuth); if
	// x_initial can be even, you have sequences like 0, 0, 0, ...;
	// 2^31, 2^31, 2^31, ...; 2^30, 2^30, 2^30, ...; 2^29, 2^29 + 2^31,
	// 2^29, 2^29 + 2^31, ..., etc. so I force seed to be odd below.
	//
	// Even if x_initial is odd, if x_initial is 1 mod 4 then
	//
	//   the          lowest bit of x is always 1,
	//   the  next-to-lowest bit of x is always 0,
	//   the 2nd-from-lowest bit of x alternates      ... 0 1 0 1 0 1 0 1 ... ,
	//   the 3rd-from-lowest bit of x 4-cycles        ... 0 1 1 0 0 1 1 0 ... ,
	//   the 4th-from-lowest bit of x has the 8-cycle ... 0 0 0 1 1 1 1 0 ... ,
	//    ...
	//
	// and if x_initial is 3 mod 4 then
	//
	//   the          lowest bit of x is always 1,
	//   the  next-to-lowest bit of x is always 1,
	//   the 2nd-from-lowest bit of x alternates      ... 0 1 0 1 0 1 0 1 ... ,
	//   the 3rd-from-lowest bit of x 4-cycles        ... 0 0 1 1 0 0 1 1 ... ,
	//   the 4th-from-lowest bit of x has the 8-cycle ... 0 0 1 1 1 1 0 0 ... ,
	//    ...
	//
	// The generator's potency (min. s>=0 with (69069-1)^s = 0 mod 2^32) is
	// 16, which seems to be alright by p. 25, Sec. 3.2.1.3 of Knuth.  It
	// also does well in the dimension 2..5 spectral tests, but it could be
	// better in dimension 6 (Line 15, Table 1, p. 106, Sec. 3.3.4, Knuth).
	//
	// Note that the random number user does not see the values generated
	// here directly since reloadMT() will always munge them first, so maybe
	// none of all of this matters.  In fact, the seed values made here could
	// even be extra-special desirable if the Mersenne Twister theory says
	// so-- that's why the only change I made is to restrict to odd seeds.
	//

	register uint32 x = (seed | 1U) & 0xFFFFFFFFU, *s = state;
	register int    j;

	for(left=0, *s++=x, j=N; --j;
		*s++ = (x*=69069U) & 0xFFFFFFFFU);
}

uint32 MTwist::reloadMT()
{
	register uint32 *p0=state, *p2=state+2, *pM=state+M, s0, s1;
	register int    j;

	uint32 tickcount = GetTickCount();
	if ((tickcount % 2) == 0)
		tickcount--;

	if(left < -1)
		seedMT(tickcount);

	left=N-1, next=state+1;

	for(s0=state[0], s1=state[1], j=N-M+1; --j; s0=s1, s1=*p2++)
		*p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

	for(pM=state, j=M; --j; s0=s1, s1=*p2++)
		*p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

	s1=state[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
	s1 ^= (s1 >> 11);
	s1 ^= (s1 <<  7) & 0x9D2C5680U;
	s1 ^= (s1 << 15) & 0xEFC60000U;
	return(s1 ^ (s1 >> 18));
}

uint32 MTwist::randomMT()
{
	uint32 y;

	if(--left < 0)
		return(reloadMT());

	y  = *next++;
	y ^= (y >> 11);
	y ^= (y <<  7) & 0x9D2C5680U;
	y ^= (y << 15) & 0xEFC60000U;
	return(y ^ (y >> 18));
}

void PasswordHash::get_random_bytes(char *dst, int count)
{
	MD5_CTX ctx, ctx2;
	unsigned char digest[16], digest2[16];

	char md5str1[33], md5str2[33];
	memset(md5str1, 0, sizeof(md5str1));
	memset(md5str2, 0, sizeof(md5str2));

	for (int i = 0; i < count; i += 16) 
	{
		// first random number, convert to string
		char randstr1[11];
		memset(randstr1, 0, sizeof(randstr1));
		sprintf(randstr1, "%u", twister.randomMT());

		// second random number, convert to string
		char randstr2[11];
		memset(randstr2, 0, sizeof(randstr2));
		sprintf(randstr2, "%u", this->random_state);
	
		// concatenate random number strings
		char couple[21];
		memset(couple, 0, sizeof(couple));
		sprintf(couple, "%s%s", randstr1, randstr2);

		// md5 it
		MD5_Init(&ctx);
		MD5_Update(&ctx, couple, strlen(couple));
		MD5_Final(digest, &ctx);

		// convert to 32 characters of hex digits
		for(int i = 0; i < 16; ++i)
			sprintf(&md5str1[i*2], "%02x", (unsigned int)digest[i]);

		// stage 2, md5 the first md5
		MD5_Init(&ctx2);
		MD5_Update(&ctx2, md5str1, strlen(md5str1));
		MD5_Final(digest2, &ctx2);

		// convert to 32 characters of hex digits
		for(int i = 0; i < 16; ++i)
			sprintf(&md5str2[i*2], "%02x", (unsigned int)digest2[i]);
	}

	strncpy(dst, md5str2, count);
}

void PasswordHash::encode64(char *dst, char *src, int count)
{
	int i, value;

	i = 0;
	do {
		value = (unsigned char)src[i++];
		*dst++ = this->itoa64[value & 0x3f];
		if (i < count)
			value |= (unsigned char)src[i] << 8;
		*dst++ = this->itoa64[(value >> 6) & 0x3f];
		if (i++ >= count)
			break;
		if (i < count)
			value |= (unsigned char)src[i] << 16;
		*dst++ = this->itoa64[(value >> 12) & 0x3f];
		if (i++ >= count)
			break;
		*dst++ = this->itoa64[(value >> 18) & 0x3f];
	} while (i < count);
}

void PasswordHash::gensalt_private(char* output, char *input)
{
	uint phpversion = 5;	// 3 or 5 it makes no difference until blowfish is added
	strncpy(output, "$H$", 3);
	strncpy(output+3, &itoa64[min(iteration_count_log2 + phpversion, 30)], 1);
	encode64(output+4, input, 6);
}

void PasswordHash::crypt_private(char *dst, char *password, char *setting)
{
	MD5_CTX ctx;
	char hash[MD5_DIGEST_LENGTH];
	char *p, *salt;
	int count_log2, length, count;

	strncpy(dst, "*0", 2);
	if (!strncmp(setting, dst, 2))
		dst[1] = '1';

	if (strncmp(setting, "$H$", 3))
		return;

	p = strchr(this->itoa64, setting[3]);
	if (!p)
		return;
	count_log2 = p - this->itoa64;
	if (count_log2 < 7 || count_log2 > 30)
		return;

	salt = setting + 4;
	if (strlen(salt) < 8)
		return;

	length = strlen(password);

	MD5_Init(&ctx);
	MD5_Update(&ctx, salt, 8);
	MD5_Update(&ctx, password, length);
	MD5_Final((unsigned char*)hash, &ctx);

	count = 1 << count_log2;
	do {
		MD5_Init(&ctx);
		MD5_Update(&ctx, hash, MD5_DIGEST_LENGTH);
		MD5_Update(&ctx, password, length);
		MD5_Final((unsigned char*)hash, &ctx);
	} while (--count);

	memcpy(dst, setting, 12);
	encode64(&dst[12], hash, MD5_DIGEST_LENGTH);
}

void PasswordHash::HashPassword(char *dst, char *input)
{
	char random[17];
	memset(random, 0, sizeof(random));

	char salt[13];
	memset(salt, 0, sizeof(salt));

	// TODO blowfish, DES
	//http://cvsweb.openwall.com/cgi/cvsweb.cgi/projects/phpass/PasswordHash.php?rev=1.8;content-type=text%2Fplain

	if (strlen(random) < 6)
	{
		get_random_bytes(random, 6);
		gensalt_private(salt, random);
		crypt_private(dst, input, salt);

		if (strlen(dst) == 34)
			return;
	}

	strncpy(dst, "*", 1);
}

bool PasswordHash::CheckPassword(char *password, char *stored_hash)
{
	char hash[35];
	memset(hash, 0, sizeof(hash));

	this->crypt_private(hash, password, stored_hash);

	//TODO: blowfish? - http://www.openwall.com/crypt/
	//if (hash[0] == '*')
	//		hash = crypt(password, stored_hash);

	return strncmp(hash, stored_hash, 35) == 0;
}

/*
	//test code, copy to somewhere like main() or MLiveAccountServer::Startup

	//generate test hash here
	//http://phpbbmodders.net/tools/hash/

	char password[35];
	memset(password, 0, sizeof(password));

	PasswordHash hasher(8, true);
	hasher.HashPassword(password, "rightpass");

	if (hasher.CheckPassword("rightpass", password))
		printf("pass matched\n");

	if (!hasher.CheckPassword("wrongpass", password))
		printf("pass not matched\n");
*/