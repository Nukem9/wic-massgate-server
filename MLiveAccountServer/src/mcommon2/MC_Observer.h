#pragma once

class MC_Subject;

class MC_Observer
{
public:
	enum StateType : uchar {ClanId, OnlineStatus, Rank, RankInClan};

	virtual ~MC_Observer() {}
	virtual void update(MC_Subject* subject, StateType type) = 0;
};