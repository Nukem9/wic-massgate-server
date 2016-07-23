#pragma once

CLASS_SINGLE(MMG_ProfileEditableVariablesProtocol) 
{
public:
	class GetRsp
	{
	public:
		wchar_t motto[WIC_MOTTO_MAX_LENGTH];
		wchar_t homepage[WIC_HOMEPAGE_MAX_LENGTH];

		GetRsp();

		void ToStream	(MN_WriteMessage *aMessage);
		bool FromStream	(MN_ReadMessage *aMessage);
	};

private:

public:
	MMG_ProfileEditableVariablesProtocol();
};