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
	this->m_NumberOfPerfectDefendsInAssaultMatch	= 0;
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
	aMessage->WriteUInt(this->m_NumberOfPerfectDefendsInAssaultMatch);
	aMessage->WriteUInt(this->m_NumberOfPerfectPushesInTugOfWarMatch);
	aMessage->WriteUInt(this->m_NumberOfUnitsKilled);
	aMessage->WriteUInt(this->m_NumberOfUnitsLost);
	aMessage->WriteUInt(this->m_NumberOfReinforcementPointsSpent);
	aMessage->WriteUInt(this->m_NumberOfTacticalAidPointsSpent);
	aMessage->WriteUInt(this->m_NumberOfNukesDeployed);
	aMessage->WriteUInt(this->m_NumberOfTacticalAidCriticalHits);
}

bool MMG_Stats::PlayerStatsRsp::FromStream(MN_ReadMessage *aMessage)
{
	return aMessage->ReadUInt(this->m_ProfileId)
		&& aMessage->ReadUInt(this->m_LastMatchPlayed)
		&& aMessage->ReadUInt(this->m_ScoreTotal)
		&& aMessage->ReadUInt(this->m_ScoreAsInfantry)
		&& aMessage->ReadUInt(this->m_HighScoreAsInfantry)
		&& aMessage->ReadUInt(this->m_ScoreAsSupport)
		&& aMessage->ReadUInt(this->m_HighScoreAsSupport)
		&& aMessage->ReadUInt(this->m_ScoreAsArmor)
		&& aMessage->ReadUInt(this->m_HighScoreAsArmor)
		&& aMessage->ReadUInt(this->m_ScoreAsAir)
		&& aMessage->ReadUInt(this->m_HighScoreAsAir)
		&& aMessage->ReadUInt(this->m_ScoreByDamagingEnemies)
		&& aMessage->ReadUInt(this->m_ScoreByUsingTacticalAid)
		&& aMessage->ReadUInt(this->m_ScoreByCapturingCommandPoints)
		&& aMessage->ReadUInt(this->m_ScoreByRepairing)
		&& aMessage->ReadUInt(this->m_ScoreByFortifying)
		&& aMessage->ReadUInt(this->m_HighestScore)
		&& aMessage->ReadUInt(this->m_CurrentLadderPosition)
		&& aMessage->ReadUInt(this->m_TimeTotalMatchLength)
		&& aMessage->ReadUInt(this->m_TimePlayedAsUSA)
		&& aMessage->ReadUInt(this->m_TimePlayedAsUSSR)
		&& aMessage->ReadUInt(this->m_TimePlayedAsNATO)
		&& aMessage->ReadUInt(this->m_TimePlayedAsInfantry)
		&& aMessage->ReadUInt(this->m_TimePlayedAsSupport)
		&& aMessage->ReadUInt(this->m_TimePlayedAsArmor)
		&& aMessage->ReadUInt(this->m_TimePlayedAsAir)
		&& aMessage->ReadUInt(this->m_NumberOfMatches)
		&& aMessage->ReadUInt(this->m_NumberOfMatchesWon)
		&& aMessage->ReadUInt(this->m_NumberOfMatchesLost)
		&& aMessage->ReadUInt(this->m_NumberOfAssaultMatches)
		&& aMessage->ReadUInt(this->m_NumberOfAssaultMatchesWon)
		&& aMessage->ReadUInt(this->m_NumberOfDominationMatches)
		&& aMessage->ReadUInt(this->m_NumberOfDominationMatchesWon)
		&& aMessage->ReadUInt(this->m_NumberOfTugOfWarMatches)
		&& aMessage->ReadUInt(this->m_NumberOfTugOfWarMatchesWon)
		&& aMessage->ReadUInt(this->m_CurrentWinningStreak)
		&& aMessage->ReadUInt(this->m_BestWinningStreak)
		&& aMessage->ReadUInt(this->m_NumberOfMatchesWonByTotalDomination)
		&& aMessage->ReadUInt(this->m_NumberOfPerfectDefendsInAssaultMatch)
		&& aMessage->ReadUInt(this->m_NumberOfPerfectPushesInTugOfWarMatch)
		&& aMessage->ReadUInt(this->m_NumberOfUnitsKilled)
		&& aMessage->ReadUInt(this->m_NumberOfUnitsLost)
		&& aMessage->ReadUInt(this->m_NumberOfReinforcementPointsSpent)
		&& aMessage->ReadUInt(this->m_NumberOfTacticalAidPointsSpent)
		&& aMessage->ReadUInt(this->m_NumberOfNukesDeployed)
		&& aMessage->ReadUInt(this->m_NumberOfTacticalAidCriticalHits);
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

bool MMG_Stats::ClanStatsRsp::FromStream(MN_ReadMessage *aMessage)
{
	return aMessage->ReadUInt(this->m_ClanId)
		&& aMessage->ReadUInt(this->m_LastMatchPlayed)
		&& aMessage->ReadUInt(this->m_MatchesPlayed)
		&& aMessage->ReadUInt(this->m_MatchesWon)
		&& aMessage->ReadUInt(this->m_BestWinningStreak)
		&& aMessage->ReadUInt(this->m_CurrentWinningStreak)
		&& aMessage->ReadUInt(this->m_CurrentLadderPosition)
		&& aMessage->ReadUInt(this->m_TournamentsPlayed)
		&& aMessage->ReadUInt(this->m_TournamentsWon)
		&& aMessage->ReadUInt(this->m_DominationMatchesPlayed)
		&& aMessage->ReadUInt(this->m_DominationMatchesWon)
		&& aMessage->ReadUInt(this->m_DominationMatchesWonByTotalDomination)
		&& aMessage->ReadUInt(this->m_AssaultMatchesPlayed)
		&& aMessage->ReadUInt(this->m_AssaultMatchesWon)
		&& aMessage->ReadUInt(this->m_AssaultMatchesPerfectDefense)
		&& aMessage->ReadUInt(this->m_TowMatchesPlayed)
		&& aMessage->ReadUInt(this->m_TowMatchesMatchesWon)
		&& aMessage->ReadUInt(this->m_TowMatchesMatchesPerfectPushes)
		&& aMessage->ReadUInt(this->m_NumberOfUnitsKilled)
		&& aMessage->ReadUInt(this->m_NumberOfUnitsLost)
		&& aMessage->ReadUInt(this->m_NumberOfNukesDeployed)
		&& aMessage->ReadUInt(this->m_NumberOfTACriticalHits)
		&& aMessage->ReadUInt(this->m_TimeAsUSA)
		&& aMessage->ReadUInt(this->m_TimeAsUSSR)
		&& aMessage->ReadUInt(this->m_TimeAsNATO)
		&& aMessage->ReadUInt(this->m_TotalScore)
		&& aMessage->ReadUInt(this->m_HighestScoreInAMatch)
		&& aMessage->ReadUInt(this->m_ScoreByDamagingEnemies)
		&& aMessage->ReadUInt(this->m_ScoreByRepairing)
		&& aMessage->ReadUInt(this->m_ScoreByTacticalAid)
		&& aMessage->ReadUInt(this->m_ScoreByFortifying)
		&& aMessage->ReadUInt(this->m_ScoreAsInfantry)
		&& aMessage->ReadUInt(this->m_ScoreAsSupport)
		&& aMessage->ReadUInt(this->m_ScoreAsAir)
		&& aMessage->ReadUInt(this->m_ScoreAsArmor)
		&& aMessage->ReadUInt(this->m_HighScoreAsInfantry)
		&& aMessage->ReadUInt(this->m_HighScoreAsSupport)
		&& aMessage->ReadUInt(this->m_HighScoreAsAir)
		&& aMessage->ReadUInt(this->m_HighScoreAsArmor);
}

MMG_Stats::PlayerMatchStats::PlayerMatchStats()
{
	m_GameProtocolVersion				= 0;
	m_ProfileId							= 0;
	m_ScoreTotal						= 0;
	m_ScoreAsInfantry					= 0;
	m_ScoreAsSupport					= 0;
	m_ScoreAsArmor						= 0;
	m_ScoreAsAir						= 0;
	m_ScoreByDamagingEnemies			= 0;
	m_ScoreByUsingTacticalAids			= 0;
	m_ScoreByCommandPointCaptures		= 0;
	m_ScoreByRepairing					= 0;
	m_ScoreByFortifying					= 0;
	m_ScoreLostByKillingFriendly		= 0;
	m_TimeTotalMatchLength				= 0;
	m_TimePlayedAsUSA					= 0;
	m_TimePlayedAsUSSR					= 0;
	m_TimePlayedAsNATO					= 0;
	m_TimePlayedAsInfantry				= 0;
	m_TimePlayedAsAir					= 0;
	m_TimePlayedAsArmor					= 0;
	m_TimePlayedAsSupport				= 0;
	m_MatchWon							= 0;
	m_MatchType							= 0;
	m_MatchLost							= 0;
	m_MatchWasFlawlessVictory			= 0;
	m_NumberOfUnitsKilled				= 0;
	m_NumberOfUnitsLost					= 0;
	m_NumberOfCommandPointCaptures		= 0;
	m_NumberOfReinforcementPointsSpent	= 0;
	m_NumberOfRoleChanges				= 0;
	m_NumberOfTacticalAidPointsSpent	= 0;
	m_NumberOfNukesDeployed				= 0;
	m_NumberOfTacticalAidCriticalHits	= 0;
	m_BestData							= 0;
	m_TotalTimePlayed					= 0.0f;
	m_WasPlayingAtMatchEnd				= 0;
}

void MMG_Stats::PlayerMatchStats::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(MassgateProtocolVersion);
	aMessage->WriteUInt(this->m_ProfileId);
	aMessage->WriteUShort(this->m_TimePlayedAsUSA);
	aMessage->WriteUShort(this->m_TimePlayedAsUSSR);
	aMessage->WriteUShort(this->m_TimePlayedAsNATO);
	aMessage->WriteUShort(this->m_TimePlayedAsInfantry);
	aMessage->WriteUShort(this->m_TimePlayedAsAir);
	aMessage->WriteUShort(this->m_TimePlayedAsArmor);
	aMessage->WriteUShort(this->m_TimePlayedAsSupport);
	aMessage->WriteUShort(this->m_ScoreTotal);
	aMessage->WriteUShort(this->m_ScoreAsInfantry);
	aMessage->WriteUShort(this->m_ScoreAsSupport);
	aMessage->WriteUShort(this->m_ScoreAsArmor);
	aMessage->WriteUShort(this->m_ScoreAsAir);
	aMessage->WriteUShort(this->m_ScoreByDamagingEnemies);
	aMessage->WriteUShort(this->m_ScoreByRepairing);
	aMessage->WriteUShort(this->m_ScoreByUsingTacticalAids);
	aMessage->WriteUShort(this->m_ScoreByCommandPointCaptures);
	aMessage->WriteUShort(this->m_ScoreByFortifying);
	aMessage->WriteUShort(this->m_ScoreLostByKillingFriendly);
	aMessage->WriteUShort(this->m_NumberOfUnitsKilled);
	aMessage->WriteUShort(this->m_NumberOfUnitsLost);
	aMessage->WriteUShort(this->m_NumberOfCommandPointCaptures);
	aMessage->WriteUShort(this->m_NumberOfReinforcementPointsSpent);
	aMessage->WriteUShort(this->m_NumberOfTacticalAidPointsSpent);
	aMessage->WriteUShort(this->m_NumberOfNukesDeployed);
	aMessage->WriteUShort(this->m_NumberOfTacticalAidCriticalHits);
	aMessage->WriteUShort(this->m_NumberOfRoleChanges);
	aMessage->WriteUShort(this->m_TimeTotalMatchLength);
	aMessage->WriteUChar(this->m_MatchType);
	aMessage->WriteUChar(this->m_MatchWon);
	aMessage->WriteUChar(this->m_MatchLost);
	aMessage->WriteUChar(this->m_MatchWasFlawlessVictory);
	aMessage->WriteUChar(this->m_BestData);
}

