#ifndef __BOTRIX_WEAPON_H__
#define __BOTRIX_WEAPON_H__


#include "vector.h"

#include "item.h"
#include "mod.h"
#include "server_plugin.h"


//****************************************************************************************************************
/// Class to represent ammunition's item name along with bullets count.
//****************************************************************************************************************
class CAmmoClass
{
public:
	CAmmoClass( const CEntityClass* pAmmoClass, int iBullets ): pAmmoClass(pAmmoClass), iBullets(iBullets) {}
	const CEntityClass* pAmmoClass;   ///< Pointer to entity class.
	int iBullets;                     ///< How much bullets it gives.
};


//****************************************************************************************************************
/// Weapon abstract class. Used to get needed angles to aim.
//****************************************************************************************************************
class CWeapon
{
public:
	/// Default constructor.
	CWeapon()
	{
		memset(this, 0, sizeof(CWeapon));
		iAttackBullets[0] = iAttackBullets[1] = 1;
	}

	static bool IsValid( TWeaponId iId ) { return iId != -1; }

	static const int PRIMARY = 0;                ///< Index for primary ammo.
	static const int SECONDARY = 1;              ///< Index for secondary ammo.

	TWeaponId iId;                               ///< Weapon id.
	const CEntityClass* pWeaponClass;            ///< Pointer to weapon class.
	TWeaponType iType;                           ///< Weapon type.
	int iTeamOnly;                               ///< Only this class can buy this weapon.

	bool bHasSecondary:1;                        ///< True if weapon can perform secondary attacks.
	bool bSecondaryUseSameBullets:1;             ///< True if secondary attack uses same bullets as primary (shotgun for example).
	bool bForceAim:1;                            ///< Force to mantain aim while shooting (to rpg like weapons).
	bool bBackgroundReload:1;                    ///< Bug for some mods: some weapons reload while you hold another weapon (pistol for hl2dm).
	bool bAddClip:1;                             ///< Bug for some mods: some weapons add clip size to extra ammo when weapon is picked (crossbow for hl2dm).
	bool bForbidden:1;                           ///< Weapon was forbidden by "botrix bot restrict" command.
		
	float fMinDistanceSqr[2];                    ///< Minimum distance to enemy to safely use this weapon (0 by default).
	float fMaxDistanceSqr[2];                    ///< Maximum distance to enemy to be able to use this weapon (0 by default).
	float fHolsterTime;                          ///< Time from change weapon to be able to shoot.
	float fShotTime[2];                          ///< Duration of shoot (primary and secondary).
	float fReloadTime[2];                        ///< Duration of reload (primary and secondary).

	unsigned char iDamage[2];                    ///< How much damage one bullet does.
	unsigned char iClipSize[2];                  ///< How much bullets fit in one clip.
	unsigned char iDefaultAmmo[2];               ///< Count of bullets this weapon gives by default.
	unsigned char iMaxAmmo[2];                   ///< Count of max bullets this weapon can have (besides the clip in the weapon).
	unsigned char iAttackBullets[2];             ///< How many bullets are used in primary attack.
	int iParabolic[2];                           ///< Straight line if 0, else look 1 angle upper for each given distance.

	TBotIntelligence iBotPreference;             ///< Smart bots will prefer weapons with higher preference.

	good::vector<const CEntityClass*> aAmmos[2]; ///< Ammo item classes.
};


//****************************************************************************************************************
/// Weapon with ammunition.
//****************************************************************************************************************
class CWeaponWithAmmo
{
public:
	/// Constructor.
	CWeaponWithAmmo( const CWeapon* pWeapon )
	{
		memset(this, 0, sizeof(CWeaponWithAmmo));
		m_pWeapon = pWeapon;
	}

	/** 
	 * Get vector to aim to . iBotIntelligence is used to get some errors in aim.
	 * Returns false if can't use (invalid distance). Will return angles, based on random and 
	 * bot intelligence.
	 */
	void GetLook( const Vector& vAim, float fDistance, bool bDuck,
	              TBotIntelligence iBotIntelligence, bool m_bSecondary, QAngle& angResult );

