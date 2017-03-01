#pragma once

class MMG_Stats
{
public:

	// MMG_DecorationProtocol
	class Medal
	{
	public:
		uint level;
		uint stars;

		Medal() : level(0), stars(0){}
	};

	class Badge
	{
	public:
		uint level;
		uint stars;

		Badge() : level(0), stars(0){}
	};

	// not in IDA
	class ExtraPlayerStats
	{
	public:
		uint m_CurrentAssaultWinStreak;
		uint m_CurrentDominationWinStreak;
		uint m_CurrentTugOfWarWinStreak;
		uint m_CurrentDominationPerfectStreak;
		uint m_CurrentAssaultPerfectStreak;
		uint m_CurrentTugOfWarPerfectStreak;
		uint m_CurrentNukesDeployedStreak;
		uint m_NumberOfTimesBestPlayer;
		uint m_CurrentBestPlayerStreak;
		uint m_NumberOfTimesBestInfantry;
		uint m_CurrentBestInfantryStreak;
		uint m_NumberOfTimesBestSupport;
		uint m_CurrentBestSupportStreak;
		uint m_NumberOfTimesBestAir;
		uint m_CurrentBestAirStreak;
		uint m_NumberOfTimesBestArmor;
		uint m_CurrentBestArmorStreak;

	private:

	public:
		ExtraPlayerStats()
		{
			this->m_CurrentAssaultWinStreak				= 0;
			this->m_CurrentDominationWinStreak			= 0;
			this->m_CurrentTugOfWarWinStreak			= 0;
			this->m_CurrentDominationPerfectStreak		= 0;
			this->m_CurrentAssaultPerfectStreak			= 0;
			this->m_CurrentTugOfWarPerfectStreak		= 0;
			this->m_CurrentNukesDeployedStreak			= 0;
			this->m_NumberOfTimesBestPlayer				= 0;
			this->m_CurrentBestPlayerStreak				= 0;
			this->m_NumberOfTimesBestInfantry			= 0;
			this->m_CurrentBestInfantryStreak			= 0;
			this->m_NumberOfTimesBestSupport			= 0;
			this->m_CurrentBestSupportStreak			= 0;
			this->m_NumberOfTimesBestAir				= 0;
			this->m_CurrentBestAirStreak				= 0;
			this->m_NumberOfTimesBestArmor				= 0;
			this->m_CurrentBestArmorStreak				= 0;
		}
	};

	class PlayerStatsRsp : public MMG_IStreamable
	{
	public:
		uint m_ProfileId;
		uint m_LastMatchPlayed;
		uint m_ScoreTotal;
		uint m_ScoreAsInfantry;
		uint m_HighScoreAsInfantry;
		uint m_ScoreAsSupport;
		uint m_HighScoreAsSupport;
		uint m_ScoreAsArmor;
		uint m_HighScoreAsArmor;
		uint m_ScoreAsAir;
		uint m_HighScoreAsAir;
		uint m_ScoreByDamagingEnemies;
		uint m_ScoreByUsingTacticalAid;
		uint m_ScoreByCapturingCommandPoints;
		uint m_ScoreByRepairing;
		uint m_ScoreByFortifying;
		uint m_HighestScore;
		uint m_CurrentLadderPosition;
		uint m_TimeTotalMatchLength;
		uint m_TimePlayedAsUSA;
		uint m_TimePlayedAsUSSR;
		uint m_TimePlayedAsNATO;
		uint m_TimePlayedAsInfantry;
		uint m_TimePlayedAsSupport;
		uint m_TimePlayedAsArmor;
		uint m_TimePlayedAsAir;
		uint m_NumberOfMatches;
		uint m_NumberOfMatchesWon;
		uint m_NumberOfMatchesLost;
		uint m_NumberOfAssaultMatches;
		uint m_NumberOfAssaultMatchesWon;
		uint m_NumberOfDominationMatches;
		uint m_NumberOfDominationMatchesWon;
		uint m_NumberOfTugOfWarMatches;
		uint m_NumberOfTugOfWarMatchesWon;
		uint m_CurrentWinningStreak;
		uint m_BestWinningStreak;
		uint m_NumberOfMatchesWonByTotalDomination;
		uint m_NumberOfPerfectDefendsInAssaultMatch;
		uint m_NumberOfPerfectPushesInTugOfWarMatch;
		uint m_NumberOfUnitsKilled;
		uint m_NumberOfUnitsLost;
		uint m_NumberOfReinforcementPointsSpent;
		uint m_NumberOfTacticalAidPointsSpent;
		uint m_NumberOfNukesDeployed;
		uint m_NumberOfTacticalAidCriticalHits;

	private:

	public:
		PlayerStatsRsp();

		void ToStream	(MN_WriteMessage *aMessage);
		bool FromStream	(MN_ReadMessage *aMessage);
	};