bool MMG_Stats::PlayerMatchStats::FromStream(MN_ReadMessage *aMessage)
{
	// Validate protocol first
	if (!aMessage->ReadUInt(this->m_GameProtocolVersion))
		return false;

	if (this->m_GameProtocolVersion != MassgateProtocolVersion)
		return false;

	return aMessage->ReadUInt(this->m_ProfileId)
		&& aMessage->ReadUShort(this->m_TimePlayedAsUSA)
		&& aMessage->ReadUShort(this->m_TimePlayedAsUSSR)
		&& aMessage->ReadUShort(this->m_TimePlayedAsNATO)
		&& aMessage->ReadUShort(this->m_TimePlayedAsInfantry)
		&& aMessage->ReadUShort(this->m_TimePlayedAsAir)
		&& aMessage->ReadUShort(this->m_TimePlayedAsArmor)
		&& aMessage->ReadUShort(this->m_TimePlayedAsSupport)
		&& aMessage->ReadUShort(this->m_ScoreTotal)
		&& aMessage->ReadUShort(this->m_ScoreAsInfantry)
		&& aMessage->ReadUShort(this->m_ScoreAsSupport)
		&& aMessage->ReadUShort(this->m_ScoreAsArmor)
		&& aMessage->ReadUShort(this->m_ScoreAsAir)
		&& aMessage->ReadUShort(this->m_ScoreByDamagingEnemies)
		&& aMessage->ReadUShort(this->m_ScoreByRepairing)
		&& aMessage->ReadUShort(this->m_ScoreByUsingTacticalAids)
		&& aMessage->ReadUShort(this->m_ScoreByCommandPointCaptures)
		&& aMessage->ReadUShort(this->m_ScoreByFortifying)
		&& aMessage->ReadUShort(this->m_ScoreLostByKillingFriendly)
		&& aMessage->ReadUShort(this->m_NumberOfUnitsKilled)
		&& aMessage->ReadUShort(this->m_NumberOfUnitsLost)
		&& aMessage->ReadUShort(this->m_NumberOfCommandPointCaptures)
		&& aMessage->ReadUShort(this->m_NumberOfReinforcementPointsSpent)
		&& aMessage->ReadUShort(this->m_NumberOfTacticalAidPointsSpent)
		&& aMessage->ReadUShort(this->m_NumberOfNukesDeployed)
		&& aMessage->ReadUShort(this->m_NumberOfTacticalAidCriticalHits)
		&& aMessage->ReadUShort(this->m_NumberOfRoleChanges)
		&& aMessage->ReadUShort(this->m_TimeTotalMatchLength)
		&& aMessage->ReadUChar(this->m_MatchType)
		&& aMessage->ReadUChar(this->m_MatchWon)
		&& aMessage->ReadUChar(this->m_MatchLost)
		&& aMessage->ReadUChar(this->m_MatchWasFlawlessVictory)
		&& aMessage->ReadUChar(this->m_BestData);
}

