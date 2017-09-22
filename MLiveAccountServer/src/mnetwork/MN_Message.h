#pragma once

class MN_Message
{
public:
	friend class MN_ReadMessage;
	friend class MN_WriteMessage;

private:
	// Limits
	const static sizeptr_t MESSAGE_MIN_LENGTH = sizeof(ushort);
	const static sizeptr_t MESSAGE_MAX_LENGTH = 0x3FFF;
	
	// Packet flags
	const static sizeptr_t MESSAGE_FLAG_TYPECHECKS = 0x4000;
	const static sizeptr_t MESSAGE_FLAG_COMPRESSED = 0x8000;

	uintptr_t	m_PacketData;
	sizeptr_t	m_PacketMaxSize;
	sizeptr_t	m_DataLen;
	bool		m_TypeChecks;

public:
	MN_Message			(sizeptr_t aMaxSize);
	~MN_Message			();

private:
};