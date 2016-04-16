#pragma once

CLASS_SINGLE(MMG_TrackableServer)
{
public:

private:

public:
	MMG_TrackableServer();

	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
};

struct MMG_TrackableServerCookie
{
	union
	{
		uint64 contents[2];

		struct
		{
			uint64 trackid;
			uint64 hash;
		};
	};
};