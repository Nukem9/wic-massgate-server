#define _CRT_SECURE_NO_WARNINGS
#include "../stdafx.h"

void PasswordHash::get_random_bytes(char *dst, int count)
{
	MD5_CTX ctx;
	unsigned char digest[MD5_DIGEST_LENGTH] = "";
	char md5str[CRYPT_BUFFER_SIZE] = "";

	for (int j = 0; j < count; j += 16)
	{
		char random_string[CRYPT_BUFFER_SIZE] = "";
		sprintf(random_string, "%u%u", twister.Random(), this->random_state);

		MD5_Init(&ctx);
		MD5_Update(&ctx, random_string, strlen(random_string));
		MD5_Final(digest, &ctx);

		for (int i = 0; i < 16; ++i)
			sprintf(&md5str[i*2], "%02x", (unsigned int)digest[i]);

		MD5_Init(&ctx);
		MD5_Update(&ctx, md5str, strlen(md5str));
		MD5_Final(digest, &ctx);

		for (int i = 0; i < 16; ++i)
			sprintf(&md5str[i*2], "%02x", (unsigned int)digest[i]);
	}

	memcpy(dst, md5str, count);
}

void PasswordHash::encode64(char *dst, unsigned char *src, int count)
{
	int i, value;

	i = 0;
	do {
		value = (unsigned char)src[i++];
		*dst++ = itoa64[value & 0x3f];
		if (i < count)
			value |= (unsigned char)src[i] << 8;
		*dst++ = itoa64[(value >> 6) & 0x3f];
		if (i++ >= count)
			break;
		if (i < count)
			value |= (unsigned char)src[i] << 16;
		*dst++ = itoa64[(value >> 12) & 0x3f];
		if (i++ >= count)
			break;
		*dst++ = itoa64[(value >> 18) & 0x3f];
	} while (i < count);

	*dst = 0;
}

void PasswordHash::gensalt_private(char *dst, char *src)
{
	memcpy(dst, "$H$", 3);
	memcpy(dst+3, &itoa64[min(iteration_count_log2 + (PHP_VERSION >= 5 ? 5 : 3), 30)], 1);
	encode64(dst+4, (unsigned char*)src, 6);
}

void PasswordHash::crypt_private(char *dst, char *password, char *setting)
{
	MD5_CTX ctx;
	unsigned char hash[MD5_DIGEST_LENGTH];
	char *p, *salt;
	int count_log2, length, count;

	strcpy(dst, "*0");
	if (!strncmp(setting, dst, 2))
		dst[1] = '1';

	if (strncmp(setting, "$H$", 3))
		return;

	p = strchr(itoa64, setting[3]);
	if (!p)
		return;
	count_log2 = p - itoa64;
	if (count_log2 < 7 || count_log2 > 30)
		return;

	salt = setting + 4;
	if (strlen(salt) < 8)
		return;

	length = strlen(password);

	MD5_Init(&ctx);
	MD5_Update(&ctx, salt, 8);
	MD5_Update(&ctx, password, length);
	MD5_Final(hash, &ctx);

	count = 1 << count_log2;
	do {
		MD5_Init(&ctx);
		MD5_Update(&ctx, hash, MD5_DIGEST_LENGTH);
		MD5_Update(&ctx, password, length);
		MD5_Final(hash, &ctx);
	} while (--count);

	memcpy(dst, setting, 12);
	encode64(&dst[12], hash, MD5_DIGEST_LENGTH);
}

void PasswordHash::gensalt_extended(char *dst, char *src)
{
	int count_log2 = min(this->iteration_count_log2 + 8, 24);
	int count = (1 << count_log2) - 1;

	*dst++ = '_';
	*dst++ = itoa64[count & 0x3f];
	*dst++ = itoa64[(count >> 6) & 0x3f];
	*dst++ = itoa64[(count >> 12) & 0x3f];
	*dst++ = itoa64[(count >> 18) & 0x3f];
	encode64(dst++, (unsigned char*)src, 3);
}

void PasswordHash::gensalt_blowfish(char *dst, char *src)
{
	_crypt_gensalt_blowfish_rn("$2y$", this->iteration_count_log2, src, strlen(src), dst, CRYPT_GENSALT_OUTPUT_SIZE);
}

void PasswordHash::crypt_blowfish(char *dst, char *password, char *setting)
{
	_crypt_blowfish_rn(password, setting, dst, CRYPT_OUTPUT_SIZE);
}

int PasswordHash::hash_equals(const void *stored_hash, const void *user_hash, const size_t size)
{
	const unsigned char *_a = (const unsigned char *) stored_hash;
	const unsigned char *_b = (const unsigned char *) user_hash;
	unsigned char result = 0;
	size_t i;

	for (i = 0; i < size; i++)
		result |= _a[i] ^ _b[i];

	return result;
}

void PasswordHash::HashPassword(char *dst, char *password)
{
	char random[CRYPT_BUFFER_SIZE];
	memset(random, 0, sizeof(random));

	char salt[CRYPT_GENSALT_OUTPUT_SIZE];
	memset(salt, 0, sizeof(salt));

	if (CRYPT_BLOWFISH == 1 && !this->portable_hashes)
	{
		get_random_bytes(random, 16);
		gensalt_blowfish(salt, random);
		crypt_blowfish(dst, password, salt);

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
		crypt_private(dst, password, salt);

		if (strlen(dst) == 34)
			return;
	}

	dst[0] = '*';
}

bool PasswordHash::CheckPassword(char *password, char *stored_hash)
{
	char hash[CRYPT_OUTPUT_SIZE];
	memset(hash, 0, sizeof(hash));

	crypt_private(hash, password, stored_hash);

	if (hash[0] == '*')
		crypt_blowfish(hash, password, stored_hash);

	return hash_equals(stored_hash, hash, CRYPT_OUTPUT_SIZE) == 0;
}

/*
	char password[64];
	memset(password, 0, sizeof(password));

	PasswordHash hasher(8, false);
	hasher.HashPassword(password, "rightpass");

	if (hasher.CheckPassword("rightpass", password))
		printf("pass matched\n");

	if (!hasher.CheckPassword("wrongpass", password))
		printf("pass not matched\n");
*/