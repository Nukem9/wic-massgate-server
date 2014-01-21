#pragma once

class MT_Mutex
{
public:

private:
	uint				m_LockCount;
	CRITICAL_SECTION	m_CriticalSection;

public:
	MT_Mutex();
	~MT_Mutex();

	void Lock();
	void Unlock();

private:
};