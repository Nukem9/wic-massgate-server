#pragma once

CLASS_SINGLE(MMG_Chat)
{
public:

private:

public:
	MMG_Chat();

	bool HandleDelayedItems();
	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
};