	/// Game frame.
	void GameFrame();

	/// Return base weapon.
	const CWeapon* GetBaseWeapon() const { return m_pWeapon; }

	/// Return weapon's entity name.
	const good::string& GetName() const { return m_pWeapon->pWeaponClass->sClassName; }

	/// Return true if weapon is not reloading or shooting or changing zoom or changing to other weapon.
	bool CanUse() const { return !IsShooting() && !IsReloading() && !IsChanging() && !IsChangingZoom(); }

	/// Return true if weapon can be changed.
	bool CanChange() const { return !IsShooting(); }

	/// Return time when weapon can be used.
	float GetEndTime() const { return m_fEndTime; }

	/// Return true if can start shooting at enemy right away.
	bool CanUse( float fDistanceSqr ) const
	{
		if ( !CanUse() )
			return false;
		if ( HasAmmoInClip(0) && IsDistanceSafe(fDistanceSqr, 0) ) // Primary.
			return true;
		if ( m_pWeapon->bHasSecondary && !m_pWeapon->bSecondaryUseSameBullets && // Secondary.
			 HasAmmoInClip(1) && IsDistanceSafe(fDistanceSqr, 1) )
			return true;
		return false;
	}

	/// Return false if there is only ammo for this weapon, but without weapon itself.
	bool IsPresent() const { return m_bWeaponPresent && !m_pWeapon->bForbidden; }

	/// Return true if this weapon is ranged.
	bool IsRanged() const { return !IsManual() && !IsPhysics() && !IsGrenade(); }

	/// Return true if this weapon is manual.
	bool IsManual() const { return m_pWeapon->iType == EWeaponManual; }

	/// Return true if need to throw.
	bool IsGrenade() const { return (EWeaponGrenade <= m_pWeapon->iType) && (m_pWeapon->iType <= EWeaponRemoteDetonation); }

	/// Return true if weapon is sniper.
	bool IsSniper() const { return m_pWeapon->iType == EWeaponSniper; }

	/// Return true if weapon is physics.
	bool IsPhysics() const { return m_pWeapon->iType == EWeaponPhysics; }

	/// Return true if currently reloading.
	bool IsReloading() const { return m_bReloading; }

	/// Return true if currently shooting.
	bool IsShooting() const { return m_bShooting; }

	/// Return true if currently changing to this weapon.
	bool IsChanging() const { return m_bChanging; }

	/// Return true if currently using zoom.
	bool IsUsingZoom() const { return m_bUsingZoom; }

	/// Return true if currently zooming in or out.
	bool IsChangingZoom() const { return m_bChangingZoom && IsSniper(); }

	// Return true if can use this weapon for given distance to enemy (it is safe).
	bool IsDistanceSafe( float fDistanceSqr, bool bSecondary ) const
	{
		bool bMinPass = m_pWeapon->fMinDistanceSqr[bSecondary] <= fDistanceSqr;
		bool bMaxPass = (m_pWeapon->fMaxDistanceSqr[bSecondary] == 0.0f) || (fDistanceSqr <= m_pWeapon->fMaxDistanceSqr[bSecondary]);
		return bMinPass && bMaxPass;
	}

	/// Return damage per second this weapon can do with primary ammo.
	float DamagePerSecond() const { return (float)m_pWeapon->iDamage[0] / m_pWeapon->fShotTime[0]; }

	/// Return amount of bullets this weapon has for primary or secondary attacks.
	int Bullets( bool bSecondary ) const { return m_iBulletsInClip[bSecondary]; }

	/// Return amount of extra bullets this weapon has for primary or secondary attacks.
	int ExtraBullets( bool bSecondary ) const { return m_iBulletsExtra[bSecondary]; }

	/// Return approximate damage this weapon can do with bullets in clip.
	int Damage( bool bSecondary ) const { return m_pWeapon->iDamage[bSecondary]; }

	/// Return approximate damage this weapon can do with bullets in clip.
	int TotalDamage( bool bSecondary ) const { return Bullets(bSecondary) * m_pWeapon->iDamage[bSecondary]; }