MMG_Stats::ClanMatchStats::ClanMatchStats()
{
	m_GameProtocolVersion		= 0;
	m_MapHash					= 0;
	memset(m_MapName, 0, sizeof(m_MapName));
	m_ClanId					= 0;
	m_MatchType					= 0;
	m_DominationPercentage		= 0;
	m_AssaultNumCommandPoints	= 0;
	m_AssaultEndTime			= 0;
	m_TowNumLinesPushed			= 0;
	m_TowNumPerimiterPoints		= 0;
	m_MatchWon					= 0;
	m_IsTournamentMatch			= 0;
	m_MatchWasFlawlessVictory	= 0;
	m_NumberOfUnitsKilled		= 0;
	m_NumberOfUnitsLost			= 0;
	m_NumberOfNukesDeployed		= 0;
	m_NumberOfTACriticalHits	= 0;
	m_TimeAsUSA					= 0;
	m_TimeAsUSSR				= 0;
	m_TimeAsNATO				= 0;
	m_TotalScore				= 0;
	m_ScoreByDamagingEnemies	= 0;
	m_ScoreByRepairing			= 0;
	m_ScoreByTacticalAid		= 0;
	m_ScoreByFortifying			= 0;
	m_ScoreByInfantry			= 0;
	m_ScoreBySupport			= 0;
	m_ScoreByAir				= 0;
	m_ScoreByArmor				= 0;
	m_PerPlayerDataCount		= 0;
	memset(m_PerPlayerData, 0, sizeof(m_PerPlayerData));
}

