#include "../stdafx.h"

MMG_PlayerStats::MMG_PlayerStats()
{
	this->m_PlayerId = 0;								// must match with logged in player id, otherwise empty stats screen
	this->m_LastMatchPlayed = 0;						// time in seconds since 1970-01-01 at 01:00 am
	this->m_ScoreTotal = 0;
	this->m_ScoreAsInfantry = 0;
	this->m_HighScoreAsInfantry = 0;
	this->m_ScoreAsSupport = 0;
	this->m_HighScoreAsSupport = 0;
	this->m_ScoreAsArmor = 0;
	this->m_HighScoreAsArmor = 0;
	this->m_ScoreAsAir = 0;
	this->m_HighScoreAsAir = 0;
	this->m_ScoreByDamagingEnemies = 0;
	this->m_ScoreByUsingTacticalAid = 0;
	this->m_ScoreByCapturingCommandPoints = 0;
	this->m_ScoreByRepairing = 0;
	this->m_ScoreByFortifying = 0;
	this->m_HighestScore = 0;
	this->m_CurrentLadderPosition = 0;
	this->m_TimeTotalMatchLength = 0;					// not displayed, used to calculate average match length
	this->m_TimePlayedAsUSA = 0;						// time in seconds
	this->m_TimePlayedAsUSSR = 0;						// time in seconds
	this->m_TimePlayedAsNATO = 0;						// time in seconds
	this->m_TimePlayedAsInfantry = 0;					// time in seconds
	this->m_TimePlayedAsSupport = 0;					// time in seconds
	this->m_TimePlayedAsArmor = 0;						// time in seconds
	this->m_TimePlayedAsAir = 0;						// time in seconds
	this->m_NumberOfMatches = 0;
	this->m_NumberOfMatchesWon = 0;
	this->m_NumberOfMatchesLost = 0;
	this->m_CurrentWinningStreak = 0;
	this->m_BestWinningStreak = 0;
	this->m_NumberOfAssaultMatches = 0;					// not displayed, used to calculate number of lost as_ matches
	this->m_NumberOfAssaultMatchesWon = 0;
	this->m_NumberOfDominationMatches = 0;				// not displayed, used to calculate number of lost do_ matches
	this->m_NumberOfDominationMatchesWon = 0;
	this->m_NumberOfTugOfWarMatches = 0;				// not displayed, used to calculate number of lost tw_ matches
	this->m_NumberOfTugOfWarMatchesWon = 0;
	this->m_NumberOfMatchesWonByTotalDomination = 0;
	this->m_NumberOfPerfectDefensesInAssaultMatch = 0;
	this->m_NumberOfPerfectPushesInTugOfWarMatch = 0;
	this->m_NumberOfUnitsKilled = 0;
	this->m_NumberOfUnitsLost = 0;
	this->m_NumberOfReinforcementPointsSpent = 0;
	this->m_NumberOfTacticalAidPointsSpent = 0;
	this->m_NumberOfNukesDeployed = 0;
	this->m_NumberOfTacticalAidCriticalHits = 0;
}

void MMG_PlayerStats::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_PlayerId);
	aMessage->WriteUInt(this->m_LastMatchPlayed);
	aMessage->WriteUInt(this->m_ScoreTotal);
	aMessage->WriteUInt(this->m_ScoreAsInfantry);
	aMessage->WriteUInt(this->m_HighScoreAsInfantry);
	aMessage->WriteUInt(this->m_ScoreAsSupport);
	aMessage->WriteUInt(this->m_HighScoreAsSupport);
	aMessage->WriteUInt(this->m_ScoreAsArmor);
	aMessage->WriteUInt(this->m_HighScoreAsArmor);
	aMessage->WriteUInt(this->m_ScoreAsAir);
	aMessage->WriteUInt(this->m_HighScoreAsAir);
	aMessage->WriteUInt(this->m_ScoreByDamagingEnemies);
	aMessage->WriteUInt(this->m_ScoreByUsingTacticalAid);
	aMessage->WriteUInt(this->m_ScoreByCapturingCommandPoints);
	aMessage->WriteUInt(this->m_ScoreByRepairing);
	aMessage->WriteUInt(this->m_ScoreByFortifying);
	aMessage->WriteUInt(this->m_HighestScore);
	aMessage->WriteUInt(this->m_CurrentLadderPosition);
	aMessage->WriteUInt(this->m_TimeTotalMatchLength);
	aMessage->WriteUInt(this->m_TimePlayedAsUSA);
	aMessage->WriteUInt(this->m_TimePlayedAsUSSR);
	aMessage->WriteUInt(this->m_TimePlayedAsNATO);
	aMessage->WriteUInt(this->m_TimePlayedAsInfantry);
	aMessage->WriteUInt(this->m_TimePlayedAsSupport);
	aMessage->WriteUInt(this->m_TimePlayedAsArmor);
	aMessage->WriteUInt(this->m_TimePlayedAsAir);
	aMessage->WriteUInt(this->m_NumberOfMatches);
	aMessage->WriteUInt(this->m_NumberOfMatchesWon);
	aMessage->WriteUInt(this->m_NumberOfMatchesLost);
	aMessage->WriteUInt(this->m_NumberOfAssaultMatches);
	aMessage->WriteUInt(this->m_NumberOfAssaultMatchesWon);
	aMessage->WriteUInt(this->m_NumberOfDominationMatches);
	aMessage->WriteUInt(this->m_NumberOfDominationMatchesWon);
	aMessage->WriteUInt(this->m_NumberOfTugOfWarMatches);
	aMessage->WriteUInt(this->m_NumberOfTugOfWarMatchesWon);
	aMessage->WriteUInt(this->m_CurrentWinningStreak);
	aMessage->WriteUInt(this->m_BestWinningStreak);
	aMessage->WriteUInt(this->m_NumberOfMatchesWonByTotalDomination);
	aMessage->WriteUInt(this->m_NumberOfPerfectDefensesInAssaultMatch);
	aMessage->WriteUInt(this->m_NumberOfPerfectPushesInTugOfWarMatch);
	aMessage->WriteUInt(this->m_NumberOfUnitsKilled);
	aMessage->WriteUInt(this->m_NumberOfUnitsLost);
	aMessage->WriteUInt(this->m_NumberOfReinforcementPointsSpent);
	aMessage->WriteUInt(this->m_NumberOfTacticalAidPointsSpent);
	aMessage->WriteUInt(this->m_NumberOfNukesDeployed);
	aMessage->WriteUInt(this->m_NumberOfTacticalAidCriticalHits);
	
}

