//========= Copyright Valve Corporation, All rights reserved. ============//
//
//  
//
//=============================================================================
#include "cbase.h"
#include "tf_fx_shared.h"
#include "tf_weaponbase.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "fx_impact.h"
// Server specific.
#else
#include "tf_fx.h"
#include "ilagcompensationmanager.h"
#include "tf_passtime_logic.h"
#endif

ConVar tf_use_fixed_weaponspreads( "tf_use_fixed_weaponspreads", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "If set to 1, weapons that fire multiple pellets per shot will use a non-random pellet distribution." );

// Client specific.
#ifdef CLIENT_DLL

class CGroupedSound
{
public:
	string_t	m_SoundName;
	Vector		m_vecPos;
};

CUtlVector<CGroupedSound> g_aGroupedSounds;

//-----------------------------------------------------------------------------
// Purpose: Called by the ImpactSound function.
//-----------------------------------------------------------------------------
void ImpactSoundGroup( const char *pSoundName, const Vector &vecEndPos )
{
	int iSound = 0;

	// Don't play the sound if it's too close to another impact sound.
	for ( iSound = 0; iSound < g_aGroupedSounds.Count(); ++iSound )
	{
		CGroupedSound *pSound = &g_aGroupedSounds[iSound];
		if ( pSound )
		{
			if ( vecEndPos.DistToSqr( pSound->m_vecPos ) < ( 300.0f * 300.0f ) )
			{
				if ( Q_stricmp( pSound->m_SoundName, pSoundName ) == 0 )
					return;
			}
		}
	}

	// Ok, play the sound and add it to the list.
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, NULL, pSoundName, &vecEndPos );

	iSound = g_aGroupedSounds.AddToTail();
	g_aGroupedSounds[iSound].m_SoundName = pSoundName;
	g_aGroupedSounds[iSound].m_vecPos = vecEndPos;
}

