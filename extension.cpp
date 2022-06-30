#include "extension.h"

#include <map>
#include <iostream>

#include "CDetour/detours.h"
#include "structs.h"

AwesomeHvH g_Extension;
SMEXT_LINK( &g_Extension );

ICvar* g_pCVar = nullptr;
CGlobalVars* g_pGlobals = nullptr;
ConVar* sv_maxunlag = nullptr;

CDetour* g_RecordDataIntoTrack_Detour = nullptr;
CDetour* g_StartLagCompensation_Detour = nullptr;
CDetour* g_ClampBonesInBBox_Detour = nullptr;

IGameConfig* g_GameConfig = nullptr;

int g_flSimulationTimeOffset = 0;
int g_vecOriginOffset = 0;
int g_hActiveWeaponOffset = 0;
int g_fLastShotTimeOffset = 0;
int g_ViewAngleOffset = 0;
int g_CompensatedEntitiesOffset = 0;
int g_bLagCompensationOffset = 0;

struct LagListCustomData_t
{
    LagListCustomData_t( ) : m_vecOriginLast{}, m_flShotTimeLast{} {};

    Vector m_vecOriginLast;
    float m_flShotTimeLast;
};
std::map<void*, LagListCustomData_t> g_LagListCustomData;

ConVar sv_legacy_desync( "sv_legacy_desync", "0", FCVAR_RELEASE | FCVAR_REPLICATED, "Disables recently added valve shitcode that breaks lag compensation" );
ConVar sv_disable_lagpeek( "sv_disable_lagpeek", "1", FCVAR_RELEASE, "Disables lagpeek aka \"defensive doubletap\"" );
ConVar sv_disable_roll_aa( "sv_disable_roll_aa", "1", FCVAR_RELEASE, "Disables extended roll desync that was added by some uneducated valve employee" );
ConVar sv_force_lag_compensation( "sv_force_lag_compensation", "0", FCVAR_RELEASE, "Forces lagcompensation to avoid anti-exploiters with cl_lagcompensation set to 0" );

DETOUR_DECL_MEMBER2( ClampBonesInBBox, void, void*, matrix, int, mask )
{
    if ( sv_legacy_desync.GetBool( ) )
        return;

    auto& v_angle = *reinterpret_cast< Vector* >( uintptr_t( this ) + g_ViewAngleOffset );

    const auto roll_backup = v_angle.z;

    if ( sv_disable_roll_aa.GetBool( ) )
        v_angle.z = 0.f;

    DETOUR_MEMBER_CALL( ClampBonesInBBox )( matrix, mask );

    v_angle.z = roll_backup;
}

DETOUR_DECL_MEMBER3( RecordDataIntoTrack, void, void*, entity, LagRecordList*, track, bool, wantsAnims )
{
    if ( sv_disable_lagpeek.GetBool( ) )
    {
        // remove all records before that time:
        int flDeadtime = g_pGlobals->curtime - sv_maxunlag->GetFloat( );

        // remove tail records that are too old
        int tailIndex = track->Tail( );
        while ( track->IsValidIndex( tailIndex ) )
        {
            LagRecord& tail = track->Element( tailIndex );

            // if tail is within limits, stop
            if ( tail.m_flSimulationTime >= flDeadtime )
                break;

            // remove tail, get new tail
            track->Remove( tailIndex );
            tailIndex = track->Tail( );
        }

        auto lagListIter = g_LagListCustomData.find( track );
        if ( lagListIter == g_LagListCustomData.end( ) )
        {
            auto insert_result = g_LagListCustomData.insert( std::pair<void*, LagListCustomData_t>( track, LagListCustomData_t( ) ) );
            lagListIter = insert_result.first;
        }

        auto pLagListData = &lagListIter->second;

        auto& flSimulationTime = *reinterpret_cast< float* >( uintptr_t( entity ) + g_flSimulationTimeOffset );
        auto& vecOrigin = *reinterpret_cast< Vector* >( uintptr_t( entity ) + g_vecOriginOffset );

        auto flShotTime = [ entity ]( ) -> float
        {
            auto& hWeapon = *reinterpret_cast< CBaseHandle* >( uintptr_t( entity ) + g_hActiveWeaponOffset );

            auto pWeapon = gamehelpers->ReferenceToEntity( hWeapon.GetEntryIndex( ) );
            if ( !pWeapon )
                return 0.f;

            return *reinterpret_cast< float* >( uintptr_t( pWeapon ) + g_fLastShotTimeOffset );
        }( );

        if ( track->Count( ) > 0 )
        {
            LagRecord& head = track->Element( track->Head( ) );

            // detect lagpeek, skip hideshots
            if ( flSimulationTime <= head.m_flSimulationTime
                && vecOrigin != pLagListData->m_vecOriginLast
                && flShotTime == pLagListData->m_flShotTimeLast )
            {
                flSimulationTime = head.m_flSimulationTime + g_pGlobals->interval_per_tick;
                // g_pSM->LogMessage(myself, "simtime corrected");
            }
        }

        pLagListData->m_vecOriginLast = vecOrigin;
        pLagListData->m_flShotTimeLast = flShotTime;
    }

    DETOUR_MEMBER_CALL( RecordDataIntoTrack )( entity, track, wantsAnims );
};

