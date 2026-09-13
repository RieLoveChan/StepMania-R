// Characterization test for GameState's CoinMode (Home/Pay/Free) credit
// gating -- confirms Pay mode is fully functional, and pins a genuinely
// surprising, long-standing (upstream since 2011, not fork-specific)
// interaction: PrefsManager's EventMode preference defaults to true, and
// GameState::GetCoinMode() silently downgrades CoinMode_Pay to
// CoinMode_Free whenever EventMode is on -- so picking "Pay" in the
// System Options menu has no visible effect until EventMode is also
// turned off. See DocsAgents/adr/0006-test-harness.md and
// DocsAgents/modernization-backlog.md item 9.

#include "global.h"
#include "EngineTestEnv.h"

#include "GameState.h"
#include "GamePreferences.h"
#include "PrefsManager.h"
#include "GameConstantsAndTypes.h"

#include "catch_amalgamated.hpp"

namespace
{
	// Restores every preference this test file mutates, so other test
	// cases in the same process see the compiled defaults.
	struct CoinPrefsScopeGuard
	{
		CoinMode m_OldCoinMode;
		int m_iOldCoinsPerCredit;
		int m_iOldCoins;
		bool m_bOldEventMode;
		bool m_bOldTemporaryEventMode;

		CoinPrefsScopeGuard()
		{
			m_OldCoinMode = GamePreferences::m_CoinMode;
			m_iOldCoinsPerCredit = PREFSMAN->m_iCoinsPerCredit;
			m_iOldCoins = GAMESTATE->m_iCoins;
			m_bOldEventMode = PREFSMAN->m_bEventMode;
			m_bOldTemporaryEventMode = GAMESTATE->m_bTemporaryEventMode;
		}
		~CoinPrefsScopeGuard()
		{
			GamePreferences::m_CoinMode.Set( m_OldCoinMode );
			PREFSMAN->m_iCoinsPerCredit.Set( m_iOldCoinsPerCredit );
			GAMESTATE->m_iCoins.Set( m_iOldCoins );
			PREFSMAN->m_bEventMode.Set( m_bOldEventMode );
			GAMESTATE->m_bTemporaryEventMode = m_bOldTemporaryEventMode;
		}
	};
}

TEST_CASE( "CoinMode_Home never requires coins to join", "[GameState][CoinMode]" )
{
	EngineTestEnv::Require();
	CoinPrefsScopeGuard guard;

	GamePreferences::m_CoinMode.Set( CoinMode_Home );
	GAMESTATE->m_iCoins.Set( 0 );

	CHECK( GAMESTATE->GetCoinMode() == CoinMode_Home );
	CHECK( GAMESTATE->GetCoinsNeededToJoin() == 0 );
	CHECK( GAMESTATE->EnoughCreditsToJoin() );
}

TEST_CASE( "CoinMode_Free never requires coins to join", "[GameState][CoinMode]" )
{
	EngineTestEnv::Require();
	CoinPrefsScopeGuard guard;

	GamePreferences::m_CoinMode.Set( CoinMode_Free );
	GAMESTATE->m_iCoins.Set( 0 );

	CHECK( GAMESTATE->GetCoinMode() == CoinMode_Free );
	CHECK( GAMESTATE->GetCoinsNeededToJoin() == 0 );
	CHECK( GAMESTATE->EnoughCreditsToJoin() );
}

TEST_CASE( "EventMode (on by compiled default) silently downgrades CoinMode_Pay to Free", "[GameState][CoinMode]" )
{
	EngineTestEnv::Require();
	CoinPrefsScopeGuard guard;

	GamePreferences::m_CoinMode.Set( CoinMode_Pay );
	PREFSMAN->m_bEventMode.Set( true );
	GAMESTATE->m_bTemporaryEventMode = false;

	CHECK( GAMESTATE->IsEventMode() );
	CHECK( GAMESTATE->GetCoinMode() == CoinMode_Free );
	CHECK( GAMESTATE->GetCoinsNeededToJoin() == 0 );
}

TEST_CASE( "CoinMode_Pay charges CoinsPerCredit and gates joining once EventMode is off", "[GameState][CoinMode]" )
{
	EngineTestEnv::Require();
	CoinPrefsScopeGuard guard;

	GamePreferences::m_CoinMode.Set( CoinMode_Pay );
	PREFSMAN->m_bEventMode.Set( false );
	GAMESTATE->m_bTemporaryEventMode = false;
	PREFSMAN->m_iCoinsPerCredit.Set( 3 );

	CHECK_FALSE( GAMESTATE->IsEventMode() );
	CHECK( GAMESTATE->GetCoinMode() == CoinMode_Pay );
	CHECK( GAMESTATE->GetCoinsNeededToJoin() == 3 );

	GAMESTATE->m_iCoins.Set( 2 );
	CHECK_FALSE( GAMESTATE->EnoughCreditsToJoin() );

	GAMESTATE->m_iCoins.Set( 3 );
	CHECK( GAMESTATE->EnoughCreditsToJoin() );

	GAMESTATE->m_iCoins.Set( 4 );
	CHECK( GAMESTATE->EnoughCreditsToJoin() );
}
