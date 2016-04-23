#pragma once

class MySQLQuery
{
private:
	MYSQL_STMT *m_Statement;

public:
	MySQLQuery(MYSQL *con, const char *SQLText) : m_Statement(nullptr)
	{
		this->m_Statement = nullptr;

		this->m_Statement = mysql_stmt_init(con);
		
		if(!this->m_Statement)
			DebugLog(L_ERROR, "mysql_stmt_init(), out of memory.");

		if (mysql_stmt_prepare(this->m_Statement, SQLText, strlen(SQLText)) != 0)
		{
			DebugLog(L_ERROR, "mysql_stmt_prepare(), invalid SQL statement.");
			DebugLog(L_ERROR, "%s", mysql_stmt_error(this->m_Statement));
		}
	}

	virtual ~MySQLQuery()
	{
		this->Finalize();
	}

	MYSQL_STMT *GetStatement()
	{
		return this->m_Statement;
	}
	
	/* mysql internal data types */
	//TINYINT
	void Bind(MYSQL_BIND &param, const uchar &Value)
	{
		param.buffer_type= MYSQL_TYPE_TINY;
		param.buffer= (char *)&Value;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//INT
	void Bind(MYSQL_BIND &param, const int &Value)
	{
		param.buffer_type= MYSQL_TYPE_LONG;
		param.buffer= (char *)&Value;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//INT
	void Bind(MYSQL_BIND &param, const uint &Value)
	{
		param.buffer_type= MYSQL_TYPE_LONG;
		param.buffer= (char *)&Value;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//INT
	void Bind(MYSQL_BIND &param, const ulong &Value)
	{
		param.buffer_type= MYSQL_TYPE_LONG;
		param.buffer= (char *)&Value;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//BIGINT
	void Bind(MYSQL_BIND &param, const __int64 &Value)
	{
		param.buffer_type= MYSQL_TYPE_LONGLONG;
		param.buffer= (char *)&Value;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//FLOAT
	void Bind(MYSQL_BIND &param, const float &Value)
	{
		param.buffer_type= MYSQL_TYPE_FLOAT;
		param.buffer= (char *)&Value;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//DOUBLE
	void Bind(MYSQL_BIND &param, const double &Value)
	{
		param.buffer_type= MYSQL_TYPE_DOUBLE;
		param.buffer= (char *)&Value;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//VARCHAR (multibyte)
	void Bind(MYSQL_BIND &param, const char Value[], ulong &str_length)
	{
		param.buffer_type= MYSQL_TYPE_VAR_STRING;
		param.buffer= (char *)&Value[0];
		param.buffer_length= 100; //temporary
		param.is_null= (my_bool*)0;
		param.length= &str_length;

		str_length = strlen(Value);
	}

	//VARCHAR (unicode)
	void Bind(MYSQL_BIND &param, const wchar_t Value[], ulong &str_length)
	{
		param.buffer_type= MYSQL_TYPE_VAR_STRING;
		param.buffer= (char *)&Value[0];
		param.buffer_length= 100; //temporary
		param.is_null= (my_bool*)0;
		param.length= &str_length;

		str_length= wcslen(Value) * 2;
	}

	//TIME - MYSQL_TYPE_TIME
	void BindTime(MYSQL_BIND &param, MYSQL_TIME datetime)
	{
		param.buffer_type= MYSQL_TYPE_TIME;
		param.buffer= (char *)&datetime;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//DATE - MYSQL_TYPE_DATE
	void BindDate(MYSQL_BIND &param, MYSQL_TIME datetime)
	{
		param.buffer_type= MYSQL_TYPE_DATE;
		param.buffer= (char *)&datetime;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//DATETIME - MYSQL_TYPE_DATETIME
	void BindDateTime(MYSQL_BIND &param, MYSQL_TIME datetime)
	{
		param.buffer_type= MYSQL_TYPE_DATETIME;
		param.buffer= (char *)&datetime;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//TIMESTAMP - MYSQL_TYPE_TIMESTAMP
	void BindTimeStamp(MYSQL_BIND &param, MYSQL_TIME datetime)
	{
		param.buffer_type= MYSQL_TYPE_TIMESTAMP;
		param.buffer= (char *)&datetime;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//BLOB
	void BindBlob(MYSQL_BIND &param, voidptr_t Data, ulong Length)
	{
		//mysql_stmt_send_long_data(stmt, 2, (const char*)Data, Length)

		param.buffer_type= MYSQL_TYPE_BLOB;
		param.buffer= (char *)Data;
		param.buffer_length= Length;
		param.is_null= (my_bool*)0;
		param.length= 0;
	}

	//NULL
	void BindNull(MYSQL_BIND &param)
	{
		param.buffer_type= MYSQL_TYPE_NULL;
		//param.buffer= (char *)nullptr;
		param.is_null= (my_bool*)1;
		param.length= 0;
	}

	bool StmtExecute(MYSQL_BIND param[])
	{
		if(this->m_Statement)
		{
			if (mysql_stmt_bind_param(this->m_Statement, param))
			{
				DebugLog(L_ERROR, "StmtExecute(param): mysql_stmt_bind_param() failed.");
				DebugLog(L_ERROR, "%s", mysql_stmt_error(this->m_Statement));
				return false;
			}

			if (mysql_stmt_execute(this->m_Statement))
			{
				DebugLog(L_ERROR, "StmtExecute(param): mysql_stmt_execute() failed.");
				DebugLog(L_ERROR, "%s", mysql_stmt_error(this->m_Statement));
				return false;
			}
		}
		
		return true;
	}

	bool StmtExecute(MYSQL_BIND param[], MYSQL_BIND result[])
	{
		if(this->m_Statement)
		{
			if (mysql_stmt_bind_param(this->m_Statement, param))
			{
				DebugLog(L_ERROR, "StmtExecute(param, result): mysql_stmt_bind_param() failed.");
				DebugLog(L_ERROR, "%s", mysql_stmt_error(this->m_Statement));
				return false;
			}

			if (mysql_stmt_bind_result(this->m_Statement, result))
			{
				DebugLog(L_ERROR, "mysql_stmt_bind_result() failed.");
				DebugLog(L_ERROR, "%s", mysql_stmt_error(this->m_Statement));
				return false;
			}

			if (mysql_stmt_execute(this->m_Statement))
			{
				DebugLog(L_ERROR, "StmtExecute(param, result): mysql_stmt_execute() failed.");
				DebugLog(L_ERROR, "%s", mysql_stmt_error(this->m_Statement));
				return false;
			}

			//this step is optional, but in order to use mysql_stmt_num_rows() then mysql_stmt_store_result() must be called
			if (mysql_stmt_store_result(this->m_Statement))
			{
				DebugLog(L_ERROR, "mysql_stmt_store_result() failed.");
				DebugLog(L_ERROR, "%s", mysql_stmt_error(this->m_Statement));
				return false;
			}
		}
		
		return true;
	}

	bool StmtFetch()
	{
		if(this->m_Statement)
		{
			if (mysql_stmt_fetch(this->m_Statement))
				return false;
		}
		
		return true;
	}

	int Step()
	{
		if (this->m_Statement)
		{
			if (mysql_stmt_execute(this->m_Statement))
			{
				DebugLog(L_ERROR, "Step(): mysql_stmt_execute() failed.");
				DebugLog(L_ERROR, "%s", mysql_stmt_error(this->m_Statement));
				return false;
			}
		}

		return true;
	}

	void Finalize()
	{
		if (this->m_Statement)
		{
			if(mysql_stmt_close(this->m_Statement))
			{
				DebugLog(L_ERROR, "mysql_stmt_close() failed.");
				DebugLog(L_ERROR, "%s", mysql_stmt_error(this->m_Statement));
			}
		}
		
		this->m_Statement = nullptr;
	}
};