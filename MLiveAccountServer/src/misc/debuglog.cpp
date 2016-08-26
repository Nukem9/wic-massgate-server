#include "../stdafx.h"

#define DEBUG_OUT(dest, fmt, ...) fprintf(dest, fmt, __VA_ARGS__); DebugLogFile(fmt, __VA_ARGS__);

bool LogfileRegistered;
FILE *Logfile;

void DebugLogFile(const char *Format, ...)
{
	// Register a logfile when available
	if (!LogfileRegistered)
	{
		LogfileRegistered = true;

		// Generate a path with programname.exe.log
		char currentPath[MAX_PATH];
		memset(currentPath, 0, sizeof(currentPath));

		if (GetModuleFileNameA(GetModuleHandle(nullptr), currentPath, ARRAYSIZE(currentPath) - 4) > 0)
		{
			strcat_s(currentPath, ".log");
			fopen_s(&Logfile, currentPath, "w");
		}

		// Close the file handle on exit
		std::atexit([]
		{
			if (Logfile)
				fclose(Logfile);
		});
	}

	// Perform actual file write
	if (Logfile)
	{
		va_list va;
		va_start(va, Format);
		vfprintf(Logfile, Format, va);
		va_end(va);
	}
}

void DebugLog(LogLevel Level, const char *Format, ...)
{
	char buffer[2048];
	memset(buffer, 0, sizeof(buffer));

	va_list va;
    va_start(va, Format);
	vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, Format, va);
    va_end(va);

	switch(Level)
	{
		case L_INFO: DEBUG_OUT(stdout, "[+] %s\n", buffer); break;
		case L_WARN: DEBUG_OUT(stderr, "[*] %s\n", buffer); break;
		case L_ERROR:DEBUG_OUT(stderr, "[!] %s\n", buffer); break;
	}

	fflush(stdout);
	fflush(stderr);

	if (Level == L_ERROR)
	{
		printf("Unable to recover from error. Press enter to exit.\n");
		getchar();

		ExitProcess(0);
	}
}