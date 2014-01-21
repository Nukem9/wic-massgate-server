#pragma once

class MN_WriteMessage : public MN_Message
{
public:

private:
	uintptr_t	m_WritePtr;
	uint		m_WritePos;

public:
	MN_WriteMessage(uint aMaxSize) : MN_Message(aMaxSize)
	{
		this->m_WritePtr	= this->m_PacketData + sizeof(ushort);
		this->m_WritePos	= 0;
		this->m_DataLen		= 2;
	}

	template<typename T> __declspec(noinline) void Write(T aValue)
	{
		// Check if the buffer can fit the data
		this->CheckWriteSize(sizeof(T));

		// Write the data
		*(T *)this->m_WritePtr = aValue;

		// Increment pointer
		this->IncWritePos(sizeof(T));
	}

	void WriteDelimiter	(ushort aDelimiter);
	void WriteUChar		(uchar aUChar);
	void WriteUShort	(ushort aUShort);
	void WriteUInt		(uint aUInt);
	void WriteULong		(ulong aULong);
	void WriteUInt64	(uint64 aUInt64);
	void WriteFloat		(float aFloat);
	void WriteRawData	(PVOID aBuffer, uint aBufferSize);
	void WriteString	(char *aBuffer);
	void WriteString	(char *aBuffer, uint aStringSize);
	void WriteString	(wchar_t *aBuffer);
	void WriteString	(wchar_t *aBuffer, uint aStringSize);

	bool SendMe			(SOCKET sock);
	bool SendMe			(SOCKET sock, bool aClearData);

	PVOID GetDataStream	();
	uint GetDataLength	();

private:
	void IncWritePos	(uint aSize);
	void CheckWriteSize	(uint aSize);
};