#pragma once

class MN_WriteMessage : public MN_Message
{
public:

private:
	uintptr_t	m_WritePtr;
	sizeptr_t	m_WritePos;

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
	void WriteRawData	(voidptr_t aBuffer, sizeptr_t aBufferSize);
	void WriteString	(char *aBuffer);
	void WriteString	(char *aBuffer, sizeptr_t aStringSize);
	void WriteString	(wchar_t *aBuffer);
	void WriteString	(wchar_t *aBuffer, sizeptr_t aStringSize);

	bool SendMe			(SOCKET sock);
	bool SendMe			(SOCKET sock, bool aClearData);

	voidptr_t GetDataStream	();
	sizeptr_t GetDataLength	();

private:
	void IncWritePos	(sizeptr_t aSize);
	void CheckWriteSize	(sizeptr_t aSize);
};