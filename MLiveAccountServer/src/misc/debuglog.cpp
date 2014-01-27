#include "../stdafx.h"

void DebugLog(LogLevel level, const char *format, ...)
{
    va_list va;
	char buffer[2048];

	memset(buffer, 0, sizeof(buffer));

    va_start(va, format);
	vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, va);
    va_end(va);

	switch(level)
	{
		case L_INFO:	fprintf(stdout, "[+] %s\n", buffer); break;
		case L_WARN:	fprintf(stderr, "[*] %s\n", buffer); break;
		case L_ERROR:	fprintf(stderr, "[!] %s\n", buffer); break;
	}

	fflush(stdout);
	fflush(stderr);

	if (level == L_ERROR)
	{
		printf("Unable to recover from error. Press enter to exit.\n");
		getchar();

		ExitProcess(0);
	}
}