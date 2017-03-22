#pragma once

CLASS_SINGLE(MMG_TrackableServer)
{
public:
	class Server
	{
	public:
		// Identification
		uint	m_PublicId;
		uint	m_AddressHash;

		// Authorization
		bool	m_KeyAuthenticated;
		uint	m_KeySequence;
		uint	m_QuizAnswer;

		// Server listing information
		MMG_ServerStartupVariables		m_Info;
		MMG_TrackableServerHeartbeat	m_Heartbeat;
		MMG_TrackableServerCookie		m_Cookie;

		Server(uint SourceIp, uint SourcePort) : m_Info(), m_Heartbeat(), m_Cookie()
		{
			this->m_PublicId			= 0;
			this->m_AddressHash			= SourceIp ^ (SourcePort + 0x1010101);

			this->m_KeyAuthenticated	= false;
			this->m_KeySequence			= 0;
			this->m_QuizAnswer			= 0;

			this->m_Cookie.trackid		= rand() * 1029384756i64;
			this->m_Cookie.hash			= (m_AddressHash << 30) | (MI_Time::GetTick() & 0xFFFFFFFF);
		}

		bool TestIPHash(uint SourceIp, uint SourcePort)
		{
			return (SourceIp ^ (SourcePort + 0x1010101)) == this->m_AddressHash;
		}
	};

private:
	MT_Mutex m_Mutex;
	std::vector<Server> m_ServerList;

public:
	MMG_TrackableServer();

	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
	bool GetServerListInfo(MMG_ServerFilter *aFilters, std::vector<MMG_TrackableServerFullInfo> *aFullInfo, std::vector<MMG_TrackableServerBriefInfo> *aBriefInfo, uint *aTotalCount);
	void DisconnectServer(SvClient *aClient);

private:
	Server *FindServer(SvClient *aClient);
	bool AuthServer(SvClient *aClient, uint aKeySequence, ushort aProtocolVersion);
	bool ConnectServer(Server *aServer, uint aSourceIp, MMG_ServerStartupVariables *aStartupVars);
	bool UpdateServer(Server *aServer, MMG_TrackableServerHeartbeat *aHeartbeat);

	bool FilterCheck(MMG_ServerFilter *filters, Server *aServer);
};