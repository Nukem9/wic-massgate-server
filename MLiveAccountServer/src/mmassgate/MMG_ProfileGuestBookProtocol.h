#pragma once

class MMG_ProfileGuestBookProtocol : public MMG_IStreamable
{
public:
	uint m_RequestId;
	uchar m_IgnoresGettingProfile;
	uint m_Count;
	uint m_MaxSize;
	uint m_Data;
	MMG_ProfileGuestBookEntry m_GuestBookEntry[1]; // probably needs a linked list as well, max: 32
	
private:

public:
	MMG_ProfileGuestBookProtocol() : m_GuestBookEntry()
	{
		this->m_RequestId = 0;
		this->m_IgnoresGettingProfile = 0;
		this->m_Count = 0;
		this->m_MaxSize = 0;
		this->m_Data = 0;
	}

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};