#pragma once

typedef struct GEOIP_DATA
{
	ulong from;
	ulong to;
	char code[5];
} GEOIP_DATA;

namespace GeoIP
{
	bool	Initialize			();
	void	Unload				();
	bool	ReadFile			(char *filename);

	char	*ClientLocateIP		(uint IpAddress);
}