bool MMG_PlayerStats::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadUInt(this->m_PlayerId) || !aMessage->ReadUInt(this->m_LastMatchPlayed) || !aMessage->ReadUInt(this->m_ScoreTotal))
		return false;
	if (!aMessage->ReadUInt(this->m_ScoreAsInfantry) || !aMessage->ReadUInt(this->m_HighScoreAsInfantry) || !aMessage->ReadUInt(this->m_ScoreAsSupport))
		return false;
	if (!aMessage->ReadUInt(this->m_HighScoreAsSupport) || !aMessage->ReadUInt(this->m_ScoreAsArmor) || !aMessage->ReadUInt(this->m_HighScoreAsArmor))
		return false;
	if (!aMessage->ReadUInt(this->m_ScoreAsAir) || !aMessage->ReadUInt(this->m_HighScoreAsAir) || !aMessage->ReadUInt(this->m_ScoreByDamagingEnemies))
		return false;
	if (!aMessage->ReadUInt(this->m_ScoreByUsingTacticalAid) || !aMessage->ReadUInt(this->m_ScoreByCapturingCommandPoints) || !aMessage->ReadUInt(this->m_ScoreByRepairing))
		return false;
	if (!aMessage->ReadUInt(this->m_ScoreByFortifying) || !aMessage->ReadUInt(this->m_HighestScore) || !aMessage->ReadUInt(this->m_CurrentLadderPosition))
		return false;
	if (!aMessage->ReadUInt(this->m_TimeTotalMatchLength) || !aMessage->ReadUInt(this->m_TimePlayedAsUSA) || !aMessage->ReadUInt(this->m_TimePlayedAsUSSR))
		return false;
	if (!aMessage->ReadUInt(this->m_TimePlayedAsNATO) || !aMessage->ReadUInt(this->m_TimePlayedAsInfantry) || !aMessage->ReadUInt(this->m_TimePlayedAsSupport))
		return false;
	if (!aMessage->ReadUInt(this->m_TimePlayedAsArmor) || !aMessage->ReadUInt(this->m_TimePlayedAsAir) || !aMessage->ReadUInt(this->m_NumberOfMatches))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfMatchesWon) || !aMessage->ReadUInt(this->m_NumberOfMatchesLost) || !aMessage->ReadUInt(this->m_CurrentWinningStreak))
		return false;
	if (!aMessage->ReadUInt(this->m_BestWinningStreak) || !aMessage->ReadUInt(this->m_NumberOfAssaultMatches) || !aMessage->ReadUInt(this->m_NumberOfAssaultMatchesWon))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfDominationMatches) || !aMessage->ReadUInt(this->m_NumberOfDominationMatchesWon) || !aMessage->ReadUInt(this->m_NumberOfTugOfWarMatches))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfTugOfWarMatchesWon) || !aMessage->ReadUInt(this->m_NumberOfMatchesWonByTotalDomination) || !aMessage->ReadUInt(this->m_NumberOfPerfectDefensesInAssaultMatch))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfPerfectPushesInTugOfWarMatch) || !aMessage->ReadUInt(this->m_NumberOfUnitsKilled) || !aMessage->ReadUInt(this->m_NumberOfUnitsLost))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfReinforcementPointsSpent) || !aMessage->ReadUInt(this->m_NumberOfTacticalAidPointsSpent) || !aMessage->ReadUInt(this->m_NumberOfNukesDeployed))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfTacticalAidCriticalHits))
		return false;

	return true;
}