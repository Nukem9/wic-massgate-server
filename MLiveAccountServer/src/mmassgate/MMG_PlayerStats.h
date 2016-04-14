#pragma once

class MMG_PlayerStats : public MMG_IStreamable
{
public:
	uint m_PlayerId;
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
	uint m_CurrentWinningStreak;
	uint m_BestWinningStreak;
	uint m_NumberOfAssaultMatches;
	uint m_NumberOfAssaultMatchesWon;
	uint m_NumberOfDominationMatches;
	uint m_NumberOfDominationMatchesWon;
	uint m_NumberOfTugOfWarMatches;
	uint m_NumberOfTugOfWarMatchesWon;
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
	MMG_PlayerStats();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};