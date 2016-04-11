#include "../stdafx.h"

#include <fstream>
#include <string>

#define DatabaseLog(format, ...) DebugLog(L_INFO, "[Database]: "format, __VA_ARGS__)

/*
	MySQL C API - Documentation
	https://dev.mysql.com/doc/refman/5.7/en/c-api.html

	MySQL C API Prepared Statements
	https://dev.mysql.com/doc/refman/5.7/en/c-api-prepared-statements.html

	NOTES: -taken directly from the documentation
	1)	To obtain a statement handle, pass a MYSQL connection handler to mysql_stmt_init(), which returns a pointer to a MYSQL_STMT data structure. 
		This structure is used for further operations with the statement. 
		To specify the statement to prepare, pass the MYSQL_STMT pointer and the statement string to mysql_stmt_prepare().
			- The string must consist of a single SQL statement. You should not add a terminating semicolon (";") or \g to the statement.
			  See: https://dev.mysql.com/doc/refman/5.7/en/mysql-stmt-prepare.html

		*The MYSQLQuery constuctor calls mysql_stmt_init() and mysql_stmt_prepare()

	2)	To provide input parameters for a prepared statement, set up MYSQL_BIND structures and pass them to mysql_stmt_bind_param(). 
		To receive output column values, set up MYSQL_BIND structures and pass them to mysql_stmt_bind_result().
		See:https://dev.mysql.com/doc/refman/5.7/en/c-api-prepared-statement-data-structures.html

		*after using Bind(), you can use StmtExecute() to execute your prepared statement
		*otherwise you can use Step() if its not a prepared statement
		*use StmtFetch() to get the results

	3)	Multiple statement handles can be associated with a single connection. The limit on the number of handles depends on the available system resources.

	4)	To use a MYSQL_BIND structure, zero its contents to initialize it, then set its members appropriately.

		*Bind has been overloaded for ease of use, to bind dates, nulls, and blobs, use BindDate(), BindNull() and BindBlob()

		*to use a statement that returns a result set, call StmtFetch() after calling StmtExecute()

	Currently i have to open a new connection for each thread that tries to execute a query because the library is not thread safe
	if i want use only one connection for all queries, i will need to use a mutex when executing prepared statements from each client
	https://dev.mysql.com/doc/refman/5.7/en/c-api-threaded-clients.html
	https://dev.mysql.com/doc/refman/5.7/en/mysql-thread-init.html
	https://dev.mysql.com/doc/refman/5.7/en/mysql-thread-end.html

	The MySQLQuery class is basically a wrapper that utilises all of the functions needed to execute a prepared statement
	the only problem is that a new connection is required for each thread that makes a query.
	A thread CAN execute multiple queries on the same connection.

	todo:
	- save/get cipher keys to/from database
	- do checkcdkeyexists function
	- rename database fields to something that better describes what they are
		emailme, acceptsemail, isbanned, cdkey
		experience, onlinestatus can probably be removed
	- CreateUserProfile, DeleteUserProfile and QueryUserProfile are a little messy but are sufficient for now

*/

namespace MySQLDatabase
{
	char* host;
	char* user;
	char* pass;
	char* db;

	bool Initialize()
	{
		//mysql_library_init(); //doesnt work with braces :S

		if(!ReadConfig())
			return false;

		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}
		
		DatabaseLog("client version: %s started", mysql_get_client_info());
		DatabaseLog("connection to %s ok", host);

		mysql_close(con);
		mysql_thread_end();

