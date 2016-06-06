#include "../stdafx.h"

void seedMT(uint32 seed)
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


uint32 reloadMT(void)
{
	register uint32 *p0=state, *p2=state+2, *pM=state+M, s0, s1;
	register int    j;

	if(left < -1)
		seedMT((uint32)GetTickCount());

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


uint32 randomMT(void)
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
/*
void PasswordHash::get_random_bytes(char *src, int count)
{
	static char output[35];
	memset(src, 0, sizeof(src));

	for (int i = 0; i < count; i += 16) 
	{
		//this->random_state = md5(microtime() . $this->random_state);
		//$output .= pack('H*', md5($this->random_state));
	}
}
*/
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

//char *PasswordHash::gensalt_private(char *input)
//{
//
//}

char *PasswordHash::crypt_private(char *password, char *setting)
{
	static char output[35];
	MD5_CTX ctx;
	char hash[MD5_DIGEST_LENGTH];
	char *p, *salt;
	int count_log2, length, count;

	strcpy(output, "*0");
	if (!strncmp(setting, output, 2))
		output[1] = '1';

	if (strncmp(setting, "$H$", 3))
		return output;

	p = strchr(this->itoa64, setting[3]);
	if (!p)
		return output;
	count_log2 = p - this->itoa64;
	if (count_log2 < 7 || count_log2 > 30)
		return output;

	salt = setting + 4;
	if (strlen(salt) < 8)
		return output;

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

	memcpy(output, setting, 12);
	encode64(&output[12], hash, MD5_DIGEST_LENGTH);

	return output;
}

//char *PasswordHash::HashPassword(char *input)
//{
//
//}

bool PasswordHash::CheckPassword(char *password, char *stored_hash)
{
	char *hash;

	hash = this->crypt_private(password, stored_hash);

	//blowfish? - http://www.openwall.com/crypt/
	//if (hash[0] == '*')
	//		hash = crypt(password, stored_hash);

	return strcmp(hash, stored_hash) == 0;
}

/*
	//test code, copy to somewhere like main() or MLiveAccountServer::Startup
	//http://cvsweb.openwall.com/cgi/cvsweb.cgi/projects/phpass/PasswordHash.php?rev=1.8;content-type=text%2Fplain
	//generate test hash here, change to phpbb format
	//http://phpbbmodders.net/tools/hash/
	PasswordHash hasher(8, true);
	if (hasher.CheckPassword("testpass", "$H$9aHh9LeQd4rHfHxPbqbi1q.xOBgKQj1"))
		printf("pass matched\n");
*/