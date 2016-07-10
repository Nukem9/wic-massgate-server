#pragma once

#define _USE_32BIT_TIME_T

#include <Windows.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>
#include <assert.h>
#include <intrin.h>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "libeay32.lib")

/***** Constants & type definitions *****/
#define VAR_STRING(a) #a

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned __int32	uint32;
typedef unsigned __int64	uint64;
typedef unsigned long		ulong;
typedef void				*voidptr_t;
#ifdef _M_IX86
typedef __w64 unsigned long sizeptr_t;
#else
typedef unsigned long long	sizeptr_t;
#endif
/****************************************/

#ifdef _M_IX86
#define SERVER_ARCHITECTURE "x86"
#else
#define SERVER_ARCHITECTURE "x86_64"
#endif

// Included/used libraries
#include "../libraries/stdafx.h"

// Misc
#include "misc/stdafx.h"

// MCommon2
#include "mcommon2/stdafx.h"

// MNetwork
#include "mnetwork/stdafx.h"

// MMassgate
#include "mmassgate/stdafx.h"

// Server modules
#include "Server Modules/stdafx.h"