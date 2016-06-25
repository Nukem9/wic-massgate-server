#pragma once

class MI_Time
{
public:

private:

public:
	static __int64	GetQPC			();
	static __int32	GetTick32		();
	static __int64	GetTick			();
	static __int64	GetRDTSC		();
	static uint		GetSystemTime	();

private:
};