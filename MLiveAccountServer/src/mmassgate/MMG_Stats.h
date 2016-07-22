#pragma once

class MMG_Stats
{
public:
	class PlayerStatsRsp
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
		uint m_NumberOfPerfectDefensesInAssaultMatch;
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
	};

	class ClanStatsRsp
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
	};
	
private:

public:
	MMG_Stats();
};