	/// Return approximate damage this weapon can do without reload.
	int TotalDamage() const { return TotalDamage(0) + TotalDamage(1); }

	/// Return true if weapon needs to be reloaded.
	bool NeedReload( bool bSecondary ) const { return (m_iBulletsInClip[bSecondary] < m_pWeapon->iClipSize[bSecondary]) && HasAmmoExtra(bSecondary); }

	/// Return true if need to use zoom.
	bool ShouldZoom( float fDistanceToEnemySqr ) const
	{
		DebugAssert( IsSniper() );
		return fDistanceToEnemySqr >= m_pWeapon->fMinDistanceSqr[1];
	}

	/// Return true if weapon has ammo.
	bool HasAmmoInClip( bool bSecondary ) const { return m_iBulletsInClip[bSecondary] > 0; }

	/// Return true if weapon has ammo, beside the ammo in clip.
	bool HasAmmoExtra( bool bSecondary ) const { return m_iBulletsExtra[bSecondary] > 0; }

	/// Return true if weapon has full ammunition.
	bool FullAmmo( bool bSecondary ) const { return m_iBulletsExtra[bSecondary] == m_pWeapon->iMaxAmmo[bSecondary]; }

	/// Return true if weapon has ammo.
	bool HasAmmo() const { return HasAmmoInClip(0) || HasAmmoInClip(1) || HasAmmoExtra(0) || HasAmmoExtra(1); }

	/// Start to shoot weapon.
	void Shoot( bool bSecondary );

	/// Start to reload weapon.
	void Reload( bool bSecondary )
	{
		DebugAssert( NeedReload(bSecondary) && CanUse() );
		m_bReloading = true;
		m_bSecondary = bSecondary;
		m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fReloadTime[bSecondary];
	}

	/// Remove this weapon (called when player's team is different from this weapons's team).
	void RemoveWeapon() { m_bWeaponPresent = false; }

	/// Called when bot picks up this weapon.
	void AddWeapon( int iExtraAmmo0 = -1, int iExtraAmmo1 = -1 )
	{
		if ( iExtraAmmo0 < 0 )
			iExtraAmmo0 = m_pWeapon->iDefaultAmmo[0];
		if ( iExtraAmmo1 < 0 )
			iExtraAmmo1 = m_pWeapon->iDefaultAmmo[1];

		AddBullets(iExtraAmmo0, 0);
		AddBullets(iExtraAmmo1, 1);

		if ( m_bWeaponPresent ) // Just add bullets.
		{
			if ( m_pWeapon->bAddClip )
				AddBullets(m_pWeapon->iClipSize[0], 0);
		}
		else
			m_iBulletsInClip[0] = m_pWeapon->iClipSize[0]; // Picked weapon has full clip.

		m_bWeaponPresent = true;
	}

	/// Add bullets to this weapon.
	void AddBullets( int iCount, bool bSecondary )
	{
		m_iBulletsExtra[bSecondary] += iCount;
		if ( m_iBulletsExtra[bSecondary] > m_pWeapon->iMaxAmmo[bSecondary] )
			m_iBulletsExtra[bSecondary] = m_pWeapon->iMaxAmmo[bSecondary];
	}

	/// Change weapon.
	void Holster( CWeaponWithAmmo& cSwitchTo );

	/// Zoom in.
	void ZoomIn()
	{
		DebugAssert( IsSniper() );
		m_bChangingZoom = true;
		m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fShotTime[1];
		m_bUsingZoom = true;
	}

	/// Zoom out.
	void ZoomOut()
	{
		DebugAssert( IsSniper() );
		m_bChangingZoom = true;
		m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fShotTime[1];
		m_bUsingZoom = false;
	}

protected:
	// End reloading weapon.
	void EndReload();

	const CWeapon* m_pWeapon; ///< Weapon itself.
	int m_iBulletsInClip[2];  ///< Bullets in current clip (inside weapon).
	int m_iBulletsExtra[2];   ///< Amount of bullets extra.
	
	float m_fEndTime;         ///< Time to end reloading/shooting.

	bool m_bSecondary:1;      ///< Reloading / shooting secondary ammo or using zoom.

