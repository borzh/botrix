#include "source_engine.h"
#include "weapon.h"


#define BOT_AIM_ERROR 0


//----------------------------------------------------------------------------------------------------------------
good::vector<CWeaponWithAmmo> CWeapons::m_aWeapons;


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::GetLook( const Vector& vAim, float fDistance, bool bDuck,
                               TBotIntelligence iBotIntelligence, bool m_bSecondary, QAngle& angResult )
{
    DebugAssert( m_pWeapon->bHasSecondary || !m_bSecondary, return );

    Vector v(vAim);
    if ( iBotIntelligence < EBotSmart ) // Aim at body instead of head.
        v.z -= bDuck ? (CUtil::iPlayerEyeLevelCrouched/3) : (CUtil::iPlayerEyeLevel/3);

    VectorAngles(v, angResult);

    if ( m_pWeapon->iParabolic[m_bSecondary] != 0.0f )
        angResult.x += fDistance / m_pWeapon->iParabolic[m_bSecondary];

#if BOT_AIM_ERROR
    static const float fMaxErrorDistance = 1000.0f;

    // Aim errors (pitch/yaw): max error for a distance = 1000, and max error for a distance = 0.
    static const float aAimErrors[EBotIntelligenceTotal][2] =
    {
        // Pitch(when distance is 0) Yaw(when distance is 0). When distance is 1000 both pitch and yaw is 1.
        { 45, 30, }, // I.e. fool bot at distance 0 will have error in 45 degrees for pitch and 30 for yaw.
        { 35, 22, }, // At distance 1000 error should not be greater than 1 degree.
        { 25, 15, },
        { 15, 7,  },
        { 5,  5,  },
    };

    int iRangePitch = 1, iRangeYaw = 1;
    if ( fDistance < fMaxErrorDistance )
    {
        float fPercentage = fDistance / fMaxErrorDistance;
        iRangePitch = 2 * (int)(aAimErrors[iBotIntelligence][0] * fPercentage);
        iRangeYaw = 2 * (int)(aAimErrors[iBotIntelligence][1] * fPercentage);
    }

    int iRandPitch = (rand() % iRangePitch) - (iRangePitch >> 1); // 45 degrees, errors = 22.5 up and 22.5 down
    int iRandYaw = (rand() % iRangeYaw) - (iRangeYaw >> 1);

    angResult.x += iRandPitch;
    angResult.y += iRandYaw;
#endif // BOT_AIM_ERROR
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::GameFrame()
{
    if ( m_bShooting )
    {
        if ( CBotrixPlugin::fTime >= m_fEndTime )
            m_bShooting = false;
        else
            return;
    }
    else if ( m_bReloading )
    {
        if ( CBotrixPlugin::fTime >= m_fEndTime )
            EndReload();
        else
            return;
    }
    else if ( m_bChanging )
    {
        if ( CBotrixPlugin::fTime >= m_fEndTime )
            m_bChanging = false;
        else
            return;
    }
    else if ( m_bChangingZoom )
    {
        if ( CBotrixPlugin::fTime >= m_fEndTime )
            m_bChangingZoom = false;
        else
            return;
    }

    // Check to reload secondary ammo.
    if ( m_pWeapon->bHasSecondary && !m_pWeapon->bSecondaryUseSameBullets && !HasAmmoInClip(1) && HasAmmoExtra(1) )
    {
        Reload(1);
        if ( m_pWeapon->fReloadTime[1] == 0.0f )
            EndReload();
    }

    // Check to reload primary ammo.
    if ( !HasAmmoInClip(0) && HasAmmoExtra(0) )
    {
        Reload(0);
        if ( m_pWeapon->fReloadTime[0] == 0.0f )
            EndReload();
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::Shoot( bool bSecondary )
{
    DebugAssert( CanUse() && (HasAmmoInClip(bSecondary) || ( IsManual() || IsPhysics() ) ), return );
    if ( bSecondary && m_pWeapon->bSecondaryUseSameBullets )
        m_iBulletsInClip[0] -= m_pWeapon->iAttackBullets[bSecondary]; // Shotgun type: uses same bullets for primary and secondary attack.
    else
        m_iBulletsInClip[bSecondary] -= m_pWeapon->iAttackBullets[bSecondary];
    m_bShooting = true;
    m_bSecondary = bSecondary;
    m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fShotTime[bSecondary];
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::Holster( CWeaponWithAmmo& cSwitchTo )
{
    DebugAssert( !m_bShooting, return );

    m_bChanging = m_bReloading = m_bChangingZoom = m_bUsingZoom = false;
    m_fEndTime = CBotrixPlugin::fTime; // Save time when switched weapon.

    if ( cSwitchTo.m_pWeapon->bBackgroundReload && !cSwitchTo.HasAmmoInClip(0) && cSwitchTo.HasAmmoExtra(0) &&
         CBotrixPlugin::fTime >= cSwitchTo.m_fEndTime + m_pWeapon->fReloadTime[0] )
    {
        cSwitchTo.m_bReloading = true;
        cSwitchTo.EndReload();
    }

    cSwitchTo.m_bChanging = true;
    cSwitchTo.m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fHolsterTime;
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::EndReload()
{
    DebugAssert( m_bReloading && (m_iBulletsInClip[m_bSecondary] < m_pWeapon->iClipSize[m_bSecondary]) );

    if ( m_pWeapon->iType == EWeaponShotgun ) // Reload one bullet.
    {
        ++m_iBulletsInClip[m_bSecondary];
        --m_iBulletsExtra[m_bSecondary];
        if ( m_iBulletsInClip[m_bSecondary] < m_pWeapon->iClipSize[m_bSecondary] )
            m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fReloadTime[m_bSecondary];
        else
            m_bReloading = false;
    }
    else
    {
        int iBulletsAdd = m_pWeapon->iClipSize[m_bSecondary] - m_iBulletsInClip[m_bSecondary];
        if ( m_iBulletsExtra[m_bSecondary] < iBulletsAdd )
            iBulletsAdd = m_iBulletsExtra[m_bSecondary];
        m_iBulletsInClip[m_bSecondary] += iBulletsAdd;
        m_iBulletsExtra[m_bSecondary] -= iBulletsAdd;
        m_bReloading = false;
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWeapons::GetRespawnWeapons( good::vector<CWeaponWithAmmo>& aWeapons, int iTeam )
{
    DebugAssert( (aWeapons.size() == 0) || (aWeapons.size() == m_aWeapons.size()), aWeapons.clear() );

    aWeapons.reserve( m_aWeapons.size() );

    if ( aWeapons.size() == 0 )
    {
        for ( size_t i=0; i < m_aWeapons.size(); ++i )
            aWeapons.push_back( m_aWeapons[i] );
    }
    else
    {
        for ( size_t i=0; i < m_aWeapons.size(); ++i )
            aWeapons[i] = m_aWeapons[i];
    }

    // Remove default weapons that are not for that team.
    if (iTeam != CMod::iUnassignedTeam)
    {
        for ( size_t i=0; i < m_aWeapons.size(); ++i )
        {
            int iTeamOnly = m_aWeapons[i].GetBaseWeapon()->iTeamOnly;
            if ( (iTeamOnly != CMod::iUnassignedTeam) && (iTeamOnly != iTeam) )
                aWeapons[i].RemoveWeapon();
        }
    }
}


//----------------------------------------------------------------------------------------------------------------
TWeaponId CWeapons::GetBestRangedWeapon( const good::vector<CWeaponWithAmmo>& aWeapons )
{
    // Choose best weapon. Skip grenades.
    bool bCanKill = false, bOneBullet = false;
    int iIdx = -1, iDamage = 0;
    float fDamagePerSec = 0.0f;
    for ( size_t i=0; i < aWeapons.size(); ++i )
    {
        const CWeaponWithAmmo& cWeapon = aWeapons[i];

        if ( !cWeapon.IsPresent() || !cWeapon.IsRanged() || !cWeapon.HasAmmo() ) // Skip all manuals, grenades and physics or without ammo.
            continue;

        int iDamage0 = cWeapon.Damage(0);
        int iDamage1 = cWeapon.Damage(1);
        bool bOneBullet0 = (iDamage0 >= CUtil::iPlayerMaxHealth);
        bool bOneBullet1 = (iDamage1 >= CUtil::iPlayerMaxHealth);

        // Check if can kill with one bullet first.
        if ( bOneBullet0 && cWeapon.HasAmmoInClip(0) && (!bOneBullet || (iDamage < iDamage0)) )
        {
            iIdx = i;
            bOneBullet = bCanKill = true;
            iDamage = iDamage0;
            continue;
        }
        if ( bOneBullet1 && cWeapon.HasAmmoInClip(1) && (!bOneBullet || (iDamage < iDamage1))  )
        {
            iIdx = i;
            bOneBullet = bCanKill = true;
            iDamage = iDamage1;
            continue;
        }

        if ( bOneBullet ) // We have founded previously weapon that can kill with one bullet.
            continue;

        // Check if weapon has sufficient bullets to kill.
        iDamage0 = cWeapon.TotalDamage();
        bool bCanKill0 = iDamage0 >= CUtil::iPlayerMaxHealth;          // Has enough bullets to kill?
        float fDamagePerSec0 = cWeapon.DamagePerSecond(); // How fast this weapon kills.

        if ( bCanKill0 && (!bCanKill || (fDamagePerSec < fDamagePerSec0)) )
        {
            iIdx = i;
            bCanKill = true;
            fDamagePerSec = fDamagePerSec0;
            continue;
        }

        if ( bCanKill ) // We have founded previously weapon that has enough bullets to kill.
            continue;

        // Check total damage.
        if ( iDamage < iDamage0 ) // Previous weapon does less damage.
        {
            iIdx = i;
            iDamage = iDamage0;
        }
    }
    return iIdx;
}


//----------------------------------------------------------------------------------------------------------------
TWeaponId CWeapons::GetRandomWeapon( TBotIntelligence iIntelligence, const good::bitset& cSkipWeapons )
{
    TWeaponId iIdx = rand() % Size();

    for ( TWeaponId i = iIdx+1; i < Size(); ++i )
    {
        CWeaponWithAmmo& cWeapon = m_aWeapons[i];
        const CWeapon* pWeapon = cWeapon.GetBaseWeapon();
        if ( cWeapon.IsRanged() && (iIntelligence <= pWeapon->iBotPreference) && CItems::ExistsOnMap(pWeapon->pWeaponClass))
            if ( !cSkipWeapons.test(i) )
                return i;
    }
    for ( TWeaponId i = iIdx; i >= 0; --i )
    {
        CWeaponWithAmmo& cWeapon = m_aWeapons[i];
        const CWeapon* pWeapon = cWeapon.GetBaseWeapon();
        if ( cWeapon.IsRanged() && (iIntelligence <= pWeapon->iBotPreference) && CItems::ExistsOnMap(pWeapon->pWeaponClass))
            if ( !cSkipWeapons.test(i) )
                return i;
    }

    return -1;
}
