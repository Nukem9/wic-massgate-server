#include "../stdafx.h"

MMG_OptionalContentProtocol::MMG_OptionalContentProtocol()
{
}

void MMG_OptionalContentProtocol::ToStream(MN_WriteMessage *aMessage)
{
	// Map count
	aMessage->WriteUInt(1);

	// File size
	aMessage->WriteUInt64(0x1000000);

	aMessage->WriteString("Map Name Here");
	aMessage->WriteString("Map URL here.ice");

	// File hash
	aMessage->WriteUInt(0x00023523);
}