DETOUR_DECL_MEMBER5( StartLagCompensation, void, void*, player, int, lagCompensationType, const Vector&, weaponPos, const QAngle&, weaponAngles, float, weaponRange )
{
    auto& m_CompensatedEntities = *reinterpret_cast< CUtlMap<EHANDLE, EntityLagData*>* >( uintptr_t( this ) + g_CompensatedEntitiesOffset );

    CUtlVector< EHANDLE > invalidList;
    FOR_EACH_MAP( m_CompensatedEntities, i )
    {
        EHANDLE key;
        key = m_CompensatedEntities.Key( i );
        if ( !gamehelpers->GetHandleEntity( key ) )
        {
            // Note that the EHANDLE is NULL now
            invalidList.AddToTail( key );
        }
    }

    // Wipe any deleted entities from the list
    for ( int i = 0; i < invalidList.Count( ); ++i )
    {
        int slot = m_CompensatedEntities.Find( invalidList[ i ] );
        Assert( slot != m_CompensatedEntities.InvalidIndex( ) );
        if ( slot == m_CompensatedEntities.InvalidIndex( ) )
            continue;

        EntityLagData* ld = m_CompensatedEntities[ slot ];

        g_LagListCustomData.erase( reinterpret_cast< void* >( &ld->m_LagRecords ) );
    }

    auto& player_bLagCompensation = *reinterpret_cast< bool* >( uintptr_t( player ) + g_bLagCompensationOffset );

    const auto player_bLagCompensation_backup = player_bLagCompensation;

    if ( sv_force_lag_compensation.GetBool( ) )
        player_bLagCompensation = true;

    DETOUR_MEMBER_CALL( StartLagCompensation )( player, lagCompensationType, weaponPos, weaponAngles, weaponRange );

    player_bLagCompensation = player_bLagCompensation_backup;
}