void MMG_Stats::ClanMatchStats::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(MassgateProtocolVersion);
	aMessage->WriteUInt64(this->m_MapHash);
	aMessage->WriteString(this->m_MapName);
	aMessage->WriteUInt(this->m_ClanId);
	aMessage->WriteUChar(this->m_MatchType);

	switch (this->m_MatchType)
	{
	case 0:
		aMessage->WriteUShort(this->m_DominationPercentage);
		break;

	case 1:
		aMessage->WriteUChar(this->m_AssaultNumCommandPoints);
		aMessage->WriteUInt(this->m_AssaultEndTime);
		break;

	case 2:
		aMessage->WriteUChar(this->m_TowNumLinesPushed);
		aMessage->WriteUChar(this->m_TowNumPerimiterPoints);
		break;
	}

	aMessage->WriteUChar(this->m_MatchWon);
	aMessage->WriteUChar(this->m_IsTournamentMatch);
	aMessage->WriteUChar(this->m_MatchWasFlawlessVictory);
	aMessage->WriteUInt(this->m_NumberOfUnitsKilled);
	aMessage->WriteUInt(this->m_NumberOfUnitsLost);
	aMessage->WriteUInt(this->m_NumberOfNukesDeployed);
	aMessage->WriteUInt(this->m_NumberOfTACriticalHits);
	aMessage->WriteUInt(this->m_TimeAsUSA);
	aMessage->WriteUInt(this->m_TimeAsUSSR);
	aMessage->WriteUInt(this->m_TimeAsNATO);
	aMessage->WriteUInt(this->m_TotalScore);
	aMessage->WriteUInt(this->m_ScoreByDamagingEnemies);
	aMessage->WriteUInt(this->m_ScoreByRepairing);
	aMessage->WriteUInt(this->m_ScoreByTacticalAid);
	aMessage->WriteUInt(this->m_ScoreByFortifying);
	aMessage->WriteUInt(this->m_ScoreByInfantry);
	aMessage->WriteUInt(this->m_ScoreBySupport);
	aMessage->WriteUInt(this->m_ScoreByAir);
	aMessage->WriteUInt(this->m_ScoreByArmor);
	aMessage->WriteUInt(this->m_PerPlayerDataCount);

	for (uint i = 0; i < this->m_PerPlayerDataCount; i++)
	{
		aMessage->WriteUInt(this->m_PerPlayerData[i].m_ProfileId);
		aMessage->WriteUShort(this->m_PerPlayerData[i].m_Score);
		aMessage->WriteUChar(this->m_PerPlayerData[i].m_Role);
	}
}

