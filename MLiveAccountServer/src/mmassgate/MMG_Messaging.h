#pragma once

CLASS_SINGLE(MMG_Messaging)
{
public:
	class IM_Settings : public MMG_IStreamable
	{
	public:
		uchar m_Friends;
		uchar m_Clanmembers;
		uchar m_Acquaintances;
		uchar m_Anyone;

	private:

	public:
		void ToStream	(MN_WriteMessage *aMessage);
		bool FromStream	(MN_ReadMessage *aMessage);

	private:
	};

	class ProfileStateObserver : public MC_Observer
	{
	public:
		void update(MC_Subject *subject, StateType type);
	};

private:

public:
	MMG_Messaging();

	bool HandleMessage				(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
	bool SendProfileName			(SvClient *aClient, MN_WriteMessage *aMessage, MMG_Profile *aProfile);
	bool SendInstantMessage			(MMG_InstantMessageListener::InstantMessage *message);
	bool SendCommOptions			(SvClient *aClient, MN_WriteMessage *aMessage);
	bool SendIMSettings				(SvClient *aClient, MN_WriteMessage *aMessage);
	bool SendPingsPerSecond			(SvClient *aClient, MN_WriteMessage *aMessage);
	bool SendStartupSequenceComplete(SvClient *aClient, MN_WriteMessage *aMessage);
	bool SendOptionalContent		(SvClient *aClient, MN_WriteMessage *aMessage);
	bool SendClanWarsFilterWeights	(SvClient *aClient, MN_WriteMessage *aMessage);
};