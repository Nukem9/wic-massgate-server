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
		char logFilePath[MAX_PATH];
		memset(logFilePath, 0, sizeof(logFilePath));

		char currentPath[MAX_PATH];
		memset(currentPath, 0, sizeof(currentPath));

		// create a date string with format of YYYYMMDD
		time_t local_timestamp = time(NULL);
		struct tm timestruct;
		localtime_s(&timestruct, &local_timestamp);

		char strTheDate[16] = "";
		strftime(strTheDate, sizeof(strTheDate), "%Y%m%d", &timestruct);

		if (GetModuleFileNameA(GetModuleHandle(nullptr), currentPath, ARRAYSIZE(currentPath) - 4) > 0)
		{
			sprintf_s(logFilePath, "%s_%s.log", currentPath, strTheDate);
			fopen_s(&Logfile, logFilePath, "a+");
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

		fflush(Logfile);
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

	if (Level == L_ERROR)
	{
		fflush(stdout);
		fflush(stderr);

		printf("Unable to recover from error. Press enter to exit.\n");
		getchar();

		ExitProcess(0);
	}
}