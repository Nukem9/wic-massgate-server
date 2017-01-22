#define _CRT_SECURE_NO_WARNINGS
#include "../stdafx.h"

void PasswordHash::get_random_bytes(char *dst, int count)
{
	MD5_CTX ctx, ctx2;
	unsigned char digest[16], digest2[16];

	char md5str1[64], md5str2[64];
	memset(md5str1, 0, sizeof(md5str1));
	memset(md5str2, 0, sizeof(md5str2));

	for (int i = 0; i < count; i += 16) 
	{
		// first random number, convert to string
		char randstr1[16];
		memset(randstr1, 0, sizeof(randstr1));
		sprintf(randstr1, "%u", twister.Random());

		// second random number, convert to string
		char randstr2[16];
		memset(randstr2, 0, sizeof(randstr2));
		sprintf(randstr2, "%u", this->random_state);
	
		// concatenate random number strings
		char couple[32];
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
	uint phpversion = 5;	// 3-5 it makes no difference until blowfish is added
	strncpy(output, "$H$", 3);
	strncpy(output+3, &itoa64[min(iteration_count_log2 + phpversion, 30)], 1);
	encode64(output+4, input, 6);
}

void PasswordHash::crypt_private(char *dst, wchar_t *password, char *setting)
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

	length = wcslen(password) << 1;

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

void PasswordHash::HashPassword(char *dst, wchar_t *input)
{
	char random[32];
	memset(random, 0, sizeof(random));

	char salt[16];
	memset(salt, 0, sizeof(salt));

	// todo: blowfish, DES.
	// not really required as its 'portable' hashes. but its better than md5
	//http://cvsweb.openwall.com/cgi/cvsweb.cgi/projects/phpass/PasswordHash.php?rev=1.8;content-type=text%2Fplain

	if (!this->portable_hashes)
	{
		dst[0] = '*';
		return;
	}

	if (strlen(random) < 6)
	{
		get_random_bytes(random, 6);
		gensalt_private(salt, random);
		crypt_private(dst, input, salt);

		if (strlen(dst) == 34)
			return;
	}

	//memset(dst, 0, strlen(dst));
	dst[0] = '*';
}

bool PasswordHash::CheckPassword(wchar_t *password, char *stored_hash)
{
	char hash[35];
	memset(hash, 0, sizeof(hash));

	this->crypt_private(hash, password, stored_hash);

	//TODO: blowfish? - http://www.openwall.com/crypt/
	//if (hash[0] == '*')
	//		hash = crypt(password, stored_hash);

	return strncmp(hash, stored_hash, 34) == 0;
}

/*
	//test code, copy to somewhere like main() or MLiveAccountServer::Startup

	char password[35];
	memset(password, 0, sizeof(password));

	PasswordHash hasher(8, true);
	hasher.HashPassword(password, L"rightpass");

	if (hasher.CheckPassword(L"rightpass", password))
		printf("pass matched\n");

	if (!hasher.CheckPassword(L"wrongpass", password))
		printf("pass not matched\n");
*/