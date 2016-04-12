#pragma once

CLASS_SINGLE(MMG_ServerTracker)
{
public:
	enum ServerType : int
	{
		NORMAL_SERVER,
		MATCH_SERVER,
		FPM_SERVER,
		TOURNAMENT_SERVER,
		CLANMATCH_SERVER
	};

	class Pinger
	{
	public:
		uint64 m_StartTimeStamp;
		uchar m_SequenceNumber;
		uchar m_GetExtendedInfoFlag;
		sockaddr_in m_Target;
	private:
	public:
		Pinger();
	};

private:

public:
	MMG_ServerTracker();

	bool PrivHandlePingers();
	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
};