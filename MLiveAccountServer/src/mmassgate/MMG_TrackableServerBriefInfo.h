#pragma once

class MMG_TrackableServerBriefInfo : public MMG_IStreamable
{
public:
	wchar_t m_GameName[64];
	uint m_IP;
	uint m_ModId;
	uint m_ServerId;
	ushort m_MassgateCommPort;
	uint64 m_CycleHash;
	uchar m_ServerType;
	bool m_IsRankBalanced;

private:

public:
	MMG_TrackableServerBriefInfo();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};