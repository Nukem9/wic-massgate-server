#pragma once

CLASS_SINGLE(MMG_InstantMessageListener)
{
public:
	class InstantMessage : public MMG_IStreamable
	{
	public:
		uint m_MessageId;
		MMG_Profile m_SenderProfile;
		uint m_RecipientProfile;
		wchar_t m_Message[WIC_INSTANTMSG_MAX_LENGTH];
		uint m_WrittenAt;

	private:

	public:
		InstantMessage();

		void ToStream	(MN_WriteMessage *aMessage);
		bool FromStream	(MN_ReadMessage *aMessage);
	};

private:

public:
	MMG_InstantMessageListener();

};