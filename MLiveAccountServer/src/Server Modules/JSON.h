#pragma once

class JSON
{
public:
	json_t *root;
	json_error_t error;

private:

public:
	JSON()
	{
		this->root = nullptr;
	}

	~JSON()
	{
		if (this->root)
			json_decref(root);
	}

	bool LoadJSON(const char *data)
	{
		this->root = json_loads(data, 0, &this->error);

		return this->root != nullptr;
	}

	//bool ReadInteger(const char *element, uint *value)
	//{
	//	return this->ReadInteger(element, (int *)value);
	//}

	bool ReadUChar(const char *element, uchar *value)
	{
		uint _ui_val = 0;

		if (!this->ReadUInt(element, &_ui_val))
			return false;

		if (value)
			*value = _ui_val & 0xFF;

		return true;
	}

	bool ReadUInt(const char *element, uint *value)
	{
		return this->ReadInt(element, (int *)value);
	}

	bool ReadInt(const char *element, int *value)
	{
		long long _ll_val = 0;

		if (!this->ReadLongLong(element, &_ll_val))
			return false;

		if (value)
			*value = _ll_val & 0xFFFFFFFF;

		return true;
	}

	bool ReadLongLong(const char *element, long long *value)
	{
		json_t *obj = json_object_get(this->root, element);

		if (!obj)
			return false;

		if (!json_is_integer(obj))
			return false;

		if (value)
			*value = json_integer_value(obj);

		return true;
	}

	bool ReadDouble(const char *element, double *value)
	{
		json_t *obj = json_object_get(this->root, element);

		if (!obj)
			return false;

		if (!json_is_real(obj))
			return false;

		if (value)
			*value = json_real_value(obj);

		return true;
	}

	bool ReadString(const char *element, char *buffer, size_t bufferSize)
	{
		json_t *obj = json_object_get(this->root, element);

		if (!obj)
			return false;

		if (!json_is_string(obj))
			return false;

		if (buffer)
			strcpy_s(buffer, bufferSize, json_string_value(obj));

		return true;
	}

	bool ReadString(const char *element, wchar_t *buffer, size_t bufferSize)
	{
		json_t *obj = json_object_get(this->root, element);

		if (!obj)
			return false;

		if (!json_is_string(obj))
			return false;

		if (buffer)
			MC_Misc::MultibyteToUnicode(json_string_value(obj), buffer, (uint)(bufferSize / sizeof(wchar_t)));

		return true;
	}

private:
};
