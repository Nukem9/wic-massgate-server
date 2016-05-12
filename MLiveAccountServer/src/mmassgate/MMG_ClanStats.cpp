#include "../stdafx.h"

MMG_ClanStats::MMG_ClanStats()
{
	this->m_ClanId = 0;									// must match with logged in clan id, otherwise empty stats screen
	this->m_LastMatchPlayed = 0;						// time in seconds since 1970-01-01 at 01:00 am
	this->m_NumberOfMatches = 0;
	this->m_NumberOfMatchesWon = 0;
	this->m_BestWinningStreak = 0;
	this->m_CurrentWinningStreak = 0;
	this->m_CurrentLadderPosition = 0;
	this->m_NumberOfTournaments = 0;
	this->m_NumberOfTournamentsWon = 0;

	this->m_NumberOfDominationMatches = 0;
	this->m_NumberOfDominationMatchesWon = 0;
	this->m_NumberOfMatchesWonByTotalDomination = 0;

	this->m_NumberOfAssaultMatches = 0;
	this->m_NumberOfAssaultMatchesWon = 0;
	this->m_NumberOfPerfectDefensesInAssaultMatch = 0;

	this->m_NumberOfTugOfWarMatches = 0;
	this->m_NumberOfTugOfWarMatchesWon = 0;
	this->m_NumberOfPerfectPushesInTugOfWarMatch = 0;

	this->m_NumberOfUnitsKilled = 0;
	this->m_NumberOfUnitsLost = 0;
	this->m_NumberOfNukesDeployed = 0;
	this->m_NumberOfTacticalAidCriticalHits = 0;

	this->m_TimePlayedAsUSA = 0;
	this->m_TimePlayedAsUSSR = 0;
	this->m_TimePlayedAsNATO = 0;
	this->m_ScoreTotal = 0;
	this->m_HighestScore = 0;

	this->m_ScoreByDamagingEnemies = 0;
	this->m_ScoreByRepairing = 0;
	this->m_ScoreByUsingTacticalAid = 0;
	this->m_ScoreByFortifying = 0;

	this->m_ScoreAsInfantry = 0;
	this->m_ScoreAsSupport = 0;
	this->m_ScoreAsAir = 0;
	this->m_ScoreAsArmor = 0;
	
	this->m_HighScoreAsInfantry = 0;
	this->m_HighScoreAsSupport = 0;
	this->m_HighScoreAsAir = 0;
	this->m_HighScoreAsArmor = 0;
}

void MMG_ClanStats::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_ClanId);
	aMessage->WriteUInt(this->m_LastMatchPlayed);
	aMessage->WriteUInt(this->m_NumberOfMatches);
	aMessage->WriteUInt(this->m_NumberOfMatchesWon);
	aMessage->WriteUInt(this->m_BestWinningStreak);
	aMessage->WriteUInt(this->m_CurrentWinningStreak);
	aMessage->WriteUInt(this->m_CurrentLadderPosition);
	aMessage->WriteUInt(this->m_NumberOfTournaments);
	aMessage->WriteUInt(this->m_NumberOfTournamentsWon);

	aMessage->WriteUInt(this->m_NumberOfDominationMatches);
	aMessage->WriteUInt(this->m_NumberOfDominationMatchesWon);
	aMessage->WriteUInt(this->m_NumberOfMatchesWonByTotalDomination);

	aMessage->WriteUInt(this->m_NumberOfAssaultMatches);
	aMessage->WriteUInt(this->m_NumberOfAssaultMatchesWon);
	aMessage->WriteUInt(this->m_NumberOfPerfectDefensesInAssaultMatch);

	aMessage->WriteUInt(this->m_NumberOfTugOfWarMatches);
	aMessage->WriteUInt(this->m_NumberOfTugOfWarMatchesWon);
	aMessage->WriteUInt(this->m_NumberOfPerfectPushesInTugOfWarMatch);

	aMessage->WriteUInt(this->m_NumberOfUnitsKilled);
	aMessage->WriteUInt(this->m_NumberOfUnitsLost);
	aMessage->WriteUInt(this->m_NumberOfNukesDeployed);
	aMessage->WriteUInt(this->m_NumberOfTacticalAidCriticalHits);

	aMessage->WriteUInt(this->m_TimePlayedAsUSA);
	aMessage->WriteUInt(this->m_TimePlayedAsUSSR);
	aMessage->WriteUInt(this->m_TimePlayedAsNATO);
	aMessage->WriteUInt(this->m_ScoreTotal);
	aMessage->WriteUInt(this->m_HighestScore);

	aMessage->WriteUInt(this->m_ScoreByDamagingEnemies);
	aMessage->WriteUInt(this->m_ScoreByRepairing);
	aMessage->WriteUInt(this->m_ScoreByUsingTacticalAid);
	aMessage->WriteUInt(this->m_ScoreByFortifying);

	aMessage->WriteUInt(this->m_ScoreAsInfantry);
	aMessage->WriteUInt(this->m_ScoreAsSupport);
	aMessage->WriteUInt(this->m_ScoreAsAir);
	aMessage->WriteUInt(this->m_ScoreAsArmor);

	aMessage->WriteUInt(this->m_HighScoreAsInfantry);
	aMessage->WriteUInt(this->m_HighScoreAsSupport);
	aMessage->WriteUInt(this->m_HighScoreAsAir);
	aMessage->WriteUInt(this->m_HighScoreAsArmor);

}

