#include "../stdafx.h"

bool MN_ReadMessage::TypeCheck(ushort aType)
{
	// Succeed if checks not enabled
	if (!this->m_TypeChecks)
		return true;

	// Validate size and read the delimiter
	if (!this->CheckReadSize(sizeof(ushort)))
		return false;

	return (aType == this->Read<ushort>());
}

bool MN_ReadMessage::ReadDelimiter(ushort &aDelimiter)
{
	return this->ReadChecked('DL', aDelimiter);
}

bool MN_ReadMessage::ReadBool(bool &aBool)
{
	return this->ReadChecked('BL', aBool);
}

bool MN_ReadMessage::ReadChar(char &aChar)
{
	return this->ReadChecked('CH', aChar);
}

bool MN_ReadMessage::ReadUChar(uchar &aUChar)
{
	return this->ReadChecked('UC', aUChar);
}

bool MN_ReadMessage::ReadUShort(ushort &aUShort)
{
	return this->ReadChecked('US', aUShort);
}

bool MN_ReadMessage::ReadUInt(uint &aUInt)
{
	return this->ReadChecked('UI', aUInt);
}

bool MN_ReadMessage::ReadULong(ulong &aULong)
{
	// Not implemented in the game network engine
	return this->ReadUInt((uint &)aULong);
}

bool MN_ReadMessage::ReadUInt64(uint64 &aUInt64)
{
	return this->ReadChecked('U6', aUInt64);
}

bool MN_ReadMessage::ReadFloat(float &aFloat)
{
	return this->ReadChecked('FL', aFloat);
}

bool MN_ReadMessage::ReadRawData(voidptr_t aBuffer, sizeptr_t aBufferSize, sizeptr_t *aTotalSize)
{
	assert((aBuffer && aBufferSize > 0) || aTotalSize);

	ushort dataLength = 0;
	if (!this->ReadChecked('RD', dataLength))
		return false;

	if (!this->CheckReadSize(dataLength))
		return false;

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

	if (!this->CheckReadSize(sizeof(ushort)))
		return false;

	// Packet will exceed bounds before it exceeds ushort
	ushort bufferLength = (ushort)(aStringSize * sizeof(char));
	ushort dataLength	= (ushort)(this->Read<ushort>() * sizeof(char));

	if (!this->CheckReadSize(dataLength))
		return false;

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

	if (!this->CheckReadSize(sizeof(ushort)))
		return false;

	// Packet will exceed bounds before it exceeds ushort
	ushort bufferLength = (ushort)(aStringSize * sizeof(wchar_t));
	ushort dataLength	= (ushort)(this->Read<ushort>() * sizeof(wchar_t));

	if (!this->CheckReadSize(dataLength))
		return false;

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

bool MN_ReadMessage::BuildMessage(voidptr_t aData, sizeptr_t aDataLen)
{
	assert(this->m_PacketData && this->m_DataLen == 0);
	assert(aData && aDataLen <= this->m_PacketMaxSize);
	assert(aDataLen >= sizeof(ushort));

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

	this->m_ReadPos = 0;
	this->m_ReadPtr = this->m_PacketData;
	return true;
}

bool MN_ReadMessage::DeriveMessage()
{
	//
	// Build another message after the current one is considered to be at the end. A new delimiter
	// usually begins the message in the remaining data.
	//

	// Calculate remainder
	sizeptr_t remainder = this->m_DataLen - this->m_ReadPos;

	if (remainder <= 0)
		return false;

	// Shift the data buffer over (left)
	{
		// Copy the raw packet data
		memmove((voidptr_t)this->m_PacketData, (voidptr_t)this->m_ReadPtr, remainder);

		// Zero extra data at the end
		memset((voidptr_t)(this->m_PacketData + remainder), 0, this->m_PacketMaxSize - remainder);

		// Set the maximum read length
		this->m_DataLen = remainder;
	}

	this->m_ReadPos = 0;
	this->m_ReadPtr = this->m_PacketData;
	return true;
}

bool MN_ReadMessage::CheckReadSize(sizeptr_t aSize)
{
	assert((this->m_ReadPos + aSize) <= this->m_DataLen && "Packet read would exceed data length.");

	if ((this->m_ReadPos + aSize) > this->m_DataLen)
		return false;

	return true;
}

void MN_ReadMessage::IncReadPos(sizeptr_t aSize)
{
	this->m_ReadPtr = this->m_ReadPtr + aSize;
	this->m_ReadPos += aSize;
}