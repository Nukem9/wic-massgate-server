#include "../stdafx.h"

namespace MC_Misc
{
	uint MultibyteToUnicode(const char *in, wchar_t *out, uint outSize)
	{
		size_t convertedChars = 0;

		mbstowcs_s(&convertedChars, out, outSize, in, _TRUNCATE);

		return convertedChars;
	}

	uint UnicodeToMultibyte(const wchar_t *in, char *out, uint outSize)
	{
		size_t convertedChars = 0;

		wcstombs_s(&convertedChars, out, outSize, in, _TRUNCATE);

		return convertedChars;
	}

	uint HashSDBM(const char *str)
	{
		//
		// http://www.cse.yorku.ca/~oz/hash.html
		//
		uint hash	= 0;
		int c		= 0;

		while (c = *str++)
			hash = c + (hash << 6) + (hash << 16) - hash;

		return hash;
	}

	char* MD5String(const wchar_t* input, char* output)
	{
		MD5_CTX ctx;
		unsigned char digest[16];

		MD5_Init(&ctx);
		MD5_Update(&ctx, input, wcslen(input) * 2);
		MD5_Final(digest, &ctx);

		for (int i = 0; i < 16; ++i)
			sprintf(&output[i*2], "%02x", (unsigned int)digest[i]);

		return output;
	}

	char* MD5String(const char* input, char* output)
	{
		MD5_CTX ctx;
		unsigned char digest[16];

		MD5_Init(&ctx);
		MD5_Update(&ctx, input, strlen(input));
		MD5_Final(digest, &ctx);

		for (int i = 0; i < 16; ++i)
			sprintf(&output[i*2], "%02x", (unsigned int)digest[i]);

		return output;
	}
}