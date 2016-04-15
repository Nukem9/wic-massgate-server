#pragma once

class MN_ReadMessage : public MN_Message
{
private:
	uintptr_t	m_ReadPtr;
	sizeptr_t	m_ReadPos;

public:
	MN_ReadMessage(uint aMaxSize) : MN_Message(aMaxSize)
	{
		this->m_ReadPtr = this->m_PacketData;
		this->m_ReadPos = 0;
	}

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
	bool ReadBool		(bool &aBool);
	bool ReadUChar		(uchar &aUChar);
	bool ReadUShort		(ushort &aUShort);
	bool ReadUInt		(uint &aUInt);
	bool ReadULong		(ulong &aULong);
	bool ReadUInt64		(uint64 &aUInt64);
	bool ReadFloat		(float &aFloat);
	bool ReadRawData	(voidptr_t aBuffer, sizeptr_t aBufferSize, sizeptr_t *aTotalSize);
	bool ReadString		(char *aBuffer, sizeptr_t aStringSize);
	bool ReadString		(wchar_t *aBuffer, sizeptr_t aStringSize);

	bool BuildMessage	(voidptr_t aData, sizeptr_t aDataLen);
	bool DeriveMessage	();

private:
	void IncReadPos		(sizeptr_t aSize);
	void CheckReadSize	(sizeptr_t aSize);
};