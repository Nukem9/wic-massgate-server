#pragma once

namespace MC_Misc
{
	uint MultibyteToUnicode(const char *in, wchar_t *out, uint outSize);
	uint UnicodeToMultibyte(const wchar_t *in, char *out, uint outSize);

	uint HashSDBM(const char *str);

	char* MD5String(const wchar_t* input, char* output);
	char* MD5String(const char* input, char* output);
}