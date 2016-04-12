#pragma once

CLASS_SINGLE(MMG_TrackableServer)
{
public:
	class ThreadedPingHandler
	{
	public:
	private:
	public:
		ThreadedPingHandler();
	};

private:

public:
	MMG_TrackableServer();

	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
};