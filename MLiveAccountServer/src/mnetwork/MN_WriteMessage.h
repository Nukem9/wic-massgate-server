#pragma once

class MN_WriteMessage : public MN_Message
{
private:
	uintptr_t	m_WritePtr;
	sizeptr_t	m_WritePos;

public:
	MN_WriteMessage(uint aMaxSize) : MN_Message(aMaxSize)
	{
		this->m_WritePtr	= this->m_PacketData + sizeof(ushort);
		this->m_WritePos	= 0;
		this->m_DataLen		= sizeof(ushort);
	}

	void TypeCheck		(ushort aType);
	void WriteDelimiter	(ushort aDelimiter);
	void WriteBool      (bool aBool);
	void WriteUChar		(uchar aUChar);
	void WriteUShort	(ushort aUShort);
	void WriteInt		(int aInt);
	void WriteUInt		(uint aUInt);
	void WriteULong		(ulong aULong);
	void WriteUInt64	(uint64 aUInt64);
	void WriteFloat		(float aFloat);
	void WriteRawData	(voidptr_t aBuffer, sizeptr_t aBufferSize);
	void WriteString	(char *aBuffer);
	void WriteString	(char *aBuffer, sizeptr_t aStringSize);
	void WriteString	(wchar_t *aBuffer);
	void WriteString	(wchar_t *aBuffer, sizeptr_t aStringSize);

	bool SendMe			(SOCKET aSocket);
	bool SendMe			(SOCKET aSocket, bool aClearData);

	voidptr_t GetDataStream	();
	sizeptr_t GetDataLength	();

private:
	template<typename T>
	__declspec(noinline) void Write(T aValue)
	{
		if (!this->CheckWriteSize(sizeof(T)))
		{
			// We're out of space for aValue - just ignore any further writes
			DebugLog(L_WARN, "%s: BUFFER OVERRUN", __FUNCTION__);
			return;
		}

		*(T *)this->m_WritePtr = aValue;
		this->IncWritePos(sizeof(T));
	}

	void IncWritePos	(sizeptr_t aSize);
	bool CheckWriteSize	(sizeptr_t aSize);
};