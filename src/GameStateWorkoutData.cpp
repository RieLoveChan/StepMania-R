#include "global.h"
#include "GameStateWorkoutData.h"
#include "GameConstantsAndTypes.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "StatsManager.h"
#include "StageStats.h"

GameStateWorkoutData::GameStateWorkoutData()
{
	FOREACH_PlayerNumber( p )
		m_bGoalComplete[p] = false;
	m_bWorkoutGoalComplete = false;
}

float GameStateWorkoutData::GetGoalPercentComplete( PlayerNumber pn )
{
	const Profile *pProfile = PROFILEMAN->GetProfile(pn);
	const StageStats &ssCurrent = STATSMAN->m_CurStageStats;
	const PlayerStageStats &pssCurrent = ssCurrent.m_player[pn];

	float fActual = 0;
	float fGoal = 0;
	switch( pProfile->m_GoalType )
	{
	case GoalType_Calories:
		fActual = pssCurrent.m_fCaloriesBurned;
		fGoal = (float)pProfile->m_iGoalCalories;
		break;
	case GoalType_Time:
		fActual = ssCurrent.m_fGameplaySeconds;
		fGoal = (float)pProfile->m_iGoalSeconds;
		break;
	case GoalType_None:
		return 0;	// never complete
	default:
		FAIL_M(ssprintf("Invalid GoalType: %i", pProfile->m_GoalType));
	}
	if( fGoal == 0 )
		return 0;
	else
		return fActual / fGoal;
}
