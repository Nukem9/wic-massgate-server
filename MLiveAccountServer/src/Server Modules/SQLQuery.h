#pragma once

class SQLQuery
{
private:
	sqlite3_stmt *m_Statement; 

public:
	SQLQuery(sqlite3 *db, const char *SQLText) : m_Statement(nullptr)
	{
		this->m_Statement = nullptr;

		int result = sqlite3_prepare_v2(db, SQLText, -1, &m_Statement, nullptr);

		if(result != SQLITE_OK)
			DebugBreak();
	}

	virtual ~SQLQuery()
	{
		this->Finalize();
	}

	sqlite3_stmt *GetStatement()
	{
		return this->m_Statement;
	}

	bool Bind(int Argument, int Value)
	{
		return sqlite3_bind_int(this->m_Statement, Argument, Value) == SQLITE_OK;
	}

	bool Bind(int Argument, __int64 Value)
	{
		return sqlite3_bind_int64(this->m_Statement, Argument, Value) == SQLITE_OK;
	}

	bool Bind(int Argument, double Value)
	{
		return sqlite3_bind_double(this->m_Statement, Argument, Value) == SQLITE_OK;
	}

	bool Bind(int Argument, const char *Value)
	{
		return sqlite3_bind_text(this->m_Statement, Argument, Value, (int)strlen(Value), nullptr) == SQLITE_OK;
	}

	bool Bind(int Argument, const wchar_t *Value)
	{
		return sqlite3_bind_text16(this->m_Statement, Argument, Value, (int)wcslen(Value), nullptr) == SQLITE_OK;
	}

	bool BindBlob(int Argument, voidptr_t Data, ulong Length)
	{
		return sqlite3_bind_blob(this->m_Statement, Argument, Data, Length, nullptr) == SQLITE_OK;
	}

	bool BindNull(int Argument)
	{
		return sqlite3_bind_null(this->m_Statement, Argument) == SQLITE_OK;
	}

	int Step()
	{
		return sqlite3_step(this->m_Statement);
	}

	void Finalize()
	{
		if(this->m_Statement)
			sqlite3_finalize(this->m_Statement);

		this->m_Statement = nullptr;
	}
};