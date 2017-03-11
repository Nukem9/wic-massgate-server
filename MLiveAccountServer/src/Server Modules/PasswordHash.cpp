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
		char random_string[32];
		memset(random_string, 0, sizeof(random_string));
		sprintf(random_string, "%u%u", twister.Random(), this->random_state);

		MD5_Init(&ctx);
		MD5_Update(&ctx, random_string, strlen(random_string));
		MD5_Final(digest, &ctx);

		for (int i = 0; i < 16; ++i)
			sprintf(&md5str1[i*2], "%02x", (unsigned int)digest[i]);

		MD5_Init(&ctx2);
		MD5_Update(&ctx2, md5str1, strlen(md5str1));
		MD5_Final(digest2, &ctx2);

		for (int i = 0; i < 16; ++i)
			sprintf(&md5str2[i*2], "%02x", (unsigned int)digest2[i]);
	}

	memcpy(dst, md5str2, count);
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
	memcpy(output, "$H$", 3);
	memcpy(output+3, &itoa64[min(iteration_count_log2 + (PHP_VERSION >= 5 ? 5 : 3), 30)], 1);
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

void PasswordHash::gensalt_extended(char *output, char *input)
{
	int count_log2 = min(this->iteration_count_log2 + 8, 24);
	int count = (1 << count_log2) - 1;

	*output++ = '_';
	*output++ = this->itoa64[count & 0x3f];
	*output++ = this->itoa64[(count >> 6) & 0x3f];
	*output++ = this->itoa64[(count >> 12) & 0x3f];
	*output++ = this->itoa64[(count >> 18) & 0x3f];
	encode64(output++, input, 3);
}

void PasswordHash::gensalt_blowfish(char *output, char *input)
{
	_crypt_gensalt_blowfish_rn("$2y$", this->iteration_count_log2, input, strlen(input), output, CRYPT_GENSALT_OUTPUT_SIZE);
}

void PasswordHash::crypt_blowfish(char *dst, wchar_t *password, char *setting)
{
	_crypt_blowfish_rn(password, setting, dst, CRYPT_OUTPUT_SIZE);
}

void PasswordHash::HashPassword(char *dst, wchar_t *input)
{
	char random[32];
	memset(random, 0, sizeof(random));

	char salt[CRYPT_GENSALT_OUTPUT_SIZE];
	memset(salt, 0, sizeof(salt));

	if (CRYPT_BLOWFISH == 1 && !this->portable_hashes)
	{
		get_random_bytes(random, 16);
		gensalt_blowfish(salt, random);
		crypt_blowfish(dst, input, salt);

		if (strlen(dst) == 60)
			return;
	}

	if (CRYPT_EXT_DES == 1 && !this->portable_hashes)
	{
		if (strlen(random) < 3)
			get_random_bytes(random, 3);

		gensalt_extended(salt, random);
		// todo: crypt_des? not really needed.

		if (strlen(dst) == 20)
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

	dst[0] = '*';
}

bool PasswordHash::CheckPassword(wchar_t *password, char *stored_hash)
{
	char hash[CRYPT_OUTPUT_SIZE];
	memset(hash, 0, sizeof(hash));

	this->crypt_private(hash, password, stored_hash);

	if (hash[0] == '*')
		this->crypt_blowfish(hash, password, stored_hash);

	return strncmp(hash, stored_hash, CRYPT_OUTPUT_SIZE) == 0;
}

/*
	char password[64];
	memset(password, 0, sizeof(password));

	PasswordHash hasher(8, false);
	hasher.HashPassword(password, L"rightpass");

	if (hasher.CheckPassword(L"rightpass", password))
		printf("pass matched\n");

	if (!hasher.CheckPassword(L"wrongpass", password))
		printf("pass not matched\n");
*/