bool AwesomeHvH::SDK_OnLoad( char* error, size_t maxlength, bool late )
{
    if ( !gameconfs->LoadGameConfigFile( "awesome_hvh.games", &g_GameConfig, error, maxlength ) )
    {
        smutils->Format( error, maxlength - 1, "Failed to load gamedata" );
        return false;
    }

    if ( !g_GameConfig->GetOffset( "m_CompensatedEntities", &g_CompensatedEntitiesOffset ) )
    {
        smutils->Format( error, maxlength - 1, "Failed to find m_CompensatedEntities offset" );
        return false;
    };

    if ( !g_GameConfig->GetOffset( "m_bLagCompensation", &g_bLagCompensationOffset ) )
    {
        smutils->Format( error, maxlength - 1, "Failed to find m_bLagCompensation offset" );
        return false;
    };

    sm_sendprop_info_t info{};

    gamehelpers->FindSendPropInfo( "CBaseEntity", "m_flSimulationTime", &info );
    g_flSimulationTimeOffset = info.actual_offset;

    gamehelpers->FindSendPropInfo( "CBaseEntity", "m_vecOrigin", &info );
    g_vecOriginOffset = info.actual_offset;

    gamehelpers->FindSendPropInfo( "CBaseCombatCharacter", "m_hActiveWeapon", &info );
    g_hActiveWeaponOffset = info.actual_offset;

    gamehelpers->FindSendPropInfo( "CWeaponCSBase", "m_fLastShotTime", &info );
    g_fLastShotTimeOffset = info.actual_offset;

    gamehelpers->FindSendPropInfo( "CBasePlayer", "deadflag", &info );
    g_ViewAngleOffset = info.actual_offset + 4;

    CDetourManager::Init( smutils->GetScriptingEngine( ), g_GameConfig );

    g_RecordDataIntoTrack_Detour = DETOUR_CREATE_MEMBER( RecordDataIntoTrack, "RecordDataIntoTrack" );
    if ( !g_RecordDataIntoTrack_Detour )
    {
        smutils->Format( error, maxlength - 1, "Hooking RecordDataIntoTrack failed" );
        return false;
    }

    g_StartLagCompensation_Detour = DETOUR_CREATE_MEMBER( StartLagCompensation, "StartLagCompensation" );
    if ( !g_StartLagCompensation_Detour )
    {
        smutils->Format( error, maxlength - 1, "Hooking StartLagCompensation failed" );
        return false;
    }

    g_ClampBonesInBBox_Detour = DETOUR_CREATE_MEMBER( ClampBonesInBBox, "ClampBonesInBBox" );
    if ( !g_ClampBonesInBBox_Detour )
    {
        smutils->Format( error, maxlength - 1, "Hooking ClampBonesInBBox failed" );
        return false;
    }

    g_RecordDataIntoTrack_Detour->EnableDetour( );
    g_StartLagCompensation_Detour->EnableDetour( );
    g_ClampBonesInBBox_Detour->EnableDetour( );

    sharesys->RegisterLibrary( myself, "AwesomeHvH" );

    g_pSM->LogMessage( myself, "Loaded successfully" );

    return true;
}

void AwesomeHvH::SDK_OnUnload( )
{
    gameconfs->CloseGameConfigFile( g_GameConfig );

    g_RecordDataIntoTrack_Detour->DisableDetour( );
    g_StartLagCompensation_Detour->DisableDetour( );
    g_ClampBonesInBBox_Detour->DisableDetour( );

    g_pCVar->UnregisterConCommand( ( ConCommandBase* ) &sv_legacy_desync );
    g_pCVar->UnregisterConCommand( ( ConCommandBase* ) &sv_disable_lagpeek );
    g_pCVar->UnregisterConCommand( ( ConCommandBase* ) &sv_disable_roll_aa );
    g_pCVar->UnregisterConCommand( ( ConCommandBase* ) &sv_force_lag_compensation );
}

bool AwesomeHvH::SDK_OnMetamodLoad( ISmmAPI* ismm, char* error, size_t maxlen, bool late )
{
    g_pGlobals = ismm->GetCGlobals( );
    if ( !g_pGlobals )
    {
        smutils->Format( error, maxlen - 1, "Failed to find GlobalVars" );
        return false;
    };

    GET_V_IFACE_CURRENT( GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION );
    if ( !g_pCVar )
    {
        smutils->Format( error, maxlen - 1, "Failed to find ICvar interface" );
        return false;
    };

    sv_maxunlag = g_pCVar->FindVar( "sv_maxunlag" );
    if ( !sv_maxunlag )
    {
        smutils->Format( error, maxlen - 1, "Failed to find sv_maxunlag ConVar" );
        return false;
    };

    g_pCVar->RegisterConCommand( ( ConCommandBase* ) &sv_legacy_desync );
    g_pCVar->RegisterConCommand( ( ConCommandBase* ) &sv_disable_lagpeek );
    g_pCVar->RegisterConCommand( ( ConCommandBase* ) &sv_disable_roll_aa );
    g_pCVar->RegisterConCommand( ( ConCommandBase* ) &sv_force_lag_compensation );

    return true;
}