//-----------------------------------------------------------------------------
// Purpose: This is a cheap ripoff from CBaseCombatWeapon::WeaponSound().
//-----------------------------------------------------------------------------
void FX_WeaponSound( int iPlayer, WeaponSound_t soundType, const Vector &vecOrigin, CTFWeaponInfo *pWeaponInfo )
{
	// If we have some sounds from the weapon classname.txt file, play a random one of them
	const char *pShootSound = pWeaponInfo->aShootSounds[soundType]; 
	if ( !pShootSound || !pShootSound[0] )
		return;

	CBroadcastRecipientFilter filter; 
	if ( !te->CanPredict() )
		return;

	CBaseEntity::EmitSound( filter, iPlayer, pShootSound, &vecOrigin ); 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void StartGroupingSounds()
{
	Assert( g_aGroupedSounds.Count() == 0 );
	SetImpactSoundRoute( ImpactSoundGroup );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void EndGroupingSounds()
{
	g_aGroupedSounds.Purge();
	SetImpactSoundRoute( NULL );
}

// Server specific.
#else

// Server doesn't play sounds.
void FX_WeaponSound ( int iPlayer, WeaponSound_t soundType, const Vector &vecOrigin, CTFWeaponInfo *pWeaponInfo ) {}
void StartGroupingSounds() {}
void EndGroupingSounds() {}

#endif

// Shapes

// 10, Square
Vector g_vecFixedWpnSpreadSquare[] = 
{
	Vector( 0,0,0 ),	// First pellet goes down the middle
	Vector( 1,0,0 ),	
	Vector( -1,0,0 ),	
	Vector( 0,-1,0 ),	
	Vector( 0,1,0 ),	
	Vector( 0.85,-0.85,0 ),	
	Vector( 0.85,0.85,0 ),	
	Vector( -0.85,-0.85,0 ),	
	Vector( -0.85,0.85,0 ),	
	Vector( 0,0,0 ),	// last pellet goes down the middle as well to reward fine aim
};
const int g_iSpreadSquareCount = ARRAYSIZE(g_vecFixedWpnSpreadSquare);

// 15, Rectangle - slight noise applied below (+/- 0.07)
Vector g_vecFixedWpnSpreadRectangle [] =
{
	Vector( 0.f, 0.f, 0.f ),
	Vector( -0.5f, 0.f, 0.f ),
	Vector( -1.f, 0.f, 0.f ),
	Vector( 0.5f, 0.f, 0.f ),
	Vector( 1.f, 0.f, 0.f ),

	Vector( 0.f, 0.5f, 0.f ),
	Vector( -0.5f, 0.5f, 0.f ),
	Vector( -1.f, 0.5f, 0.f ),
	Vector( 0.5f, 0.5f, 0.f ),
	Vector( 1.f, 0.5f, 0.f ),

	Vector( 0.f, -0.5f, 0.f ),
	Vector( -0.5f, -0.5f, 0.f ),
	Vector( -1.f, -0.5f, 0.f ),
	Vector( 0.5f, -0.5f, 0.f ),
	Vector( 1.f, -0.5f, 0.f ),

// 	Vector( 0.f, 0.f, 0.f ),
// 	Vector( 0.25f, 0.f, 0.f ),
// 	Vector( -0.25f, 0.f, 0.f ),
// 	Vector( 0.f, -0.25f, 0.f ),
// 	Vector( 0.f, 0.25f, 0.f ),
};
const int g_iSpreadRectangleCount = ARRAYSIZE(g_vecFixedWpnSpreadRectangle);

// 12, Ring
Vector g_vecFixedWpnSpreadRing[] =
{
	Vector(1.0f, 0.0f, 0.0f),
	Vector(0.866f, 0.5f, 0.0f),
	Vector(0.5f, 0.866f, 0.0f),
	Vector(0.0f, 1.0f, 0.0f),

	Vector(-0.5f, 0.866f, 0.0f),
	Vector(-0.866f, 0.5f, 0.0f),
	Vector(-1.0f, 0.0f, 0.0f),
	Vector(-0.866f, -0.5f, 0.0f),

	Vector(-0.5f, -0.866f, 0.0f),
	Vector(0.0f, -1.0f, 0.0f),
	Vector(0.5f, -0.866f, 0.0f),
	Vector(0.866f, -0.5f, 0.0f)
};
const int g_iSpreadRingCount = ARRAYSIZE(g_vecFixedWpnSpreadRing);

// 4, Quad
Vector g_vecFixedWpnSpreadQuad[] =
{
	Vector(-0.5f, -0.5f, 0.0f),
	Vector(0.5f, -0.5f, 0.0f),
	Vector(0.5f, 0.5f, 0.0f),
	Vector(-0.5f, 0.5f, 0.0f)
};
const int g_iSpreadQuadCount = ARRAYSIZE(g_vecFixedWpnSpreadQuad);

// 14, Hourglass
Vector g_vecFixedWpnSpreadHourglass[] =
{
	Vector(0.0f, 0.0f, 0.0f),
	Vector(0.33f, 0.5f, 0.0f),
	Vector(0.66f, 1.0f, 0.0f),
	Vector(0.0f, 0.5f, 0.0f),
	Vector(0.0f, 1.0f, 0.0f),
	Vector(-0.33f, 0.5f, 0.0f),
	Vector(-0.66f, 1.0f, 0.0f),
	Vector(-0.33f, -0.5f, 0.0f),
	Vector(-0.66f, -1.0f, 0.0f),
	Vector(0.0f, -0.5f, 0.0f),
	Vector(0.0f, -1.0f, 0.0f),
	Vector(0.33f, -0.5f, 0.0f),
	Vector(0.66f, -1.0f, 0.0f),
	Vector(0.0f, 0.0f, 0.0f)
};
const int g_iSpreadHourglassCount = ARRAYSIZE(g_vecFixedWpnSpreadHourglass);

// 10, Heart
Vector g_vecFixedWpnSpreadHeart[] =
{
	Vector(0.0f, -1.0f, 0.0f),
	Vector(0.5f, -0.6f, 0.0f),
	Vector(1.0f, 0.0f, 0.0f),
	Vector(1.0f, 0.5f, 0.0f),
	Vector(0.5f, 1.0f, 0.0f),
	Vector(0.0f, 0.5f, 0.0f),
	Vector(-0.5f, 1.0f, 0.0f),
	Vector(-1.0f, 0.5f, 0.0f),
	Vector(-1.0f, 0.0f, 0.0f),
	Vector(-0.5f, -0.6f, 0.0f)
};
const int g_iSpreadHeartCount = ARRAYSIZE(g_vecFixedWpnSpreadHeart);

//-----------------------------------------------------------------------------
// Purpose: This runs on both the client and the server.  On the server, it 
// only does the damage calculations.  On the client, it does all the effects.
//-----------------------------------------------------------------------------
void FX_FireBullets( CTFWeaponBase *pWpn, int iPlayer, const Vector &vecOrigin, const QAngle &vecAngles,
					 int iWeapon, int iMode, int iSeed, float flSpread, float flDamage /* = -1.0f */, bool bCritical /* = false*/ )
{
	// Get the weapon information.
	const char *pszWeaponAlias = WeaponIdToAlias( iWeapon );
	if ( !pszWeaponAlias )
	{
		DevMsg( 1, "FX_FireBullets: weapon alias for ID %i not found\n", iWeapon );
		return;
	}

	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponAlias );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		DevMsg( 1, "FX_FireBullets: LookupWeaponInfoSlot failed for weapon %s\n", pszWeaponAlias );
		return;
	}

	CTFWeaponInfo *pWeaponInfo = static_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	if( !pWeaponInfo )
		return;

	bool bDoEffects = false;

#ifdef CLIENT_DLL
	C_TFPlayer *pPlayer = ToTFPlayer( ClientEntityList().GetBaseEntity( iPlayer ) );
#else
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayer ) );
#endif
	if ( !pPlayer )
		return;