	class ClanStatsRsp : public MMG_IStreamable
	{
	public:
		uint m_ClanId;
		uint m_LastMatchPlayed;
		uint m_MatchesPlayed;
		uint m_MatchesWon;
		uint m_BestWinningStreak;
		uint m_CurrentWinningStreak;
		uint m_CurrentLadderPosition;
		uint m_TournamentsPlayed;
		uint m_TournamentsWon;
		uint m_DominationMatchesPlayed;
		uint m_DominationMatchesWon;
		uint m_DominationMatchesWonByTotalDomination;
		uint m_AssaultMatchesPlayed;
		uint m_AssaultMatchesWon;
		uint m_AssaultMatchesPerfectDefense;
		uint m_TowMatchesPlayed;
		uint m_TowMatchesMatchesWon;
		uint m_TowMatchesMatchesPerfectPushes;
		uint m_NumberOfUnitsKilled;
		uint m_NumberOfUnitsLost;
		uint m_NumberOfNukesDeployed;
		uint m_NumberOfTACriticalHits;
		uint m_TimeAsUSA;
		uint m_TimeAsUSSR;
		uint m_TimeAsNATO;
		uint m_TotalScore;
		uint m_HighestScoreInAMatch;
		uint m_ScoreByDamagingEnemies;
		uint m_ScoreByRepairing;
		uint m_ScoreByTacticalAid;
		uint m_ScoreByFortifying;
		uint m_ScoreAsInfantry;
		uint m_ScoreAsSupport;
		uint m_ScoreAsAir;
		uint m_ScoreAsArmor;
		uint m_HighScoreAsInfantry;
		uint m_HighScoreAsSupport;
		uint m_HighScoreAsAir;
		uint m_HighScoreAsArmor;

	private:

	public:
		ClanStatsRsp();

		void ToStream	(MN_WriteMessage *aMessage);
		bool FromStream	(MN_ReadMessage *aMessage);
	};
	
	class PlayerMatchStats : public MMG_IStreamable
	{
	public:
		uint m_GameProtocolVersion;
		uint m_ProfileId;
		ushort m_ScoreTotal;
		ushort m_ScoreAsInfantry;
		ushort m_ScoreAsSupport;
		ushort m_ScoreAsArmor;
		ushort m_ScoreAsAir;
		ushort m_ScoreByDamagingEnemies;
		ushort m_ScoreByUsingTacticalAids;
		ushort m_ScoreByCommandPointCaptures;
		ushort m_ScoreByRepairing;
		ushort m_ScoreByFortifying;
		ushort m_ScoreLostByKillingFriendly;
		ushort m_TimeTotalMatchLength;
		ushort m_TimePlayedAsUSA;
		ushort m_TimePlayedAsUSSR;
		ushort m_TimePlayedAsNATO;
		ushort m_TimePlayedAsInfantry;
		ushort m_TimePlayedAsAir;
		ushort m_TimePlayedAsArmor;
		ushort m_TimePlayedAsSupport;
		uchar m_MatchWon;
		uchar m_MatchType;
		uchar m_MatchLost;
		uchar m_MatchWasFlawlessVictory;
		ushort m_NumberOfUnitsKilled;
		ushort m_NumberOfUnitsLost;
		ushort m_NumberOfCommandPointCaptures;
		ushort m_NumberOfReinforcementPointsSpent;
		ushort m_NumberOfRoleChanges;
		ushort m_NumberOfTacticalAidPointsSpent;
		ushort m_NumberOfNukesDeployed;
		ushort m_NumberOfTacticalAidCriticalHits;
		uchar m_BestData;
		float m_TotalTimePlayed;
		uchar m_WasPlayingAtMatchEnd;

	private:

	public:
		PlayerMatchStats();

		void ToStream	(MN_WriteMessage *aMessage);
		bool FromStream	(MN_ReadMessage *aMessage);
	};

	class ClanMatchStats : public MMG_IStreamable
	{
	public:
		class PerPlayerData
		{
		public:
			uint m_ProfileId;
			ushort m_Score;
			uchar m_Role;
		};

		uint m_GameProtocolVersion;
		uint64 m_MapHash;
		wchar_t m_MapName[256];
		uint m_ClanId;
		uchar m_MatchType;
		ushort m_DominationPercentage;
		uchar m_AssaultNumCommandPoints;
		uint m_AssaultEndTime;
		uchar m_TowNumLinesPushed;
		uchar m_TowNumPerimiterPoints;
		uchar m_MatchWon;
		uchar m_IsTournamentMatch;
		uchar m_MatchWasFlawlessVictory;
		uint m_NumberOfUnitsKilled;
		uint m_NumberOfUnitsLost;
		uint m_NumberOfNukesDeployed;
		uint m_NumberOfTACriticalHits;
		uint m_TimeAsUSA;
		uint m_TimeAsUSSR;
		uint m_TimeAsNATO;
		uint m_TotalScore;
		uint m_ScoreByDamagingEnemies;
		uint m_ScoreByRepairing;
		uint m_ScoreByTacticalAid;
		uint m_ScoreByFortifying;
		uint m_ScoreByInfantry;
		uint m_ScoreBySupport;
		uint m_ScoreByAir;
		uint m_ScoreByArmor;
		uint m_PerPlayerDataCount;
		PerPlayerData m_PerPlayerData[8];

	private:

	public:
		ClanMatchStats();

		void ToStream	(MN_WriteMessage *aMessage);
		bool FromStream	(MN_ReadMessage *aMessage);
	};

private:

public:
	MMG_Stats();
};