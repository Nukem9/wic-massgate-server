#pragma once

class MMG_TrackablePlayerVariables : public MMG_IStreamable
{
public:
	uint m_ProfileId;
	uint m_Score;

private:

public:
	MMG_TrackablePlayerVariables();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};