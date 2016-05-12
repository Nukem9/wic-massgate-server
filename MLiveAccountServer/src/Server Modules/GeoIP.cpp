#include "../stdafx.h"

#define GeoIPLog(format, ...) DebugLog(L_INFO, "[GeoIP]: "format, __VA_ARGS__)

#define TOTAL_RECORDS 130899

namespace GeoIP
{
	GEOIP_DATA data[TOTAL_RECORDS];

	bool Initialize()
	{
		memset(data, 0, sizeof(data));

		//read geo ip database into array/struct
		if(!ReadFile("GeoIP_database.bin"))
			return false;

		GeoIPLog("loaded GeoIP data");

		return true;
	}

	void Unload()
	{
		//delete all variables
	}

	bool ReadFile(char *filename)
	{
		FILE *f;
		GEOIP_DATA rec;

		memset(data, 0, sizeof(data));

		f = fopen(filename, "rb");

		if(!f)
		{
			GeoIPLog("could not open geoip file '%s'", filename);
			return false;
		}

		for(int i = 0; i < TOTAL_RECORDS; i++)
		{
			fread(&rec, sizeof(GEOIP_DATA), 1, f);

			data[i].from = rec.from;
			data[i].to = rec.to;
			strcpy_s(data[i].code, rec.code);
		}

		fclose(f);

		return true;
	}

	char *ClientLocateIP(uint IpAddress)
	{
		int imin = 0;
		int imax = TOTAL_RECORDS;
		
		while (imin <= imax)
		{
			int imid = (imin + imax) >> 1;
			if ((IpAddress >= data[imid].from) && (IpAddress <= data[imid].to))
			{
				if (!strcmp(data[imid].code,"-"))		//if the country of an IP range is not specified, output "US" as default country
					return "US";
				else
					return data[imid].code;
			}
			else if (IpAddress >= data[imid].from)
			{
				imin = imid + 1;
			}
			else
			{
				imax = imid - 1;
			}
		}
	}
};