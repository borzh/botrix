#include "source_engine.h"
#include "weapon.h"


#define BOT_AIM_ERROR 0


//----------------------------------------------------------------------------------------------------------------
good::vector<CWeaponWithAmmo> CWeapons::m_aWeapons;


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::GetLook( const Vector& vAim, float fDistance, bool bDuck,
                               TBotIntelligence iBotIntelligence, bool m_bSecondary, QAngle& angResult )
{
    BASSERT( m_pWeapon->bHasSecondary || !m_bSecondary, return );

    Vector v(vAim);
    if ( iBotIntelligence < EBotSmart ) // Aim at body instead of head.
        v.z -= bDuck ? (CMod::iPlayerEyeLevelCrouched/3) : (CMod::iPlayerEyeLevel/3);

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

    // Check to reload secondary ammo. It is automatic, no need to press reload button.
    if ( m_pWeapon->bHasSecondary && !m_pWeapon->bSecondaryUseSameBullets && !HasAmmoInClip(1) && HasAmmoExtra(1) )
        Reload(1);

#ifdef BOTRIX_AUTO_RELOAD_WEAPONS
    // Check to reload primary ammo.
    if ( !HasAmmoInClip(0) && HasAmmoExtra(0) )
        Reload(0);
#endif
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::Shoot( int iSecondary )
{
    GoodAssert( CanUse() && (HasAmmoInClip(iSecondary) || ( IsMelee() || IsPhysics() ) ) );
    if ( iSecondary && m_pWeapon->bSecondaryUseSameBullets )
        m_iBulletsInClip[0] -= m_pWeapon->iAttackBullets[iSecondary]; // Shotgun type: uses same bullets for primary and secondary attack.
    else
        m_iBulletsInClip[iSecondary] -= m_pWeapon->iAttackBullets[iSecondary];
    m_bShooting = true;
    m_bSecondary = (iSecondary != 0);
    m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fShotTime[iSecondary];
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::Holster( CWeaponWithAmmo* pSwitchFrom, CWeaponWithAmmo& cSwitchTo )
{
    if ( pSwitchFrom )
    {
        GoodAssert( !pSwitchFrom->m_bShooting );

        pSwitchFrom->m_bChanging = pSwitchFrom->m_bReloading = pSwitchFrom->m_bChangingZoom = pSwitchFrom->m_bUsingZoom = false;
        pSwitchFrom->m_fEndTime = CBotrixPlugin::fTime; // Save time when switched weapon.
    }

    if ( cSwitchTo.m_pWeapon->bBackgroundReload && !cSwitchTo.HasAmmoInClip(0) && cSwitchTo.HasAmmoExtra(0) &&
         CBotrixPlugin::fTime >= cSwitchTo.m_fEndTime + cSwitchTo.m_pWeapon->fReloadTime[0] )
    {
        cSwitchTo.m_bReloading = true;
        cSwitchTo.EndReload();
    }

    cSwitchTo.m_bChanging = true;
    cSwitchTo.m_fEndTime = CBotrixPlugin::fTime + cSwitchTo.m_pWeapon->fHolsterTime;
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::EndReload()
{
    GoodAssert( m_bReloading && (m_iBulletsInClip[m_bSecondary] < m_pWeapon->iClipSize[m_bSecondary]) );

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
void CWeapons::GetRespawnWeapons( good::vector<CWeaponWithAmmo>& aWeapons, TTeam iTeam, TClass iClass )
{
    aWeapons.clear();
    aWeapons.reserve( 8 );

    int iTeamFlag = 1 << iTeam;
    int iClassFlag = 1 << iClass;
    for ( int i=0; i < m_aWeapons.size(); ++i )
    {
        if ( FLAG_SOME_SET(iTeamFlag, m_aWeapons[i].GetBaseWeapon()->iTeam) &&
             FLAG_SOME_SET(iClassFlag, m_aWeapons[i].GetBaseWeapon()->iClass) )
            aWeapons.push_back( m_aWeapons[i] );
    }
}


//----------------------------------------------------------------------------------------------------------------
TWeaponId CWeapons::GetBestRangedWeapon( const good::vector<CWeaponWithAmmo>& aWeapons )
{
    // Choose best weapon. Skip grenades.
    bool bCanKill = false, bOneBullet = false;
    int iIdx = EWeaponIdInvalid;
    float fDamagePerSec = 0.0f, fDamage = 0.0f;
    for ( int i=0; i < aWeapons.size(); ++i )
    {
        const CWeaponWithAmmo& cWeapon = aWeapons[i];

        if ( cWeapon.GetBaseWeapon()->bForbidden || !cWeapon.IsPresent() ||
            !cWeapon.IsRanged() || !cWeapon.HasAmmo() ) // Skip all melees, grenades and physics or without ammo.
            continue;

        float fDamage0 = cWeapon.Damage(0);
        float fDamage1 = cWeapon.Damage(1);
        bool bOneBullet0 = (fDamage0 >= CMod::iPlayerMaxHealth);
        bool bOneBullet1 = (fDamage1 >= CMod::iPlayerMaxHealth);

        // Check if can kill with one bullet first.
        if ( bOneBullet0 && cWeapon.HasAmmoInClip(0) && (!bOneBullet || (fDamage < fDamage0)) )
        {
            iIdx = i;
            bOneBullet = bCanKill = true;
            fDamage = fDamage0;
            continue;
        }
        if ( bOneBullet1 && cWeapon.HasAmmoInClip(1) && (!bOneBullet || (fDamage < fDamage1))  )
        {
            iIdx = i;
            bOneBullet = bCanKill = true;
            fDamage = fDamage1;
            continue;
        }

        if ( bOneBullet ) // We have founded previously weapon that can kill with one bullet.
            continue;

        // Check if weapon has sufficient bullets to kill.
        fDamage0 = cWeapon.TotalDamage();
        bool bCanKill0 = (fDamage0 >= CMod::iPlayerMaxHealth); // Has enough bullets to kill?
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
        if ( fDamage < fDamage0 ) // Previous weapon does less damage.
        {
            iIdx = i;
            fDamage = fDamage0;
        }
    }
    return iIdx;
}


//----------------------------------------------------------------------------------------------------------------
TWeaponId CWeapons::GetRandomWeapon( TBotIntelligence iIntelligence, const good::bitset& cSkipWeapons )
{
    int iSize = MIN2( cSkipWeapons.size(), Size() );

    TWeaponId iIdx = rand() % iSize;
    for ( TWeaponId i = iIdx+1; i < iSize; ++i )
    {
        CWeaponWithAmmo& cWeapon = m_aWeapons[i];
        const CWeapon* pWeapon = cWeapon.GetBaseWeapon();
        if ( !pWeapon->bForbidden && cWeapon.IsRanged() && (iIntelligence <= pWeapon->iBotPreference) &&
             CItems::ExistsOnMap(pWeapon->pWeaponClass) && !cSkipWeapons.test(i) )
            return i;
    }
    for ( TWeaponId i = iIdx; i >= 0; --i )
    {
        CWeaponWithAmmo& cWeapon = m_aWeapons[i];
        const CWeapon* pWeapon = cWeapon.GetBaseWeapon();
        if ( !pWeapon->bForbidden && cWeapon.IsRanged() && (iIntelligence <= pWeapon->iBotPreference) &&
             CItems::ExistsOnMap(pWeapon->pWeaponClass) && !cSkipWeapons.test(i) )
            return i;
    }

    return -1;
}