// Client specific.
#ifdef CLIENT_DLL
	bDoEffects = true;

	// The minigun has custom sound & animation code to deal with its windup/down.
	if ( !pPlayer->IsLocalPlayer() 
		&& iWeapon != TF_WEAPON_MINIGUN )
	{
		// Fire the animation event.
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			if ( iMode == TF_WEAPON_PRIMARY_MODE )
			{
				pPlayer->m_PlayerAnimState->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
			}
			else
			{
				pPlayer->m_PlayerAnimState->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_SECONDARY );
			}
		}

		//FX_WeaponSound( pPlayer->entindex(), SINGLE, vecOrigin, pWeaponInfo );
	}

// Server specific.
#else
	// If this is server code, send the effect over to client as temp entity and 
	// dispatch one message for all the bullet impacts and sounds.
	TE_FireBullets( pPlayer->entindex(), vecOrigin, vecAngles, iWeapon, iMode, iSeed, flSpread, bCritical );

	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pPlayer->NoteWeaponFired();

#endif

	// Fire bullets, calculate impacts & effects.
	StartGroupingSounds();

#if !defined (CLIENT_DLL)
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
	
	// PASSTIME custom lag compensation for the ball; see also tf_weapon_flamethrower.cpp
	// it would be better if all entities could opt-in to this, or a way for lagcompensation to handle non-players automatically
	if ( g_pPasstimeLogic && g_pPasstimeLogic->GetBall() )
	{
		g_pPasstimeLogic->GetBall()->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
	}
#endif

	// Get the shooting angles.
	Vector vecShootForward, vecShootRight, vecShootUp;
	AngleVectors( vecAngles, &vecShootForward, &vecShootRight, &vecShootUp );

	// Initialize the static firing information.
	FireBulletsInfo_t fireInfo;
	fireInfo.m_vecSrc = vecOrigin;
	if ( flDamage < 0.0f )
	{
		fireInfo.m_flDamage = pWeaponInfo->GetWeaponData( iMode ).m_nDamage;
	}
	else
	{
		fireInfo.m_flDamage = flDamage;
	}
	fireInfo.m_flDistance = pWeaponInfo->GetWeaponData( iMode ).m_flRange;
	fireInfo.m_iShots = 1;
	fireInfo.m_vecSpread.Init( flSpread, flSpread, 0.0f );
	fireInfo.m_iAmmoType = pWeaponInfo->iAmmoType;

	// Ammo override
	int iModUseMetalOverride = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pWpn, iModUseMetalOverride, mod_use_metal_ammo_type );
	if ( iModUseMetalOverride )
	{
		fireInfo.m_iAmmoType = TF_AMMO_METAL;
	}

	// Setup the bullet damage type & roll for crit.
	int	nDamageType	= DMG_GENERIC;
	int nCustomDamageType = TF_DMG_CUSTOM_NONE;
	CTFWeaponBase *pWeapon = pPlayer->GetActiveTFWeapon(); // FIXME: Should this be pWpn?
	if ( pWeapon )
	{
		nDamageType	= pWeapon->GetDamageType();
		if ( pWeapon->IsCurrentAttackACrit() || bCritical )
		{
			nDamageType |= DMG_CRITICAL;
		}

		nCustomDamageType = pWeapon->GetCustomDamageType();
	}

	if ( iWeapon != TF_WEAPON_MINIGUN )
	{
		fireInfo.m_iTracerFreq = 2;
	}

	// Reset multi-damage structures.
	ClearMultiDamage();