		return true;
	}

	void Unload()
	{
		free((char*)host);
		free((char*)user);
		free((char*)pass);
		free((char*)db);

		//mysql_library_end();
	}

	bool ReadConfig()
	{
		//might need to change working directory in property pages if it cant find the file
		std::ifstream file("settings.ini", std::ios::in);
		std::string line;
		std::string settings[4];

		int i=0;

		if(file.is_open())
		{
			while(getline(file, line))
			{
				if(line.substr(0, 1)!="[" && line != "")
				{
					settings[i] = line;
					i++;
				}
			}
		} else {
			DatabaseLog("could not open configuration file 'settings.ini'");
			return false;
		}

		file.close();

		host = (char*)malloc(settings[0].size() + 1);
		memcpy((char*)host, settings[0].c_str(), settings[0].size() + 1);

		user = (char*)malloc(settings[1].size() + 1);
		memcpy((char*)user, settings[1].c_str(), settings[1].size() + 1);

		pass = (char*)malloc(settings[2].size() + 1);
		memcpy((char*)pass, settings[2].c_str(), settings[2].size() + 1);

		db = (char*)malloc(settings[3].size() + 1);
		memcpy((char*)db, settings[3].c_str(), settings[3].size() + 1);

		return true;
	}

	bool InitializeSchema()
	{
		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}

		// Go through each table entry
		for(int i = 0; i < ARRAYSIZE(MySQLTableValues); i++)
		{
			MySQLQuery query(con, MySQLTableValues[i]);
		
			if (!query.Step())
				return false;
		}

		DatabaseLog("database tables created OK");

		mysql_close(con);
		mysql_thread_end();

		return true;
	}

	bool CheckIfEmailExists(const char *email, uint *dstId)
	{
		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}

		char *sql = "SELECT id FROM mg_accounts WHERE email = ?";

		MySQLQuery query(con, sql);
		MYSQL_BIND param[1], result[1];
		ulong email_len;
		uint id;
		bool querySuccess;

		memset(param, 0, sizeof(param));
		memset(result, 0, sizeof(result));

		query.Bind(param[0], email, email_len);		//bind 'email' to the first/only parameter

		query.Bind(result[0], id);		//mg_accounts.id

		if(!query.StmtExecute(param, result))
		{
			DatabaseLog("CheckIfEmailExists() query failed: %s", email);
			querySuccess = false;
			*dstId = 0;
		}
		else
		{
			if (!query.StmtFetch())
			{
				DatabaseLog("email %s not found", email);
				querySuccess = true;
				*dstId = 0;
			}
			else
			{
				DatabaseLog("email %s found", email);
				querySuccess = true;
				*dstId = id;
			}
		}
		
		mysql_close(con);
		mysql_thread_end();

		return querySuccess;
	}

	bool CheckIfCDKeyExists(uint *dstId)
	{
		*dstId = 0;
		return true;
	}

	bool CreateUserAccount(const char *email, const wchar_t *password, const char *country, const uchar *emailme, const uchar *acceptsemail)
	{
		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}

		char *sql = "INSERT INTO mg_accounts (email, password, country, emailme, acceptsemail, activeprofileid, isbanned, cdkey) VALUES (?, ?, ?, ?, ?, 0, 0, 0)";

		MySQLQuery query(con, sql);
		MYSQL_BIND params[5];
		ulong email_len, pass_len, country_len;
		bool querySuccess;

		memset(params, 0, sizeof(params));

		// TODO: save cipherkeys/cdkey
		query.Bind(params[0], email, email_len);		//email
		query.Bind(params[1], password, pass_len);		//password
		query.Bind(params[2], country, country_len);	//country
		query.Bind(params[3], *emailme);				//emailme (game related news and events)
		query.Bind(params[4], *acceptsemail);			//acceptsemail (from ubisoft (pfft) rename this field)

		if (!query.StmtExecute(params))
		{
			DatabaseLog("CreateUserAccount() query failed: %s", email);
			querySuccess = false;
		}
		else
		{
			DatabaseLog("created account: %s", email);
			querySuccess = true;
		}
		
		mysql_close(con);
		mysql_thread_end();

		return querySuccess;
	}

	bool AuthUserAccount(const char *email, wchar_t *dstPassword, uchar *dstIsBanned, MMG_AuthToken *authToken)
	{
		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}

		char *sql = "SELECT id, password, activeprofileid, isbanned FROM mg_accounts WHERE email = ?";

		MySQLQuery query(con, sql);
		MYSQL_BIND param[1], results[4];
		wchar_t password[16];
		uint id, activeprofileid;
		uchar isbanned;
		ulong email_len, pass_len;
		bool querySuccess;

		memset(password, 0, sizeof(password));

		memset(param, 0, sizeof(param));
		memset(results, 0, sizeof(results));
		
		query.Bind(param[0], email, email_len);		//email

		// TODO: get cipherkeys/cdkey
		query.Bind(results[0], id);					//mg_accounts.id
		query.Bind(results[1], password, pass_len);	//mg_accounts.password
		query.Bind(results[2], activeprofileid);	//mg_accounts.activeprofileid
		query.Bind(results[3], isbanned);			//mg_accounts.isbanned
		
		if(!query.StmtExecute(param, results))
		{
			DatabaseLog("AuthUserAccount() query failed: %s", email);
			querySuccess = false;

			authToken->m_AccountId = 0;
			authToken->m_ProfileId = 0;
			dstPassword = L"";
			*dstIsBanned = 0;
		}
		else
		{
			if (!query.StmtFetch())
			{
				DatabaseLog("account %s not found", email);
				querySuccess = true;

				authToken->m_AccountId = 0;
				authToken->m_ProfileId = 0;
				dstPassword = L"";
				*dstIsBanned = 0;
			}
			else
			{
				DatabaseLog("account %s found", email);
				querySuccess = true;

				authToken->m_AccountId = id;
				authToken->m_ProfileId = activeprofileid;
				wcscpy(dstPassword, password);
				*dstIsBanned = isbanned;
			}
		}
		
		mysql_close(con);
		mysql_thread_end();

		return querySuccess;
	}

	bool CheckIfProfileExists(const wchar_t* name, uint *dstId)
	{
		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}

		char *sql = "SELECT id FROM mg_profiles WHERE name = ?";

		MySQLQuery query(con, sql);
		MYSQL_BIND param[1], result[1];
		ulong name_len;
		uint id;
		bool querySuccess;

		memset(param, 0, sizeof(param));
		memset(result, 0, sizeof(result));

		query.Bind(param[0], name, name_len);

		query.Bind(result[0], id);		//mg_profiles.id

		if(!query.StmtExecute(param, result))
		{
			DatabaseLog("CheckIfProfileExists() query failed: %s", name);
			querySuccess = false;
			*dstId = 0;
		}
		else
		{
			if (!query.StmtFetch())
			{
				DatabaseLog("profile %s not found", name);
				querySuccess = true;
				*dstId = 0;
			}
			else
			{
				DatabaseLog("profile %s found", name);
				querySuccess = true;
				*dstId = id;
			}
		}
		
		mysql_close(con);
		mysql_thread_end();

		return querySuccess;
	}

	bool CreateUserProfile(const uint accountId, const wchar_t* name, const char* email)
	{
		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}

		//Step 1: create the profile as usual
		char *sql = "INSERT INTO mg_profiles (accountid, name, experience, rank, clanid, rankinclan, chatoptions, onlinestatus, lastlogindate) VALUES (?, ?, 0, 0, 0, 0, 0, 0, ?)";

		MySQLQuery query(con, sql);
		MYSQL_BIND params[3];
		ulong name_len, email_len;
		MYSQL_TIME datetime;
		ulong count;
		uint id, last_id;
		bool query1Success, query2Success, query3Success;

		datetime.year = 1970;
		datetime.month = 1;
		datetime.day = 1;
		datetime.hour = 0;
		datetime.minute = 0;
		datetime.second = 0;

		memset(params, 0, sizeof(params));

		query.Bind(params[0], accountId);			//account id
		query.Bind(params[1], name, name_len);	//profile name
		query.BindDateTime(params[2], datetime);	//last login date, set to 1/1/1970 for new accounts

		if (!query.StmtExecute(params))
		{
			DatabaseLog("%s create profile '%ws' failed for account id %d", email, name, accountId);
			query1Success = false;
			last_id = 0;
		}
		else
		{
			DatabaseLog("%s created profile %ws", email, name);
			query1Success = true;
			last_id = (uint)mysql_insert_id(con);
		}

		//Step 2: get profile count for current account
		if(query1Success)
		{
			char *sql2 = "SELECT mg_profiles.id "
						"FROM mg_profiles "
						"JOIN mg_accounts "
						"ON mg_profiles.accountid = mg_accounts.id "
						"WHERE mg_accounts.email = ? "
						"ORDER BY lastlogindate DESC, id ASC";

			MySQLQuery query2(con, sql2);
			MYSQL_BIND params2[1], results2[1];
			
			memset(params2, 0, sizeof(params2));
			memset(results2, 0, sizeof(results2));

			query2.Bind(params2[0], email, email_len);	//email

			query2.Bind(results2[0], id);				//mg_profiles.id

			if(!query2.StmtExecute(params2, results2))
			{
				DatabaseLog("CreateUserProfile() failed:");
				query2Success = false;
				count = 0;
			}
			else
			{
				query2Success = true;
				count = (ulong)mysql_stmt_num_rows(query2.GetStatement());
				DatabaseLog("found %d profiles", count);
			}
		}

		//Step 3: if there is only one profile for the account, set the only profile as the active profile
		if(count == 1)
		{
			char *sql3 = "UPDATE mg_accounts SET activeprofileid = ? WHERE id = ?";

			MySQLQuery query3(con, sql3);
			MYSQL_BIND params3[2];

			query3.Bind(params3[0], last_id);		//profileid
			query3.Bind(params3[1], accountId);		//account id
		
			if(!query3.StmtExecute(params3))
			{
				DatabaseLog("set active profile failed");
				query3Success = false;
				last_id = 0;
			}
			else
			{
				DatabaseLog("set active profile to %d OK", last_id);
				query3Success = true;
			}
		}
		
		mysql_close(con);
		mysql_thread_end();

		return query1Success && query2Success && query3Success;
	}

	bool DeleteUserProfile(const uint accountId, const uint profileId, const char* email)
	{
		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}

		//todo: (when forum is implemented) 
		//dont delete the profile, just rename the profile name
		//set an 'isdeleted' flag to 1
		//disassociate the profile with the account

		//Step 1: just delete the profile
		char *sql = "DELETE FROM mg_profiles WHERE id = ?";

		MySQLQuery query(con, sql);
		MYSQL_BIND param[1];
		ulong email_len;
		uint id;
		ulong count;
		bool query1Success, query2Success, query3Success;

		memset(param, 0, sizeof(param));

		query.Bind(param[0], profileId);		//profile id

		if (!query.StmtExecute(param))
		{
			DatabaseLog("DeleteUserProfile(query1) failed: profile id(%d)", email, profileId);
			query1Success = false;
		}
		else
		{
			DatabaseLog("%s deleted profile id(%d)", email, profileId);
			query1Success = true;
		}

		//Step 2: get profile count for current account
		//if(query1Success)
		//{
			char *sql2 = "SELECT mg_profiles.id "
						"FROM mg_profiles "
						"JOIN mg_accounts "
						"ON mg_profiles.accountid = mg_accounts.id "
						"WHERE mg_accounts.email = ? "
						"ORDER BY lastlogindate DESC, id ASC";

			MySQLQuery query2(con, sql2);
			MYSQL_BIND params2[1], results2[1];

			memset(params2, 0, sizeof(params2));
			memset(results2, 0, sizeof(results2));

			query2.Bind(params2[0], email, email_len);	//email

			query2.Bind(results2[0], id);				//mg_profiles.id

			if(!query2.StmtExecute(params2, results2))
			{
				DatabaseLog("DeleteUserProfile(query2) failed: account id(%d), profile id(%d)", accountId, profileId);
				query2Success = false;
				count = 0;
			}
			else
			{
				query2Success = true;
				count = (ulong)mysql_stmt_num_rows(query2.GetStatement());
			}
		//}

		//Step 3: set active profile to last used profile id, if there are no profiles set active profile id to 0
		if(count > 0)
		{

			//fetch first row from query2, this is the last used id, as it is sorted by date
			query2.StmtFetch();

			char *sql3 = "UPDATE mg_accounts SET activeprofileid = ? WHERE id = ?";

			MySQLQuery query3(con, sql3);
			MYSQL_BIND params3[2];

			query3.Bind(params3[0], id);			//last used id
			query3.Bind(params3[1], accountId);		//account id
		
			if(!query3.StmtExecute(params3))
			{
				DatabaseLog("DeleteUserProfile(query3) failed: account id(%d), profile id(%d)", accountId, profileId);
				query3Success = false;
			}
			else
			{
				DatabaseLog("account id(%d) set active profile id(0)", accountId);
				query3Success = true;
			}
		}
		else
		{
			char *sql3 = "UPDATE mg_accounts SET activeprofileid = 0 WHERE id = ?";

			MySQLQuery query3(con, sql3);
			MYSQL_BIND params3[1];

			query3.Bind(params3[0], accountId);		//account id
		
			if(!query3.StmtExecute(params3))
			{
				DatabaseLog("DeleteUserProfile(query3) failed: account id(%d), profile id(%d)", accountId, profileId);
				query3Success = false;
			}
			else
			{
				DatabaseLog("account id(%d) set active profile id(0)", accountId);
				query3Success = true;
			}
		}
		
		mysql_close(con);
		mysql_thread_end();
		
		return query1Success && query2Success && query3Success;
	}

	bool QueryUserProfile(const uint accountId, const uint profileId, MMG_Profile *profile)
	{
		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}

		//Step 1: get the requested profile
		char *sql = "SELECT id, name, rank, clanid, rankinclan FROM mg_profiles WHERE id = ?";

		MySQLQuery query(con, sql);
		MYSQL_BIND param[1], results[5];
		ulong name_len;
		wchar_t name[25];
		bool query1Success, query2Success, query3Success;

		memset(name, 0, sizeof(name));

		memset(param, 0, sizeof(param));
		memset(results, 0, sizeof(results));

		query.Bind(param[0], profileId);				//profile id

		query.Bind(results[0], profile->m_ProfileId);	//mg_profiles.id
		query.Bind(results[1], name, name_len);			//mg_profiles.name
		query.Bind(results[2], profile->m_Rank);		//mg_profiles.rank
		query.Bind(results[3], profile->m_ClanId);		//mg_profiles.clanid
		query.Bind(results[4], profile->m_RankInClan);	//mg_profiles.rankinclan

		if(!query.StmtExecute(param, results))
		{
			DatabaseLog("QueryUserProfile(query) failed: profile id(%d)", profileId);
			query1Success = false;
		}
		else
		{
			if (!query.StmtFetch())
			{
				DatabaseLog("profile id(%d) not found", profileId);
				query1Success = true;
			}
			else
			{
				wcscpy_s(profile->m_Name, name);

				DatabaseLog("profile id(%d) %ws found", profileId, profile->m_Name);
				query1Success = true;
			}
		}

		//Step 2: update last login date for the requested profile
		if(query1Success)
		{
			char *sql2 = "UPDATE mg_profiles SET lastlogindate = NOW() WHERE id = ?";

			MySQLQuery query2(con, sql2);
			MYSQL_BIND param2[1];

			query2.Bind(param2[0], profile->m_ProfileId);		//profile id
		
			if(!query2.StmtExecute(param2))
			{
				DatabaseLog("QueryUserProfile(query2) failed: profile id(%d)", profile->m_ProfileId);
				query2Success = false;
			}
			else
			{
				DatabaseLog("profile id(%d) %ws lastlogindate updated", profile->m_ProfileId, profile->m_Name);
				query2Success = true;
			}
		}

		//Step 3: set new requested profile as active for the account
		if(query2Success)
		{
			char *sql3 = "UPDATE mg_accounts SET activeprofileid = ? WHERE id = ?";

			MySQLQuery query3(con, sql3);
			MYSQL_BIND params3[2];

			query3.Bind(params3[0], profile->m_ProfileId);		//profileid
			query3.Bind(params3[1], accountId);		//account id
		
			if(!query3.StmtExecute(params3))
			{
				DatabaseLog("QueryUserProfile(query3) failed: account id(%d), profile id(%d)", accountId, profile->m_ProfileId);
				query3Success = false;
			}
			else
			{
				DatabaseLog("account id(%d) set active profile id(%d) %ws", accountId, profile->m_ProfileId, profile->m_Name);
				query3Success = true;
			}
		}

		mysql_close(con);
		mysql_thread_end();

		return query1Success && query2Success && query3Success;
	}

	bool RetrieveUserProfiles(const char *email, const wchar_t *password, ulong *dstProfileCount, MMG_Profile *profiles[])
	{
		MYSQL *con = mysql_init(NULL);

		if (mysql_real_connect(con, host, user, pass, db, 0, NULL, 0) == NULL)
		{
			DebugLog(L_ERROR, "%s", mysql_error(con));
			mysql_close(con);
			mysql_thread_end();

			return false;
		}

		char *sql = "SELECT mg_profiles.id, mg_profiles.name, mg_profiles.rank, mg_profiles.clanid, mg_profiles.rankinclan, mg_profiles.onlinestatus "
					"FROM mg_profiles "
					"JOIN mg_accounts "
					"ON mg_profiles.accountid = mg_accounts.id "
					"WHERE mg_accounts.email = ? "
					"ORDER BY lastlogindate DESC, id ASC";

		MySQLQuery query(con, sql);
		MYSQL_BIND params[1], results[6];
		ulong name_len, email_len;
		uint id, clanid, onlinestatus;
		uchar rank, rankinclan;
		wchar_t name[25];
		bool querySuccess;

		memset(name, 0, sizeof(name));

		memset(params, 0, sizeof(params));
		memset(results, 0, sizeof(results));

		query.Bind(params[0], email, email_len);		//email

		query.Bind(results[0], id);				//mg_profiles.id
		query.Bind(results[1], name, name_len);	//mg_profiles.name
		query.Bind(results[2], rank);			//mg_profiles.rank
		query.Bind(results[3], clanid);			//mg_profiles.clanid
		query.Bind(results[4], rankinclan);		//mg_profiles.rankinclan
		query.Bind(results[5], onlinestatus);	//mg_profiles.onlinestatus, do we need this in the database?

		if(!query.StmtExecute(params, results))
		{
			DatabaseLog("RetrieveUserProfiles() query failed: %s", email);
			querySuccess = false;

			MMG_Profile *tmp = new MMG_Profile();

			tmp->m_ProfileId = 0;
			wcscpy_s(tmp->m_Name, L"");
			tmp->m_Rank = 0;
			tmp->m_ClanId = 0;
			tmp->m_RankInClan = 0;
			tmp->m_OnlineStatus = 0;

			*profiles = tmp;
			*dstProfileCount = 0;
		}
		else
		{
			ulong count = (ulong)mysql_stmt_num_rows(query.GetStatement());
			DatabaseLog("%s: %d profiles found", email, count);
			querySuccess = true;

			if (count < 1)
			{
				MMG_Profile *tmp = new MMG_Profile();

				tmp->m_ProfileId = 0;
				wcscpy_s(tmp->m_Name, L"");
				tmp->m_Rank = 0;
				tmp->m_ClanId = 0;
				tmp->m_RankInClan = 0;
				tmp->m_OnlineStatus = 0;

				*profiles = tmp;
				*dstProfileCount = 0;
			}
			else
			{
				MMG_Profile *tmp = new MMG_Profile[count];
				int i = 0;

				while(query.StmtFetch())
				{
					tmp[i].m_ProfileId = id;
					wcscpy_s(tmp[i].m_Name, name);
					tmp[i].m_Rank = rank;
					tmp[i].m_ClanId = clanid;
					tmp[i].m_RankInClan = rankinclan;
					tmp[i].m_OnlineStatus = onlinestatus;

					i++;
				}

				*profiles = tmp;
				*dstProfileCount = count;
			}
		}
		
		mysql_close(con);
		mysql_thread_end();

		return querySuccess;
	}

	bool QueryUserOptions(const wchar_t *name, int *options)
	{
		return true;
	}

	bool QueryUserOptions(const uint profileId, int *options)
	{
		return true;
	}
}