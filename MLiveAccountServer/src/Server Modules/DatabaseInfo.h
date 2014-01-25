#pragma once

static const char *TableValues[] =
{
	"CREATE TABLE LiveAccUsers (ProfileID INTEGER PRIMARY KEY, Username TEXT, Email TEXT, Password BLOB, Experience INTEGER, Rank INTEGER, ClanID INTEGER, RankInClan INTEGER, ChatOptions INTEGER, OnlineStatus INTEGER);",
	"CREATE TABLE LiveAccClans (ClanID INTEGER PRIMARY KEY, Clanname TEXT);",
};