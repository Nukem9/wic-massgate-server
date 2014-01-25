#pragma once

// Game versions (Build numbers)
enum WIC_REVISION_VER
{
	VER_1000 = 0,
	VER_1001 = 13,
	VER_1002 = 19,
	VER_1003 = 24,
	VER_1004 = 41,
	VER_1005 = 42,
	VER_1006 = 57,
	VER_1007 = 72,
	VER_1008 = 81,
	VER_1009 = 100,
	VER_1010 = 101,
	VER_1011 = 134,
};

// Protocol versions
enum WIC_PROTOCOL_VER
{
	PROTO_1011 = 140,
	PROTO_1012 = 150, // 'Custom' version
};

// Limits
#define WIC_NAME_MAX_LENGTH			25
#define WIC_EMAIL_MAX_LENGTH		128

#define WIC_SERVER_MAX_CLIENTS		100

// Times
#define WIC_DEFAULT_NET_TIMEOUT		120000 /* milliseconds */
#define WIC_DEFAULT_NET_TIMEOUT_S	(WIC_DEFAULT_NET_TIMEOUT / 1000)
#define WIC_LOGGEDIN_NET_TIMEOUT	300000 /* milliseconds */
#define WIC_LOGGEDIN_NET_TIMEOUT_S	(WIC_LOGGEDIN_NET_TIMEOUT / 1000)
#define WIC_CREDAUTH_RESEND			270000 /* Credential request delay */
#define WIC_CREDAUTH_RESEND_S		(WIC_CREDAUTH_RESEND / 1000)

#define WIC_CLIENT_PPS				10 /* Pings per second (MMG_Messaging) */

// Product information
#define WIC_PRODUCT_ID				1
#define WIC_CLIENT_DISTRIBUTION		1
#define WIC_DEBUG_DISTRIBUTION		2

// Protocol and versions
#define WIC_CURRENT_VERSION			VER_1011
#define WIC_CURRENT_PROTOCOL		PROTO_1012

// Account values
#define WIC_INVALID_ACCOUNT			((uint)-1)

// Ports
#define WIC_HTTP_PORT				80

#define WIC_LIVEACCOUNT_PORT		3001
#define WIC_LIVEACCOUNT_DEBUG_PORT	3003

#define WIC_STATS_PORT				3006