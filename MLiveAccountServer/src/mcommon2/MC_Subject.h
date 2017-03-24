#pragma once

class MC_Subject
{
public:
	typedef MC_Observer::StateType StateType;

private:
	std::vector<MC_Observer*> m_ObserverList;

public:
	void addObserver(MC_Observer* observer)
	{
		auto iter = std::find(m_ObserverList.begin(), m_ObserverList.end(), observer);

		if (iter != m_ObserverList.end())
	        return;

	    m_ObserverList.push_back(observer);
	}

	void removeObserver(MC_Observer* observer)
	{
		m_ObserverList.erase(std::remove_if(m_ObserverList.begin(), m_ObserverList.end(), [observer](MC_Observer* o)
		{
			return observer == o;
		}), m_ObserverList.end());
	}

protected:
	void notifyObservers(StateType type)
	{
		for (auto &observer : m_ObserverList)
			observer->update(this, type);
	}
};