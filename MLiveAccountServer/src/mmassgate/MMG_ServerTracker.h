#pragma once

CLASS_SINGLE(MMG_ServerTracker)
{
public:

private:

public:
	MMG_ServerTracker();

	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
};