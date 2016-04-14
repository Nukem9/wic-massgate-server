#include "../stdafx.h"

bool MN_ReadMessage::BuildMessage(voidptr_t aData, sizeptr_t aDataLen)
{
	assert(this->m_PacketData && this->m_DataLen == 0);
	assert(aData && aDataLen < this->m_PacketMaxSize);

	ushort packetFlags	= *(ushort *)aData;
	ushort packetLen	= packetFlags & 0x3FFF;
	uint addSize		= sizeof(ushort);

	// Check the length
	if (packetLen > MESSAGE_MAX_LENGTH)
		return false;

	// Determine if type checks should be enabled
	if (packetFlags & MESSAGE_FLAG_TYPECHECKS)
		this->m_TypeChecks = true;

	// Decompress if needed
	if (packetFlags & MESSAGE_FLAG_COMPRESSED)
	{
		sizeptr_t usedBytes;
		this->m_DataLen = MP_Pack::UnpackZip((voidptr_t)((uintptr_t)aData + addSize), (voidptr_t)this->m_PacketData, packetLen, this->m_PacketMaxSize, &usedBytes);

		if (!this->m_DataLen || usedBytes != packetLen)
			return false;
	}
	else
	{
		// Copy the raw packet data
		memcpy((voidptr_t)this->m_PacketData, (voidptr_t)((uintptr_t)aData + addSize), packetLen);

		// Set the maximum read length
		this->m_DataLen = packetLen;
	}

	return true;
}

bool MN_ReadMessage::TypeCheck(ushort aType)
{
	return (!this->m_TypeChecks || aType == this->Read<ushort>());
}

bool MN_ReadMessage::ReadDelimiter(ushort &aDelimiter)
{
	if (!this->TypeCheck('DL'))
		return false;

	aDelimiter = this->Read<ushort>();

	return true;
}

bool MN_ReadMessage::ReadBool(bool &aBool)
{
	if (!this->TypeCheck('BL'))
		return false;

	aBool = this->Read<bool>();

	return true;
}

bool MN_ReadMessage::ReadUChar(uchar &aUChar)
{
	if (!this->TypeCheck('UC'))
		return false;

	aUChar = this->Read<uchar>();

	return true;
}

bool MN_ReadMessage::ReadUShort(ushort &aUShort)
{
	if (!this->TypeCheck('US'))
		return false;

	aUShort = this->Read<ushort>();

	return true;
}

bool MN_ReadMessage::ReadUInt(uint &aUInt)
{
	if (!this->TypeCheck('UI'))
		return false;

	aUInt = this->Read<uint>();

	return true;
}

bool MN_ReadMessage::ReadULong(ulong &aULong)
{
	// Not implemented in the game network engine
	return this->ReadUInt((uint &)aULong);
}

bool MN_ReadMessage::ReadUInt64(uint64 &aUInt64)
{
	if (!this->TypeCheck('U6'))
		return false;

	aUInt64 = this->Read<uint64>();

	return true;
}

bool MN_ReadMessage::ReadFloat(float &aFloat)
{
	if (!this->TypeCheck('FL'))
		return false;

	aFloat = this->Read<float>();

	return true;
}

bool MN_ReadMessage::ReadRawData(voidptr_t aBuffer, sizeptr_t aBufferSize, sizeptr_t *aTotalSize)
{
	assert((aBuffer && aBufferSize > 0) || aTotalSize);

	if (!this->TypeCheck('RD'))
		return false;

	ushort dataLength = this->Read<ushort>();

	this->CheckReadSize(dataLength);

	if (aBuffer)
	{
		memset(aBuffer, 0, aBufferSize);

		if (dataLength > aBufferSize)
			return false;

		memcpy(aBuffer, (voidptr_t)this->m_ReadPtr, dataLength);
	}

	if (aTotalSize)
		*aTotalSize = dataLength;

	if (aBuffer)
		this->IncReadPos(dataLength);

	return true;
}

bool MN_ReadMessage::ReadString(char *aBuffer, sizeptr_t aStringSize)
{
	assert(aBuffer && aStringSize > 0);

	// Packet will exceed bounds before it exceeds ushort
	ushort bufferLength = (ushort)(aStringSize * sizeof(char));
	ushort dataLength	= (ushort)(this->Read<ushort>() * sizeof(char));

	this->CheckReadSize(dataLength);

	if (aBuffer)
	{
		memset(aBuffer, 0, bufferLength);

		if (dataLength > bufferLength)
			return false;

		memcpy(aBuffer, (voidptr_t)this->m_ReadPtr, dataLength);
	}

	this->IncReadPos(dataLength);

	return true;
}

bool MN_ReadMessage::ReadString(wchar_t *aBuffer, sizeptr_t aStringSize)
{
	assert(aBuffer && aStringSize > 0);

	// Packet will exceed bounds before it exceeds ushort
	ushort bufferLength = (ushort)(aStringSize * sizeof(wchar_t));
	ushort dataLength	= (ushort)(this->Read<ushort>() * sizeof(wchar_t));

	this->CheckReadSize(dataLength);

	if (aBuffer)
	{
		memset(aBuffer, 0, bufferLength);

		if (dataLength > bufferLength)
			return false;

		memcpy(aBuffer, (voidptr_t)this->m_ReadPtr, dataLength);
	}

	this->IncReadPos(dataLength);

	return true;
}

void MN_ReadMessage::CheckReadSize(sizeptr_t aSize)
{
	assert((this->m_ReadPos + aSize) < this->m_PacketMaxSize && "Packet read would exceed bounds.");
}

void MN_ReadMessage::IncReadPos(sizeptr_t aSize)
{
	this->m_ReadPtr = this->m_ReadPtr + aSize;
	this->m_ReadPos += aSize;
}