bool MMG_ClanStats::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadUInt(this->m_ClanId) || !aMessage->ReadUInt(this->m_LastMatchPlayed) || !aMessage->ReadUInt(this->m_NumberOfMatches))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfMatchesWon) || !aMessage->ReadUInt(this->m_BestWinningStreak) || !aMessage->ReadUInt(this->m_CurrentWinningStreak))
		return false;
	if (!aMessage->ReadUInt(this->m_CurrentLadderPosition) || !aMessage->ReadUInt(this->m_NumberOfTournaments) || !aMessage->ReadUInt(this->m_NumberOfTournamentsWon))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfDominationMatches) || !aMessage->ReadUInt(this->m_NumberOfDominationMatchesWon) || !aMessage->ReadUInt(this->m_NumberOfMatchesWonByTotalDomination))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfAssaultMatches) || !aMessage->ReadUInt(this->m_NumberOfAssaultMatchesWon) || !aMessage->ReadUInt(this->m_NumberOfPerfectDefensesInAssaultMatch))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfTugOfWarMatches) || !aMessage->ReadUInt(this->m_NumberOfTugOfWarMatchesWon) || !aMessage->ReadUInt(this->m_NumberOfPerfectPushesInTugOfWarMatch))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfUnitsKilled) || !aMessage->ReadUInt(this->m_NumberOfUnitsLost) || !aMessage->ReadUInt(this->m_NumberOfNukesDeployed))
		return false;
	if (!aMessage->ReadUInt(this->m_NumberOfTacticalAidCriticalHits) || !aMessage->ReadUInt(this->m_TimePlayedAsUSA) || !aMessage->ReadUInt(this->m_TimePlayedAsUSSR))
		return false;
	if (!aMessage->ReadUInt(this->m_TimePlayedAsNATO) || !aMessage->ReadUInt(this->m_ScoreTotal) || !aMessage->ReadUInt(this->m_HighestScore))
		return false;
	if (!aMessage->ReadUInt(this->m_ScoreByDamagingEnemies) || !aMessage->ReadUInt(this->m_ScoreByRepairing) || !aMessage->ReadUInt(this->m_ScoreByUsingTacticalAid))
		return false;
	if (!aMessage->ReadUInt(this->m_ScoreByFortifying) || !aMessage->ReadUInt(this->m_ScoreAsInfantry) || !aMessage->ReadUInt(this->m_ScoreAsSupport))
		return false;
	if (!aMessage->ReadUInt(this->m_ScoreAsAir) || !aMessage->ReadUInt(this->m_ScoreAsArmor) || !aMessage->ReadUInt(this->m_HighScoreAsInfantry))
		return false;
	if (!aMessage->ReadUInt(this->m_HighScoreAsSupport) || !aMessage->ReadUInt(this->m_HighScoreAsAir) || !aMessage->ReadUInt(this->m_HighScoreAsArmor))
		return false;

	return true;
}