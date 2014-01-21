#pragma once

class MN_ReadMessage : public MN_Message
{
public:

private:
	uintptr_t	m_ReadPtr;
	uint		m_ReadPos;

public:
	MN_ReadMessage(uint aMaxSize) : MN_Message(aMaxSize)
	{
		this->m_ReadPtr = this->m_PacketData;
		this->m_ReadPos = 0;
	}

	bool BuildMessage	(PVOID aData, uint aDataLen);

	template<typename T> __declspec(noinline) T Read()
	{
		// Check if the read is past the end
		this->CheckReadSize(sizeof(T));

		// Read the value stored
		T val = *(T *)this->m_ReadPtr;

		// Increment the read position
		this->IncReadPos(sizeof(T));

		return val;
	}

	bool TypeCheck		(ushort aType);
	bool ReadDelimiter	(ushort &aDelimiter);
	bool ReadUChar		(uchar &aUChar);
	bool ReadUShort		(ushort &aUShort);
	bool ReadUInt		(uint &aUInt);
	bool ReadULong		(ulong &aULong);
	bool ReadUInt64		(uint64 &aUInt64);
	bool ReadFloat		(float &aFloat);
	bool ReadRawData	(PVOID aBuffer, uint aBufferSize, uint *aTotalSize);
	bool ReadString		(char *aBuffer, uint aStringSize);
	bool ReadString		(wchar_t *aBuffer, uint aStringSize);

private:
	void IncReadPos		(uint aSize);
	void CheckReadSize	(uint aSize);
};