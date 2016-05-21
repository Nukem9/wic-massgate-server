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

// Limits
#define WIC_NAME_MAX_LENGTH			25
#define WIC_EMAIL_MAX_LENGTH		64
#define WIC_PASSWORD_MAX_LENGTH		16
#define WIC_COUNTRY_MAX_LENGTH		5
#define WIC_MOTTO_MAX_LENGTH		256
#define WIC_HOMEPAGE_MAX_LENGTH		256

#define WIC_SERVER_MAX_CLIENTS		100

// Times
#define WIC_DEFAULT_NET_TIMEOUT		120000 /* Milliseconds */
#define WIC_DEFAULT_NET_TIMEOUT_S	(WIC_DEFAULT_NET_TIMEOUT / 1000)
#define WIC_LOGGEDIN_NET_TIMEOUT	360000 /* Milliseconds */
#define WIC_LOGGEDIN_NET_TIMEOUT_S	(WIC_LOGGEDIN_NET_TIMEOUT / 1000)
#define WIC_HEARTBEAT_NET_TIMEOUT	130000 /* Server heartbeat delay + 10s */
#define WIC_HEARTBEAT_NET_TIMEOUT_S	(WIC_LOGGEDIN_NET_TIMEOUT / 1000)
#define WIC_CREDAUTH_RESEND			300000 /* Credential request delay */
#define WIC_CREDAUTH_RESEND_S		(WIC_CREDAUTH_RESEND / 1000)

#define WIC_CLIENT_PPS				10 /* Pings per second (MMG_Messaging) */

// Product information
#define WIC_PRODUCT_ID				1
#define WIC_CLIENT_DISTRIBUTION		1
#define WIC_DEBUG_DISTRIBUTION		2

// Version
#define WIC_CURRENT_VERSION			VER_1011

// Account values
#define WIC_INVALID_ACCOUNT			((uint)-1)

// modify profile operation values
#define MODIFY_PROFILE_ADD			6382692
#define MODIFY_PROFILE_DELETE		6579564

// Ports
#define WIC_HTTP_PORT				80

#define WIC_LIVEACCOUNT_PORT		3001
#define WIC_LIVEACCOUNT_DEBUG_PORT	3003

#define WIC_STATS_PORT				3006