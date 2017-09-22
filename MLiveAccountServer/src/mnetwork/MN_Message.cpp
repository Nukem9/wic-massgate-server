#include "../stdafx.h"

MN_Message::MN_Message(sizeptr_t aMaxSize)
{
	assert(aMaxSize > 0);

	this->m_PacketData		= (uintptr_t)VirtualAlloc(nullptr, aMaxSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	this->m_PacketMaxSize	= aMaxSize;
	this->m_DataLen			= 0;
	this->m_TypeChecks		= false;

	assert(this->m_PacketData);
}

MN_Message::~MN_Message()
{
	VirtualFree((voidptr_t)this->m_PacketData, 0, MEM_RELEASE);
}