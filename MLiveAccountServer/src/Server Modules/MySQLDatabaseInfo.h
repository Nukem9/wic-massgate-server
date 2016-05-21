#pragma once

static const char *MySQLTableValues[] =
{
	"drop table if exists mg_utils",

	"CREATE TABLE mg_utils ("
		"poll tinyint(3) unsigned not null"
	") ENGINE=MyISAM;",

	"INSERT INTO mg_utils (poll) VALUES ('1');",

	"drop table if exists mg_accounts;",

	"drop table if exists mg_cdkeys;",

	"drop table if exists mg_profiles;",

	"drop table if exists mg_friends;",

	"drop table if exists mg_ignored;",

	"drop table if exists mg_clans;",

	"CREATE TABLE mg_accounts ("
		"id int(10) unsigned not null auto_increment,"
		"email varchar(64) not null,"
		"password varchar(32) not null,"
		"country varchar(5) not null,"
		"emailgamerelated tinyint(3) unsigned not null,"
		"acceptsemail tinyint(3) unsigned not null,"
		"activeprofileid int(10) unsigned not null,"
		"isbanned tinyint(3) unsigned not null,"
		"PRIMARY KEY (id)"
	") ENGINE=MyISAM AUTO_INCREMENT=10000;",

	"CREATE TABLE mg_cdkeys ("
		"id int(10) unsigned not null auto_increment,"
		"accountid int(10) unsigned not null,"
		"cipherkeys0 int(10) not null,"
		"cipherkeys1 int(10) not null,"
		"cipherkeys2 int(10) not null,"
		"cipherkeys3 int(10) not null,"
		"PRIMARY KEY (id)"
	") ENGINE=MyISAM AUTO_INCREMENT=10000;",
	
	"CREATE TABLE mg_profiles ("
		"id int(10) unsigned not null auto_increment,"
		"accountid int(10) unsigned not null,"
		"name varchar(32) not null,"
		"rank tinyint(3) unsigned not null,"
		"clanid int(10) unsigned not null,"
		"rankinclan tinyint(3) unsigned not null,"
		"commoptions int(10) unsigned not null,"
		"onlinestatus int(10) unsigned not null,"
		"lastlogindate datetime not null,"
		"motto varchar(512),"
		"homepage varchar(512),"
		"PRIMARY KEY (id)"
	") ENGINE=MyISAM AUTO_INCREMENT=20000;",

	"CREATE TABLE mg_friends ("
		"id int(10) unsigned not null auto_increment,"
		"profileid int(10) unsigned not null,"
		"friendprofileid int(10) unsigned not null,"
		"PRIMARY KEY (id)"
	") ENGINE=MyISAM;",

	"CREATE TABLE mg_ignored ("
		"id int(10) unsigned not null auto_increment,"
		"profileid int(10) unsigned not null,"
		"ignoredprofileid int(10) unsigned not null,"
		"PRIMARY KEY (id)"
	") ENGINE=MyISAM;",

	"CREATE TABLE mg_clans ("
		"id int(10) unsigned not null auto_increment,"
		"name varchar(25) not null,"
		"PRIMARY KEY (id)"
	") ENGINE=MyISAM AUTO_INCREMENT=20000;",
};