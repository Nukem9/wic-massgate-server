#pragma once

class MMG_ProfileEditableVariablesProtocol : public MMG_IStreamable
{
public:
	wchar_t m_Motto[256];
	wchar_t m_Homepage[256];
	
private:

public:
	MMG_ProfileEditableVariablesProtocol();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};