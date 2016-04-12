#pragma once

class MMG_TrackableServerHeartbeat : public MMG_IStreamable
{
public:

private:

public:
	MMG_TrackableServerHeartbeat();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};