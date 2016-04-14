#pragma once

class MMG_ProfileGuestBookEntry : public MMG_IStreamable
{
public:
	wchar_t m_GuestBookMessage[128];
	uint m_TimeStamp;
	uint m_ProfileId;
	uint m_MessageId;

private:

public:
	MMG_ProfileGuestBookEntry();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};