#if !defined (CLIENT_DLL)
	// If this weapon fires multiple projectiles per shot, and can penetrate multiple
	// targets, aggregate CTakeDamageInfo events and send them off as one event
	CDmgAccumulator *pDmgAccumulator = pWpn ? pWpn->GetDmgAccumulator() : NULL;
	if ( pDmgAccumulator )
	{
		pDmgAccumulator->Start();
	}
#endif // !CLIENT

	int nBulletsPerShot = pWeaponInfo->GetWeaponData( iMode ).m_nBulletsPerShot;
	bool bFixedSpread = ( nDamageType & DMG_BUCKSHOT ) && ( nBulletsPerShot > 1 ) && IsFixedWeaponSpreadEnabled( pWpn );

	int iFixedSpread = 0;
	float flFixedSpreadVarianceMult = 1.0f;
	if ( pWeapon )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, iFixedSpread, fixed_shot_pattern);
		CALL_ATTRIB_HOOK_INT_ON_OTHER(pWeapon, flFixedSpreadVarianceMult, mult_fixed_shot_pattern_variance);


		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, nBulletsPerShot, mult_bullets_per_shot );
	}

	Vector* g_vecSpreadShape = g_vecFixedWpnSpreadSquare;
	int nSpreadCount = g_iSpreadSquareCount;
	bool bVariance = true;


	// Choose shape based on factors
	switch (iFixedSpread)
	{
		// Heart - 10
		case (5):
		{
			g_vecSpreadShape = g_vecFixedWpnSpreadHeart;
			nSpreadCount = g_iSpreadHeartCount;
			bVariance = true;
			break;
		}
		// Hourglass - 14
		case (4):
		{
			g_vecSpreadShape = g_vecFixedWpnSpreadHourglass;
			nSpreadCount = g_iSpreadHourglassCount;
			bVariance = true;
			break;
		}
		// Quad - 4
		case (3): 
		{
			g_vecSpreadShape = g_vecFixedWpnSpreadQuad;
			nSpreadCount = g_iSpreadQuadCount;
			bVariance = true;
			break;
		}
		// Ring - 12
		case (2):
		{
			g_vecSpreadShape = g_vecFixedWpnSpreadRing;
			nSpreadCount = g_iSpreadRingCount;
			bVariance = true;

			break;
		}

		// Default is any weapon or a wep with fixed spread set to 1
		default:
		{
			// Use big Rect spread if we have enoguh bullets
			if (nBulletsPerShot >= 15)
			{
				g_vecSpreadShape = g_vecFixedWpnSpreadRectangle;
				nSpreadCount = g_iSpreadRectangleCount;
				bVariance = true;
			}
			else
			{
				// If we have less than 15 bullets, use the square
				g_vecSpreadShape = g_vecFixedWpnSpreadSquare;
				nSpreadCount = g_iSpreadSquareCount;
				bVariance = false;
			}
			break;
		}
	}

	// Debug if we don't have enough bullets to complete the pattern (not counting the fallback ones)
	if (nSpreadCount != nBulletsPerShot)
	{
		if (iFixedSpread != 0 && iFixedSpread != 1) 
		{
			DevMsg("Not right enough of bullets for pattern! Needs %i, currently has %i \n", nSpreadCount, nBulletsPerShot);
		}
	}



	for ( int iBullet = 0; iBullet < nBulletsPerShot; ++iBullet )
	{
		// Initialize random system with this seed.
		RandomSeed( iSeed );	

		float x = 0.f;
		float y = 0.f;

		if ( bFixedSpread || iFixedSpread != 0)
		{
			int iSpread = iBullet;
			if (nBulletsPerShot == 1) 
			{
				// do funny spiral-like thing for weps w/ 1 shot
				iSpread = (pWpn->m_iConsecutiveShots * nBulletsPerShot)  % nSpreadCount;
			}
			else 
			{
				while (iSpread >= nSpreadCount)
				{
					iSpread -= nSpreadCount;
				}
			}

			float flScalar = 1.f;
			x = (g_vecSpreadShape[iSpread].x + (bVariance ? random->RandomFloat(-0.07f * flFixedSpreadVarianceMult, 0.07f * flFixedSpreadVarianceMult) : 0)) * flScalar;
			y = (g_vecSpreadShape[iSpread].y + (bVariance ? random->RandomFloat(-0.07f * flFixedSpreadVarianceMult, 0.07f * flFixedSpreadVarianceMult) : 0)) * flScalar;

		}
		else
		{
			float flVariance = 0.5f;

			if ( iBullet == 0 && pWpn )
			{
				bool bAccuracyBonus = false;
				float flTimeSinceLastShot = ( gpGlobals->curtime - pWpn->m_flLastFireTime );

				if ( nBulletsPerShot > 1 && flTimeSinceLastShot > 0.25f )
				{
					bAccuracyBonus = true;
				}
				else if ( nBulletsPerShot == 1 && flTimeSinceLastShot > 1.25f )
				{
					bAccuracyBonus = true;
				}

				if ( bAccuracyBonus )
				{
					float flMult = 0.f;

					// By default, all guns have perfect accuracy on the first shot (unless this attribute is present).
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWpn, flMult, mult_spread_scale_first_shot );

					flVariance = flMult;
				}
			}

			if ( flVariance != 0.f )
			{
				x = RandomFloat( -flVariance, flVariance ) + RandomFloat( -flVariance, flVariance );
				y = RandomFloat( -flVariance, flVariance ) + RandomFloat( -flVariance, flVariance );
			}
		}

		// Initialize the variable firing information.
		fireInfo.m_vecDirShooting = vecShootForward + ( x *  flSpread * vecShootRight ) + ( y * flSpread * vecShootUp );
		fireInfo.m_vecDirShooting.NormalizeInPlace();
		fireInfo.m_bUseServerRandomSeed = pWpn && pWpn->UseServerRandomSeed();

		// Fire a bullet.
		pPlayer->FireBullet( pWpn, fireInfo, bDoEffects, nDamageType, nCustomDamageType );

		// Use new seed for next bullet.
		++iSeed; 
	}