	bool m_bWeaponPresent:1;  ///< True, if weapon is present, false if only ammo is present.
	bool m_bReloading:1;      ///< True, if started to reload weapon.
	bool m_bShooting:1;       ///< True, if currently shooting.
	bool m_bChanging:1;       ///< True, if started to change weapon.
	bool m_bChangingZoom:1;   ///< True, if started to zoom in / zoom out.
	bool m_bUsingZoom:1;      ///< True, if started to zoom in / zoom out.
};



//****************************************************************************************************************
/// Available weapons.
//****************************************************************************************************************
class CWeapons
{

public:
	/// Return weapons count.
	static int Size() { return m_aWeapons.size(); }

	/// Get weapon from weapon id.
	static const CWeapon* Get( TWeaponId iWeaponId ) { return m_aWeapons[iWeaponId].GetBaseWeapon(); }

	/// Add weapon.
	static void Add( CWeaponWithAmmo& cWeapon ) { m_aWeapons.push_back(cWeapon); }

	/// Add default weapon.
	static void SetDefault( TWeaponId iWeaponId, int iExtraAmmo0, int iExtraAmmo1 )
	{
		DebugAssert( iWeaponId < m_aWeapons.size() );
		m_aWeapons[iWeaponId].AddWeapon(iExtraAmmo0, iExtraAmmo1);
	}

	/// Clear all weapons.
	static void Clear()
	{
		for ( int i=0; i < m_aWeapons.size(); ++i )
			delete m_aWeapons[i].GetBaseWeapon();
		m_aWeapons.clear();
	}

	/// Get default weapons with which player respawns.
	static void GetRespawnWeapons( good::vector<CWeaponWithAmmo>& aWeapons, int iTeam );

	/// Get weapon from weapon name.
	static TWeaponId GetIdFromWeaponName( const good::string& sName )
	{
		for ( int i=0; i < m_aWeapons.size(); ++i )
			if ( m_aWeapons[i].GetName() == sName )
				return i;
		return -1;
	}

	/// Get weapon from weapon class. Faster.
	static TWeaponId GetIdFromWeaponClass( const CEntityClass* pWeaponClass )
	{
		for ( int i=0; i < m_aWeapons.size(); ++i )
			if ( m_aWeapons[i].GetBaseWeapon()->pWeaponClass == pWeaponClass )
				return i;
		return -1;
	}

	/// Get weapon, ammo's count from weapon ammo's name.
	static TWeaponId GetIdFromAmmo( const CEntityClass* pAmmoClass, bool& bSecondary, int& iAmmoCount )
	{
		for ( int i=0; i < m_aWeapons.size(); ++i )
		{
			const CWeapon* pWeapon = m_aWeapons[i].GetBaseWeapon();
			for ( int bSec=0; bSec <= 1; ++bSec )
				for ( int j=0; j < pWeapon->aAmmos[bSec].size(); ++j )
					if ( pWeapon->aAmmos[bSec][j] == pAmmoClass )
					{
						bSecondary = bSec != 0;
						iAmmoCount = pWeapon->aAmmos[bSec][j]->GetArgument();
						return i;
					}
		}
		DebugAssert(false);
		return -1;
	}

	/// Allow given weapon.
	static void Allow( TWeaponId iWeaponId ) { ((CWeapon*)m_aWeapons[iWeaponId].GetBaseWeapon())->bForbidden = false; }

	/// Forbid given weapon.
	static void Forbid( TWeaponId iWeaponId ) { ((CWeapon*)m_aWeapons[iWeaponId].GetBaseWeapon())->bForbidden = true; }

	/// Get best ranged weapon.
	static TWeaponId GetBestRangedWeapon( const good::vector<CWeaponWithAmmo>& aWeapons );

	/// Get random weapon, based on bot intelligence.
	static TWeaponId GetRandomWeapon( TBotIntelligence iIntelligence );

protected:
	static good::vector< CWeaponWithAmmo > m_aWeapons; // Array of available weapons for this mod.
};


#endif // __BOTRIX_WEAPONS_H__
