#pragma once

class MMG_ClanStats : public MMG_IStreamable
{
public:
	uint m_ClanId;
	uint m_LastMatchPlayed;
	uint m_NumberOfMatches;
	uint m_NumberOfMatchesWon;
	uint m_BestWinningStreak;
	uint m_CurrentWinningStreak;
	uint m_CurrentLadderPosition;
	uint m_NumberOfTournaments;
	uint m_NumberOfTournamentsWon;
	
	uint m_NumberOfDominationMatches;
	uint m_NumberOfDominationMatchesWon;
	uint m_NumberOfMatchesWonByTotalDomination;
	
	uint m_NumberOfAssaultMatches;
	uint m_NumberOfAssaultMatchesWon;
	uint m_NumberOfPerfectDefensesInAssaultMatch;
	
	uint m_NumberOfTugOfWarMatches;
	uint m_NumberOfTugOfWarMatchesWon;
	uint m_NumberOfPerfectPushesInTugOfWarMatch;
	
	uint m_NumberOfUnitsKilled;
	uint m_NumberOfUnitsLost;
	uint m_NumberOfNukesDeployed;
	uint m_NumberOfTacticalAidCriticalHits;
	
	uint m_TimePlayedAsUSA;
	uint m_TimePlayedAsUSSR;
	uint m_TimePlayedAsNATO;
	uint m_ScoreTotal;
	uint m_HighestScore;
	
	uint m_ScoreByDamagingEnemies;
	uint m_ScoreByRepairing;
	uint m_ScoreByUsingTacticalAid;
	uint m_ScoreByFortifying;
	
	uint m_ScoreAsInfantry;
	uint m_ScoreAsSupport;
	uint m_ScoreAsArmor;
	uint m_ScoreAsAir;
	
	uint m_HighScoreAsInfantry;
	uint m_HighScoreAsSupport;
	uint m_HighScoreAsArmor;
	uint m_HighScoreAsAir;
	

private:

public:
	MMG_ClanStats();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};