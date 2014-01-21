#pragma once

#define CLASS_SINGLE(name)	class name *MC_Singleton<class name>::ourInstance = nullptr; \
							class name : public MC_Singleton<class name>

template<typename T>
class MC_Singleton
{
public:
	static T *ourInstance;

	static T *Create()
	{
		if(ourInstance == nullptr)
			ourInstance = new T();

		return ourInstance;
	}
};