bool MMG_Stats::ClanMatchStats::FromStream(MN_ReadMessage *aMessage)
{
	// Validate protocol first
	if (!aMessage->ReadUInt(this->m_GameProtocolVersion))
		return false;

	if (this->m_GameProtocolVersion != MassgateProtocolVersion)
		return false;

	if (!aMessage->ReadUInt64(this->m_MapHash) ||
		!aMessage->ReadString(this->m_MapName, ARRAYSIZE(this->m_MapName)) ||
		!aMessage->ReadUInt(this->m_ClanId) ||
		!aMessage->ReadUChar(this->m_MatchType))
		return false;

	switch (this->m_MatchType)
	{
	case 0:
		if (!aMessage->ReadUShort(this->m_DominationPercentage))
			return false;
		break;

	case 1:
		if (!aMessage->ReadUChar(this->m_AssaultNumCommandPoints) ||
			!aMessage->ReadUInt(this->m_AssaultEndTime))
			return false;
		break;

	case 2:
		if (!aMessage->ReadUChar(this->m_TowNumLinesPushed) ||
			!aMessage->ReadUChar(this->m_TowNumPerimiterPoints))
			return false;
		break;
	}

	if (!aMessage->ReadUChar(this->m_MatchWon) ||
		!aMessage->ReadUChar(this->m_IsTournamentMatch) ||
		!aMessage->ReadUChar(this->m_MatchWasFlawlessVictory) ||
		!aMessage->ReadUInt(this->m_NumberOfUnitsKilled) ||
		!aMessage->ReadUInt(this->m_NumberOfUnitsLost) ||
		!aMessage->ReadUInt(this->m_NumberOfNukesDeployed) ||
		!aMessage->ReadUInt(this->m_NumberOfTACriticalHits) ||
		!aMessage->ReadUInt(this->m_TimeAsUSA) ||
		!aMessage->ReadUInt(this->m_TimeAsUSSR) ||
		!aMessage->ReadUInt(this->m_TimeAsNATO) ||
		!aMessage->ReadUInt(this->m_TotalScore) ||
		!aMessage->ReadUInt(this->m_ScoreByDamagingEnemies) ||
		!aMessage->ReadUInt(this->m_ScoreByRepairing) ||
		!aMessage->ReadUInt(this->m_ScoreByTacticalAid) ||
		!aMessage->ReadUInt(this->m_ScoreByFortifying) ||
		!aMessage->ReadUInt(this->m_ScoreByInfantry) ||
		!aMessage->ReadUInt(this->m_ScoreBySupport) ||
		!aMessage->ReadUInt(this->m_ScoreByAir) ||
		!aMessage->ReadUInt(this->m_ScoreByArmor) ||
		!aMessage->ReadUInt(this->m_PerPlayerDataCount))
		return false;

	for (uint i = 0; i < this->m_PerPlayerDataCount; i++)
	{
		if (!aMessage->ReadUInt(this->m_PerPlayerData[i].m_ProfileId) ||
			!aMessage->ReadUShort(this->m_PerPlayerData[i].m_Score) ||
			!aMessage->ReadUChar(this->m_PerPlayerData[i].m_Role))
			return false;
	}

	return true;
}

MMG_Stats::MMG_Stats()
{
}