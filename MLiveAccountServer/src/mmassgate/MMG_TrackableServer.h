#pragma once

CLASS_SINGLE(MMG_TrackableServer)
{
public:
	class Server
	{
	public:
		bool	m_Valid;
		int		m_Index;

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
			this->m_Valid				= false;
			this->m_Index				= 0;

			this->m_KeyAuthenticated	= false;
			this->m_KeySequence			= 0;
			this->m_QuizAnswer			= 0;

			this->m_Cookie.trackid		= rand() * 1029384756i64;
			this->m_Cookie.hash			= SourceIp ^ (SourcePort + 0x1010101);
		}

		bool TestIPHash(uint SourceIp, uint SourcePort)
		{
			return (SourceIp ^ (SourcePort + 0x1010101)) == this->m_Cookie.hash;
		}
	};

private:
	std::vector<Server> m_ServerList;
	MT_Mutex m_Mutex;

	bool AuthServer(SvClient *aClient, uint aKeySequence, ushort aProtocolVersion);
	bool ConnectServer(Server *aServer, MMG_ServerStartupVariables *aStartupVars);
	bool UpdateServer(Server *aServer, MMG_TrackableServerHeartbeat *aHeartbeat);
	void DisconnectServer(Server *aServer);

	Server *FindServer(SvClient *aClient);

public:
	MMG_TrackableServer();

	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
};