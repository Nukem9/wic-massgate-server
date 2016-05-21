#pragma once

CLASS_SINGLE(MMG_AccountProxy)
{
public:
	MMG_AccountProxy();

	bool SetClientOffline	(SvClient *aClient);
	bool SetClientOnline	(SvClient *aClient);
	bool SetClientPlaying	(SvClient *aClient);
};