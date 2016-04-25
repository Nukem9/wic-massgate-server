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
		int		m_KeySequence;
		int		m_QuizAnswer;

		// Server listing information
		MMG_TrackableServerCookie	m_Cookie;
		MMG_ServerStartupVariables	m_Info;

		Server(uint SourceIp, uint SourcePort) : m_Info()
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

	bool MMG_TrackableServer::AuthServer(SvClient *aClient, uint aKeySequence, ushort aProtocolVersion);
	bool MMG_TrackableServer::ConnectServer(MMG_TrackableServer::Server *aServer, MMG_ServerStartupVariables *aStartupVars);
	void MMG_TrackableServer::DisconnectServer(MMG_TrackableServer::Server *aServer);
	bool MMG_TrackableServer::UpdateServer(MMG_TrackableServer::Server *aServer, MMG_ServerStartupVariables *StartupVars, uint *ServerId);

	Server *FindServer(SvClient *aClient);

public:
	MMG_TrackableServer();

	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
};