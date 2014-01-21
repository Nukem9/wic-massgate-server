#include "../stdafx.h"

ULONGLONG MI_Time::GetTick()
{
	return GetTickCount64();
}

uint MI_Time::GetSystemTime()
{
	return timeGetTime();
}