#if !defined (CLIENT_DLL)
	if ( pDmgAccumulator )
	{
		pDmgAccumulator->Process();
	}
#endif	// !CLIENT

	// Apply damage if any.
	ApplyMultiDamage();

#if !defined (CLIENT_DLL)
	lagcompensation->FinishLagCompensation( pPlayer );

	// PASSTIME custom lag compensation for the ball; see also tf_weapon_flamethrower.cpp
	// it would be better if all entities could opt-in to this, or a way for lagcompensation to handle non-players automatically
	if ( g_pPasstimeLogic && g_pPasstimeLogic->GetBall() )
	{
		g_pPasstimeLogic->GetBall()->FinishLagCompensation( pPlayer );
	}
#endif

	EndGroupingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: Should we make this a per-weapon property?
//-----------------------------------------------------------------------------
bool IsFixedWeaponSpreadEnabled( CTFWeaponBase *pWeapon /*= NULL*/ )
{
	bool bFixedSpread = tf_use_fixed_weaponspreads.GetBool();

	const IMatchGroupDescription *pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
	if ( pMatchDesc )
	{
		bFixedSpread = pMatchDesc->BUsesFixedWeaponSpread();
	}

	if ( pWeapon && !bFixedSpread )
	{
		int iFixedSpread = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iFixedSpread, fixed_shot_pattern );

		// Any fixed spread type not 0 counts
		if (iFixedSpread != 0) 
		{
			return true;
		}
	}

	return bFixedSpread;
}
