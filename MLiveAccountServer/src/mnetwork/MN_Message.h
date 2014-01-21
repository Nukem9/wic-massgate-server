#pragma once

#define UC_Size sizeof(uchar)
#define US_Size sizeof(ushort)
#define UI_Size sizeof(uint)
#define UL_Size sizeof(ulong)
#define U6_Size sizeof(uint64)
#define FL_Size sizeof(float)

#define MESSAGE_FLAG_COMPRESSED 0x8000

class MN_Message
{
public:
	friend class MN_ReadMessage;
	friend class MN_WriteMessage;

private:
	uintptr_t	m_PacketData;
	uint		m_PacketMaxSize;

	uint		m_DataLen;

	bool		m_TypeChecks;

public:
	MN_Message			(uint aMaxSize);
	~MN_Message			();

private:
};