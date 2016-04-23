#pragma once

static const char *MySQLTableValues[] =
{
	"drop table if exists mg_accounts;",

	"drop table if exists mg_profiles;",

	"drop table if exists mg_clans;",

	"CREATE TABLE mg_accounts ("
		"id int not null auto_increment,"
		"email varchar(64) not null,"
		"password varchar(16) not null,"
		"country varchar(5) not null,"
		"emailme tinyint unsigned not null,"
		"acceptsemail tinyint unsigned not null,"
		"activeprofileid int notnull,"
		"isbanned tinyint unsigned not null,"
		"PRIMARY KEY (id)"
	");",

	"CREATE TABLE mg_cdkeys ("
		"id int not null auto_increment,"
		"accountid int not null,"
		"cipherkeys0 int not null,"
		"cipherkeys1 int not null,"
		"cipherkeys2 int not null,"
		"cipherkeys3 int not null,"
		"PRIMARY KEY (id)"
	");",
	
	"CREATE TABLE mg_profiles ("
		"id int not null auto_increment,"
		"accountid int not null,"
		"name varchar(25) not null,"
		"experience int unsigned not null,"
		"rank int unsigned not null,"
		"clanid int unsigned not null,"
		"rankinclan int unsigned not null,"
		"chatoptions int unsigned not null,"
		"onlinestatus int unsigned not null,"
		"lastlogindate datetime not null,"
		"PRIMARY KEY (id)"
	");",

	"CREATE TABLE mg_clans ("
		"id int not null auto_increment,"
		"name varchar(25) not null,"
		"PRIMARY KEY (id)"
	");",
};