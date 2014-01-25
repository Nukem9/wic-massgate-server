#include "../stdafx.h"

void MN_WriteMessage::WriteDelimiter(ushort aDelimiter)
{
	if(this->m_TypeChecks)
		this->Write<ushort>('DL');

	this->Write<ushort>(aDelimiter);
}

void MN_WriteMessage::WriteUChar(uchar aUChar)
{
	if(this->m_TypeChecks)
		this->Write<ushort>('UC');

	this->Write<uchar>(aUChar);
}

void MN_WriteMessage::WriteUShort(ushort aUShort)
{
	if(this->m_TypeChecks)
		this->Write<ushort>('US');

	this->Write<ushort>(aUShort);
}

void MN_WriteMessage::WriteUInt(uint aUInt)
{
	if(this->m_TypeChecks)
		this->Write<ushort>('UI');

	this->Write<uint>(aUInt);
}

void MN_WriteMessage::WriteULong(ulong aULong)
{
	this->WriteUInt(aULong);
}

void MN_WriteMessage::WriteUInt64(uint64 aUInt64)
{
	if(this->m_TypeChecks)
		this->Write<ushort>('U6');

	this->Write<uint64>(aUInt64);
}

void MN_WriteMessage::WriteFloat(float aFloat)
{
	if(this->m_TypeChecks)
		this->Write<ushort>('FL');

	this->Write<float>(aFloat);
}

void MN_WriteMessage::WriteRawData(PVOID aBuffer, uint aBufferSize)
{
	assert(aBuffer && aBufferSize > 0);

	if(this->m_TypeChecks)
		this->Write<ushort>('RD');

	this->WriteUShort(aBufferSize);
	this->CheckWriteSize(aBufferSize);

	memcpy((voidptr_t)this->m_WritePtr, aBuffer, aBufferSize);

	this->IncWritePos(aBufferSize);
}

void MN_WriteMessage::WriteString(char *aBuffer)
{
	this->WriteString(aBuffer, strlen(aBuffer) + 1);
}

void MN_WriteMessage::WriteString(char *aBuffer, uint aStringSize)
{
	assert(aBuffer && aStringSize > 0);

	ushort bufferSize = aStringSize * sizeof(char);

	this->WriteUShort(aStringSize);
	this->CheckWriteSize(bufferSize);

	memcpy((voidptr_t)this->m_WritePtr, aBuffer, bufferSize);

	this->IncWritePos(bufferSize);
}

void MN_WriteMessage::WriteString(wchar_t *aBuffer)
{
	this->WriteString(aBuffer, wcslen(aBuffer) + 1);
}

void MN_WriteMessage::WriteString(wchar_t *aBuffer, uint aStringSize)
{
	assert(aBuffer && aStringSize > 0);

	ushort bufferSize = aStringSize * sizeof(wchar_t);

	this->WriteUShort(aStringSize);
	this->CheckWriteSize(bufferSize);

	memcpy((voidptr_t)this->m_WritePtr, aBuffer, bufferSize);

	this->IncWritePos(bufferSize);
}

bool MN_WriteMessage::SendMe(SOCKET sock)
{
	return this->SendMe(sock, false);
}

bool MN_WriteMessage::SendMe(SOCKET sock, bool aClearData)
{
	bool retVal = send(sock, (const char *)this->m_PacketData, this->m_DataLen, 0) != SOCKET_ERROR;

	if(aClearData)
	{
		this->m_WritePtr	= this->m_PacketData + sizeof(ushort);
		this->m_WritePos	= 0;
		this->m_DataLen		= 2;
	}

	return retVal;
}

PVOID MN_WriteMessage::GetDataStream()
{
	return (PVOID)this->m_PacketData;
}

uint MN_WriteMessage::GetDataLength()
{
	return this->m_DataLen;
}

void MN_WriteMessage::CheckWriteSize(uint aSize)
{
	assert((this->m_DataLen + aSize) < this->m_PacketMaxSize && "Packet write exceeded bounds.");
}

void MN_WriteMessage::IncWritePos(uint aSize)
{
	this->m_WritePtr	= this->m_WritePtr + aSize;
	this->m_WritePos	+= aSize;
	this->m_DataLen		+= aSize;

	*(ushort *)this->m_PacketData = (ushort)(this->m_DataLen - sizeof(ushort));
}