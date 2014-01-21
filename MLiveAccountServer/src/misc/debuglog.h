#pragma once

enum LogLevel
{
	L_INFO,
	L_WARN,
	L_ERROR,
};

void DebugLog(LogLevel level, const char *format, ...);