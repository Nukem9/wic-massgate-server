#include "../stdafx.h"

void MN_WriteMessage::TypeCheck(ushort aType)
{
	if (this->m_TypeChecks)
		this->Write<ushort>(aType);
}

void MN_WriteMessage::WriteDelimiter(ushort aDelimiter)
{
	this->TypeCheck('DL');
	this->Write<ushort>(aDelimiter);
}

void MN_WriteMessage::WriteBool(bool aBool)
{
	this->TypeCheck('BL');
	this->Write<bool>(aBool);
}

void MN_WriteMessage::WriteUChar(uchar aUChar)
{
	this->TypeCheck('UC');
	this->Write<uchar>(aUChar);
}

void MN_WriteMessage::WriteUShort(ushort aUShort)
{
	this->TypeCheck('US');
	this->Write<ushort>(aUShort);
}

void MN_WriteMessage::WriteInt(int aInt)
{
	this->TypeCheck('IN');
	this->Write<int>(aInt);
}

void MN_WriteMessage::WriteUInt(uint aUInt)
{
	this->TypeCheck('UI');
	this->Write<uint>(aUInt);
}

void MN_WriteMessage::WriteULong(ulong aULong)
{
	this->WriteUInt(aULong);
}

void MN_WriteMessage::WriteUInt64(uint64 aUInt64)
{
	this->TypeCheck('U6');
	this->Write<uint64>(aUInt64);
}

void MN_WriteMessage::WriteFloat(float aFloat)
{
	this->TypeCheck('FL');
	this->Write<float>(aFloat);
}

void MN_WriteMessage::WriteRawData(voidptr_t aBuffer, sizeptr_t aBufferSize)
{
	assert(aBuffer && aBufferSize > 0);
	assert(aBufferSize < MESSAGE_MAX_LENGTH);

	this->TypeCheck('RD');
	ushort bufferSize = (ushort)aBufferSize;

	// Packet will exceed bounds before it exceeds ushort
	this->Write<ushort>(bufferSize);// No typecheck
	this->CheckWriteSize(bufferSize);

	memcpy((voidptr_t)this->m_WritePtr, aBuffer, bufferSize);

	this->IncWritePos(bufferSize);
}

void MN_WriteMessage::WriteString(char *aBuffer)
{
	assert(aBuffer);

	this->WriteString(aBuffer, strlen(aBuffer) + 1);
}

void MN_WriteMessage::WriteString(char *aBuffer, sizeptr_t aStringSize)
{
	assert(aBuffer && aStringSize > 0);
	assert((aStringSize * sizeof(char)) < MESSAGE_MAX_LENGTH);

	// Calculate actual size from number of characters * sizeof(single char)
	ushort stringSize = (ushort)aStringSize;
	ushort bufferSize = (ushort)(stringSize * sizeof(char));

	this->Write<ushort>(stringSize);// No typecheck
	this->CheckWriteSize(bufferSize);

	memcpy((voidptr_t)this->m_WritePtr, aBuffer, bufferSize);

	this->IncWritePos(bufferSize);
}

void MN_WriteMessage::WriteString(wchar_t *aBuffer)
{
	assert(aBuffer);

	this->WriteString(aBuffer, wcslen(aBuffer) + 1);
}

void MN_WriteMessage::WriteString(wchar_t *aBuffer, sizeptr_t aStringSize)
{
	assert(aBuffer && aStringSize > 0);
	assert((aStringSize * sizeof(wchar_t)) < MESSAGE_MAX_LENGTH);

	// Calculate actual size from number of characters * sizeof(single wchar)
	ushort stringSize = (ushort)aStringSize;
	ushort bufferSize = (ushort)(stringSize * sizeof(wchar_t));

	this->Write<ushort>(stringSize);// No typecheck
	this->CheckWriteSize(bufferSize);

	memcpy((voidptr_t)this->m_WritePtr, aBuffer, bufferSize);

	this->IncWritePos(bufferSize);
}

bool MN_WriteMessage::SendMe(SOCKET aSocket)
{
	return this->SendMe(aSocket, true);
}

bool MN_WriteMessage::SendMe(SOCKET aSocket, bool aClearData)
{
	// Windows sockets only supports int as the data size
	if (this->m_DataLen >= INT_MAX)
		return false;

	bool retVal = send(aSocket, (const char *)this->m_PacketData, (int)this->m_DataLen, 0) != SOCKET_ERROR;

	if (aClearData)
	{
		// We keep the old data but reset the writer position
		this->m_WritePtr	= this->m_PacketData + sizeof(ushort);
		this->m_WritePos	= 0;
		this->m_DataLen		= sizeof(ushort);
	}

	return retVal;
}

voidptr_t MN_WriteMessage::GetDataStream()
{
	return (voidptr_t)this->m_PacketData;
}

sizeptr_t MN_WriteMessage::GetDataLength()
{
	return this->m_DataLen;
}

bool MN_WriteMessage::CheckWriteSize(sizeptr_t aSize)
{
	// Note that m_ReadPos is 0-based and must always be less than m_DataLen
	if ((this->m_DataLen + aSize) > this->m_PacketMaxSize)
	{
		assert(false && "Packet write exceeded bounds. Increase packet allocation size.");
		return false;
	}

	return true;
}

void MN_WriteMessage::IncWritePos(sizeptr_t aSize)
{
	this->m_WritePtr	= this->m_WritePtr + aSize;
	this->m_WritePos	+= aSize;
	this->m_DataLen		+= aSize;

	// Write the message-specific information that is stored in the first 2 bytes
	ushort packetInfo = 0;

	// Packet length
	packetInfo = (ushort)(this->m_DataLen - sizeof(ushort));
	packetInfo &= MESSAGE_MAX_LENGTH;

	// Packet flags
	if (this->m_TypeChecks)
		packetInfo |= MESSAGE_FLAG_TYPECHECKS;

	*(ushort *)this->m_PacketData = packetInfo;
}