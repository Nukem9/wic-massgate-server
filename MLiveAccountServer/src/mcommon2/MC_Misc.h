#pragma once

namespace MC_Misc
{
	uint MultibyteToUnicode(const char *in, wchar_t *out, uint outSize);
	uint UnicodeToMultibyte(const wchar_t *in, char *out, uint outSize);

	uint HashSDBM(const char *str);
}