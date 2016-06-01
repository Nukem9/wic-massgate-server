#pragma once

#define TOTAL_RECORDS 130899

namespace GeoIP
{
	typedef struct GEOIP_DATA
	{
		ulong from;
		ulong to;
		char code[5];
	} GEOIP_DATA;

	static GEOIP_DATA data[TOTAL_RECORDS];

	bool	Initialize			();
	void	Unload				();
	bool	ReadFile			(char *filename);

	char	*ClientLocateIP		(uint IpAddress);
}