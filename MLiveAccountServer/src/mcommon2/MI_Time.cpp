#include "../stdafx.h"

__int64 MI_Time::GetQPC()
{
	LARGE_INTEGER li;

	//if (!QueryPerformanceFrequency(&li))
	//	DebugBreak();

	if (!QueryPerformanceCounter(&li))
		DebugBreak();

	return li.QuadPart;
}

__int32 MI_Time::GetTick32()
{
	return GetTickCount();
}

__int64 MI_Time::GetTick()
{
	return GetTickCount64();
}

__int64 MI_Time::GetRDTSC()
{
	return __rdtsc();
}

uint MI_Time::GetSystemTime()
{
	return timeGetTime();
}