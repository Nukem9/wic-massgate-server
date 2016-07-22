#include "../stdafx.h"

MMG_Stats::PlayerStatsRsp::PlayerStatsRsp()
{
	this->m_ProfileId								= 0;
	this->m_LastMatchPlayed							= 0;
	this->m_ScoreTotal								= 0;
	this->m_ScoreAsInfantry							= 0;
	this->m_HighScoreAsInfantry						= 0;
	this->m_ScoreAsSupport							= 0;
	this->m_HighScoreAsSupport						= 0;
	this->m_ScoreAsArmor							= 0;
	this->m_HighScoreAsArmor						= 0;
	this->m_ScoreAsAir								= 0;
	this->m_HighScoreAsAir							= 0;
	this->m_ScoreByDamagingEnemies					= 0;
	this->m_ScoreByUsingTacticalAid					= 0;
	this->m_ScoreByCapturingCommandPoints			= 0;
	this->m_ScoreByRepairing						= 0;
	this->m_ScoreByFortifying						= 0;
	this->m_HighestScore							= 0;
	this->m_CurrentLadderPosition					= 0;
	this->m_TimeTotalMatchLength					= 0;
	this->m_TimePlayedAsUSA							= 0;
	this->m_TimePlayedAsUSSR						= 0;
	this->m_TimePlayedAsNATO						= 0;
	this->m_TimePlayedAsInfantry					= 0;
	this->m_TimePlayedAsSupport						= 0;
	this->m_TimePlayedAsArmor						= 0;
	this->m_TimePlayedAsAir							= 0;
	this->m_NumberOfMatches							= 0;
	this->m_NumberOfMatchesWon						= 0;
	this->m_NumberOfMatchesLost						= 0;
	this->m_NumberOfAssaultMatches					= 0;
	this->m_NumberOfAssaultMatchesWon				= 0;
	this->m_NumberOfDominationMatches				= 0;
	this->m_NumberOfDominationMatchesWon			= 0;
	this->m_NumberOfTugOfWarMatches					= 0;
	this->m_NumberOfTugOfWarMatchesWon				= 0;
	this->m_CurrentWinningStreak					= 0;
	this->m_BestWinningStreak						= 0;
	this->m_NumberOfMatchesWonByTotalDomination		= 0;
	this->m_NumberOfPerfectDefensesInAssaultMatch	= 0;
	this->m_NumberOfPerfectPushesInTugOfWarMatch	= 0;
	this->m_NumberOfUnitsKilled						= 0;
	this->m_NumberOfUnitsLost						= 0;
	this->m_NumberOfReinforcementPointsSpent		= 0;
	this->m_NumberOfTacticalAidPointsSpent			= 0;
	this->m_NumberOfNukesDeployed					= 0;
	this->m_NumberOfTacticalAidCriticalHits			= 0;
}

void MMG_Stats::PlayerStatsRsp::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_ProfileId);
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

MMG_Stats::ClanStatsRsp::ClanStatsRsp()
{
	this->m_ClanId									= 0;
	this->m_LastMatchPlayed							= 0;
	this->m_MatchesPlayed							= 0;
	this->m_MatchesWon								= 0;
	this->m_BestWinningStreak						= 0;
	this->m_CurrentWinningStreak					= 0;
	this->m_CurrentLadderPosition					= 0;
	this->m_TournamentsPlayed						= 0;
	this->m_TournamentsWon							= 0;
	this->m_DominationMatchesPlayed					= 0;
	this->m_DominationMatchesWon					= 0;
	this->m_DominationMatchesWonByTotalDomination	= 0;
	this->m_AssaultMatchesPlayed					= 0;
	this->m_AssaultMatchesWon						= 0;
	this->m_AssaultMatchesPerfectDefense			= 0;
	this->m_TowMatchesPlayed						= 0;
	this->m_TowMatchesMatchesWon					= 0;
	this->m_TowMatchesMatchesPerfectPushes			= 0;
	this->m_NumberOfUnitsKilled						= 0;
	this->m_NumberOfUnitsLost						= 0;
	this->m_NumberOfNukesDeployed					= 0;
	this->m_NumberOfTACriticalHits					= 0;
	this->m_TimeAsUSA								= 0;
	this->m_TimeAsUSSR								= 0;
	this->m_TimeAsNATO								= 0;
	this->m_TotalScore								= 0;
	this->m_HighestScoreInAMatch					= 0;
	this->m_ScoreByDamagingEnemies					= 0;
	this->m_ScoreByRepairing						= 0;
	this->m_ScoreByTacticalAid						= 0;
	this->m_ScoreByFortifying						= 0;
	this->m_ScoreAsInfantry							= 0;
	this->m_ScoreAsSupport							= 0;
	this->m_ScoreAsAir								= 0;
	this->m_ScoreAsArmor							= 0;
	this->m_HighScoreAsInfantry						= 0;
	this->m_HighScoreAsSupport						= 0;
	this->m_HighScoreAsAir							= 0;
	this->m_HighScoreAsArmor						= 0;
}

void MMG_Stats::ClanStatsRsp::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_ClanId);
	aMessage->WriteUInt(this->m_LastMatchPlayed);
	aMessage->WriteUInt(this->m_MatchesPlayed);
	aMessage->WriteUInt(this->m_MatchesWon);
	aMessage->WriteUInt(this->m_BestWinningStreak);
	aMessage->WriteUInt(this->m_CurrentWinningStreak);
	aMessage->WriteUInt(this->m_CurrentLadderPosition);
	aMessage->WriteUInt(this->m_TournamentsPlayed);
	aMessage->WriteUInt(this->m_TournamentsWon);
	aMessage->WriteUInt(this->m_DominationMatchesPlayed);
	aMessage->WriteUInt(this->m_DominationMatchesWon);
	aMessage->WriteUInt(this->m_DominationMatchesWonByTotalDomination);
	aMessage->WriteUInt(this->m_AssaultMatchesPlayed);
	aMessage->WriteUInt(this->m_AssaultMatchesWon);
	aMessage->WriteUInt(this->m_AssaultMatchesPerfectDefense);
	aMessage->WriteUInt(this->m_TowMatchesPlayed);
	aMessage->WriteUInt(this->m_TowMatchesMatchesWon);
	aMessage->WriteUInt(this->m_TowMatchesMatchesPerfectPushes);
	aMessage->WriteUInt(this->m_NumberOfUnitsKilled);
	aMessage->WriteUInt(this->m_NumberOfUnitsLost);
	aMessage->WriteUInt(this->m_NumberOfNukesDeployed);
	aMessage->WriteUInt(this->m_NumberOfTACriticalHits);
	aMessage->WriteUInt(this->m_TimeAsUSA);
	aMessage->WriteUInt(this->m_TimeAsUSSR);
	aMessage->WriteUInt(this->m_TimeAsNATO);
	aMessage->WriteUInt(this->m_TotalScore);
	aMessage->WriteUInt(this->m_HighestScoreInAMatch);
	aMessage->WriteUInt(this->m_ScoreByDamagingEnemies);
	aMessage->WriteUInt(this->m_ScoreByRepairing);
	aMessage->WriteUInt(this->m_ScoreByTacticalAid);
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

MMG_Stats::MMG_Stats()
{
}