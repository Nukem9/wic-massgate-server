#pragma once

CLASS_SINGLE(MMG_ClanGuestbookProtocol)
{
public:
	class GetRsp
	{
	public:
		class GuestbookEntry
		{
		public:
			wchar_t m_Message[WIC_GUESTBOOK_MAX_LENGTH];
			uint m_Timestamp;
			uint m_ProfileId;
			uint m_MessageId;
		private:

		public:
			GuestbookEntry();
		};

		uint m_ClanId;
		uint m_RequestId;
		GuestbookEntry m_Entries[32];

	private:

	public:
		GetRsp();

		void ToStream	(MN_WriteMessage *aMessage);
	};

private:

public:
	MMG_ClanGuestbookProtocol();
};