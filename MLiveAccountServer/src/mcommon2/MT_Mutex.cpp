#include "../stdafx.h"

MT_Mutex::MT_Mutex()
{
	InitializeCriticalSection(&this->m_CriticalSection);
	InterlockedExchange((LONG *)&this->m_LockCount, 0);
}

MT_Mutex::~MT_Mutex()
{
	assert(this->m_LockCount == 0);

	DeleteCriticalSection(&this->m_CriticalSection);
}

void MT_Mutex::Lock()
{
	EnterCriticalSection(&this->m_CriticalSection);
	InterlockedIncrement((LONG *)&this->m_LockCount);
}

void MT_Mutex::Unlock()
{
	LeaveCriticalSection(&this->m_CriticalSection);
	InterlockedDecrement((LONG *)&this->m_LockCount);
}