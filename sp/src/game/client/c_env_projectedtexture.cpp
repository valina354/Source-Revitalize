//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#ifdef ASW_PROJECTED_TEXTURES
#include "C_Env_Projected_Texture.h"
#include "vprof.h"
#endif
#ifdef MAPBASE
#include "materialsystem/itexture.h"
#endif
#include "shareddefs.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "view.h"
#include "view_scene.h"
#include "viewrender.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "texture_group_names.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar gstring_volumetrics_fade_range("gstring_volumetrics_fade_range", "128.0", FCVAR_CHEAT);
ConVar gstring_volumetrics_enabled("gstring_volumetrics_enabled", "1", FCVAR_ARCHIVE);

void Gstring_volumetrics_subdiv_Callback(IConVar *var, const char *pOldValue, float flOldValue)
{
	for (C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity(); pEnt != NULL;
		pEnt = ClientEntityList().NextBaseEntity(pEnt))
	{
		C_EnvProjectedTexture *pProjectedTexture = dynamic_cast< C_EnvProjectedTexture* >(pEnt);
		if (pProjectedTexture != NULL)
		{
			pProjectedTexture->ClearVolumetricsMesh();
		}
	}
}
static ConVar gstring_volumetrics_subdiv("gstring_volumetrics_subdiv", "0", 0, "", Gstring_volumetrics_subdiv_Callback);

#ifdef ASW_PROJECTED_TEXTURES
extern ConVarRef mat_slopescaledepthbias_shadowmap;
extern ConVarRef mat_depthbias_shadowmap;

float C_EnvProjectedTexture::m_flVisibleBBoxMinHeight = -FLT_MAX;


IMPLEMENT_CLIENTCLASS_DT( C_EnvProjectedTexture, DT_EnvProjectedTexture, CEnvProjectedTexture )
	RecvPropEHandle( RECVINFO( m_hTargetEntity )	),
#ifdef MAPBASE
	RecvPropBool(	 RECVINFO( m_bDontFollowTarget )),
#endif
	RecvPropBool(	 RECVINFO( m_bState )			),
	RecvPropBool(	 RECVINFO( m_bAlwaysUpdate )	),
	RecvPropFloat(	 RECVINFO( m_flLightFOV )		),
#ifdef MAPBASE
	RecvPropFloat(	 RECVINFO( m_flLightHorFOV )	),
#endif
	RecvPropBool(	 RECVINFO( m_bEnableShadows )	),
	RecvPropBool(	 RECVINFO( m_bLightOnlyTarget ) ),
	RecvPropBool(	 RECVINFO( m_bLightWorld )		),
	RecvPropBool(	 RECVINFO( m_bCameraSpace )		),
	RecvPropFloat(	 RECVINFO( m_flBrightnessScale )	),
	RecvPropInt(	 RECVINFO( m_LightColor ), 0, RecvProxy_IntToColor32 ),
	RecvPropFloat(	 RECVINFO( m_flColorTransitionTime )		),
	RecvPropFloat(	 RECVINFO( m_flAmbient )		),
	RecvPropString(  RECVINFO( m_SpotlightTextureName ) ),
	RecvPropInt(	 RECVINFO( m_nSpotlightTextureFrame ) ),
	RecvPropFloat(	 RECVINFO( m_flNearZ )	),
	RecvPropFloat(	 RECVINFO( m_flFarZ )	),
	RecvPropInt(	 RECVINFO( m_nShadowQuality )	),
	RecvPropBool(RECVINFO(m_bEnableVolumetrics)),
	RecvPropBool(RECVINFO(m_bEnableVolumetricsLOD)),
	RecvPropFloat(RECVINFO(m_flVolumetricsFadeDistance)),
	RecvPropInt(RECVINFO(m_iVolumetricsQuality)),
	RecvPropFloat(RECVINFO(m_flVolumetricsQualityBias)),
	RecvPropFloat(RECVINFO(m_flVolumetricsMultiplier)),
#ifdef MAPBASE
	RecvPropFloat(	 RECVINFO( m_flConstantAtten ) ),
	RecvPropFloat(	 RECVINFO( m_flLinearAtten ) ),
	RecvPropFloat(	 RECVINFO( m_flQuadraticAtten ) ),
	RecvPropFloat(	 RECVINFO( m_flShadowAtten ) ),
	RecvPropFloat(   RECVINFO( m_flShadowFilter )  ),
	RecvPropBool(	 RECVINFO( m_bAlwaysDraw )	),

	// Not needed on the client right now, change when it actually is needed
	//RecvPropBool(	 RECVINFO( m_bProjectedTextureVersion )	),
#endif
END_RECV_TABLE()

C_EnvProjectedTexture *C_EnvProjectedTexture::Create( )
{
	C_EnvProjectedTexture *pEnt = new C_EnvProjectedTexture();

	pEnt->m_flNearZ = 4.0f;
	pEnt->m_flFarZ = 2000.0f;
//	strcpy( pEnt->m_SpotlightTextureName, "particle/rj" );
	pEnt->m_bLightWorld = true;
	pEnt->m_bLightOnlyTarget = false;
	pEnt->m_nShadowQuality = 1;
	pEnt->m_flLightFOV = 10.0f;
#ifdef MAPBASE
	pEnt->m_flLightHorFOV = 10.0f;
#endif
	pEnt->m_LightColor.r = 255;
	pEnt->m_LightColor.g = 255;
	pEnt->m_LightColor.b = 255;
	pEnt->m_LightColor.a = 255;
	pEnt->m_bEnableShadows = false;
	pEnt->m_flColorTransitionTime = 1.0f;
	pEnt->m_bCameraSpace = false;
	pEnt->SetAbsAngles( QAngle( 90, 0, 0 ) );
	pEnt->m_bAlwaysUpdate = true;
	pEnt->m_bState = true;
#ifdef MAPBASE
	pEnt->m_bAlwaysDraw = false;
	pEnt->m_flConstantAtten = 0.0f;
	pEnt->m_flLinearAtten = 100.0f;
	pEnt->m_flQuadraticAtten = 0.0f;
	pEnt->m_flShadowAtten = 0.0f;
	pEnt->m_flShadowFilter = 0.5f;
	//pEnt->m_bProjectedTextureVersion = 1;
#endif

	return pEnt;
}

C_EnvProjectedTexture::C_EnvProjectedTexture( void )
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	m_bForceUpdate = true;
#ifndef MAPBASE
	AddToEntityList( ENTITY_LIST_SIMULATE );
#endif
}

C_EnvProjectedTexture::~C_EnvProjectedTexture( void )
{
	Assert(m_pVolmetricMesh == NULL);
	ShutDownLightHandle();
}

void C_EnvProjectedTexture::ShutDownLightHandle( void )
{
	// Clear out the light
	if( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LightHandle );
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}


void C_EnvProjectedTexture::SetLightColor( byte r, byte g, byte b, byte a )
{
	m_LightColor.r = r;
	m_LightColor.g = g;
	m_LightColor.b = b;
	m_LightColor.a = a;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_EnvProjectedTexture::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_SpotlightTexture.Init( m_SpotlightTextureName, TEXTURE_GROUP_OTHER, true );
	}
#ifdef MAPBASE
	else //if ( updateType == DATA_UPDATE_DATATABLE_CHANGED )
	{
		// It could've been changed via input
		if( !FStrEq(m_SpotlightTexture->GetName(), m_SpotlightTextureName) )
		{
			m_SpotlightTexture.Init( m_SpotlightTextureName, TEXTURE_GROUP_OTHER, true );
		}
	}
#endif

	m_bForceUpdate = true;
	UpdateLight();
	BaseClass::OnDataChanged( updateType );

	if (m_flLastFOV != m_flLightFOV)
	{
		m_flLastFOV = m_flLightFOV;
		ClearVolumetricsMesh();
	}
}

static ConVar asw_perf_wtf("asw_perf_wtf", "0", FCVAR_DEVELOPMENTONLY, "Disable updating of projected shadow textures from UpdateLight" );
void C_EnvProjectedTexture::UpdateLight( void )
{
	VPROF("C_EnvProjectedTexture::UpdateLight");
	bool bVisible = true;

	Vector vLinearFloatLightColor( m_LightColor.r, m_LightColor.g, m_LightColor.b );
	float flLinearFloatLightAlpha = m_LightColor.a;

	if ( m_bAlwaysUpdate )
	{
		m_bForceUpdate = true;
	}

#ifdef MAPBASE
	if ( m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha || m_flCurrentBrightnessScale != m_flBrightnessScale )
	{
		if (m_flColorTransitionTime != 0.0f)
		{
			float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

			m_CurrentLinearFloatLightColor.x = Approach( vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed );
			m_CurrentLinearFloatLightColor.y = Approach( vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed );
			m_CurrentLinearFloatLightColor.z = Approach( vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed );
			m_flCurrentLinearFloatLightAlpha = Approach( flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed );
			m_flCurrentBrightnessScale = Approach( m_flBrightnessScale, m_flCurrentBrightnessScale, flColorTransitionSpeed );
		}
		else
		{
			// Just do it instantly
			m_CurrentLinearFloatLightColor.x = vLinearFloatLightColor.x;
			m_CurrentLinearFloatLightColor.y = vLinearFloatLightColor.y;
			m_CurrentLinearFloatLightColor.z = vLinearFloatLightColor.z;
			m_flCurrentLinearFloatLightAlpha = flLinearFloatLightAlpha;
			m_flCurrentBrightnessScale = m_flBrightnessScale;
		}

		m_bForceUpdate = true;
	}
#else
	if ( m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha )
	{
		float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

		m_CurrentLinearFloatLightColor.x = Approach( vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed );
		m_CurrentLinearFloatLightColor.y = Approach( vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed );
		m_CurrentLinearFloatLightColor.z = Approach( vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed );
		m_flCurrentLinearFloatLightAlpha = Approach( flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed );

		m_bForceUpdate = true;
	}
#endif
	
	if ( !m_bForceUpdate )
	{
		bVisible = IsBBoxVisible();		
	}

	if ( m_bState == false || !bVisible )
	{
		// Spotlight's extents aren't in view
		ShutDownLightHandle();
		ClearVolumetricsMesh();

		return;
	}

	if ( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE || m_hTargetEntity != NULL || m_bForceUpdate )
	{
		Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
		FlashlightState_t state;

#ifdef MAPBASE
		if ( m_hTargetEntity != NULL && !m_bDontFollowTarget )
#else
		if ( m_hTargetEntity != NULL )
#endif
		{
			if ( m_bCameraSpace )
			{
				const QAngle &angles = GetLocalAngles();

				C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
				if( pPlayer )
				{
					const QAngle playerAngles = pPlayer->GetAbsAngles();

					Vector vPlayerForward, vPlayerRight, vPlayerUp;
					AngleVectors( playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp );

					matrix3x4_t	mRotMatrix;
					AngleMatrix( angles, mRotMatrix );

					VectorITransform( vPlayerForward, mRotMatrix, vForward );
					VectorITransform( vPlayerRight, mRotMatrix, vRight );
					VectorITransform( vPlayerUp, mRotMatrix, vUp );

					float dist = (m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin()).Length();
					vPos = m_hTargetEntity->GetAbsOrigin() - vForward*dist;

					VectorNormalize( vForward );
					VectorNormalize( vRight );
					VectorNormalize( vUp );
				}
			}
			else
			{
				vForward = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
				VectorNormalize( vForward );

				// JasonM - unimplemented
				Assert (0);

				//Quaternion q = DirectionToOrientation( dir );


				//
				// JasonM - set up vRight, vUp
				//

				//			VectorNormalize( vRight );
				//			VectorNormalize( vUp );

				VectorVectors( vForward, vRight, vUp );
			}
		}
		else
		{
			AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );
		}

#ifdef MAPBASE
		float fHighFOV;
		if( m_flLightFOV > m_flLightHorFOV )
			fHighFOV = m_flLightFOV;
		else
			fHighFOV = m_flLightHorFOV;

		m_FlashlightState.m_fHorizontalFOVDegrees = m_flLightHorFOV;
#else
		m_FlashlightState.m_fHorizontalFOVDegrees = m_flLightFOV;
#endif
		m_FlashlightState.m_fVerticalFOVDegrees = m_flLightFOV;

		m_FlashlightState.m_vecLightOrigin = vPos;
		BasisToQuaternion(vForward, vRight, vUp, m_FlashlightState.m_quatOrientation);
		m_FlashlightState.m_NearZ = m_flNearZ;
		m_FlashlightState.m_FarZ = m_flFarZ;


		// quickly check the proposed light's bbox against the view frustum to determine whether we
		// should bother to create it, if it doesn't exist, or cull it, if it does.
		// get the half-widths of the near and far planes, 
		// based on the FOV which is in degrees. Remember that
		// on planet Valve, x is forward, y left, and z up. 
#ifdef MAPBASE
		const float tanHalfAngle = tan( fHighFOV * ( M_PI/180.0f ) * 0.5f );
#else
		const float tanHalfAngle = tan( m_flLightFOV * ( M_PI/180.0f ) * 0.5f );
#endif
		const float halfWidthNear = tanHalfAngle * m_flNearZ;
		const float halfWidthFar = tanHalfAngle * m_flFarZ;
		// now we can build coordinates in local space: the near rectangle is eg 
		// (0, -halfWidthNear, -halfWidthNear), (0,  halfWidthNear, -halfWidthNear), 
		// (0,  halfWidthNear,  halfWidthNear), (0, -halfWidthNear,  halfWidthNear)

		VectorAligned vNearRect[4] = { 
			VectorAligned( m_flNearZ, -halfWidthNear, -halfWidthNear), VectorAligned( m_flNearZ,  halfWidthNear, -halfWidthNear),
			VectorAligned( m_flNearZ,  halfWidthNear,  halfWidthNear), VectorAligned( m_flNearZ, -halfWidthNear,  halfWidthNear) 
		};

		VectorAligned vFarRect[4] = { 
			VectorAligned( m_flFarZ, -halfWidthFar, -halfWidthFar), VectorAligned( m_flFarZ,  halfWidthFar, -halfWidthFar),
			VectorAligned( m_flFarZ,  halfWidthFar,  halfWidthFar), VectorAligned( m_flFarZ, -halfWidthFar,  halfWidthFar) 
		};

		matrix3x4_t matOrientation( vForward, -vRight, vUp, vPos );

		enum
		{
			kNEAR = 0,
			kFAR = 1,
		};
		VectorAligned vOutRects[2][4];

		for ( int i = 0 ; i < 4 ; ++i )
		{
			VectorTransform( vNearRect[i].Base(), matOrientation, vOutRects[0][i].Base() );
		}
		for ( int i = 0 ; i < 4 ; ++i )
		{
			VectorTransform( vFarRect[i].Base(), matOrientation, vOutRects[1][i].Base() );
		}

		// now take the min and max extents for the bbox, and see if it is visible.
		Vector mins = **vOutRects; 
		Vector maxs = **vOutRects; 
		for ( int i = 1; i < 8 ; ++i )
		{
			VectorMin( mins, *(*vOutRects+i), mins );
			VectorMax( maxs, *(*vOutRects+i), maxs );
		}

#if 0 //for debugging the visibility frustum we just calculated
		NDebugOverlay::Triangle( vOutRects[0][0], vOutRects[0][1], vOutRects[0][2], 255, 0, 0, 100, true, 0.0f ); //first tri
		NDebugOverlay::Triangle( vOutRects[0][2], vOutRects[0][1], vOutRects[0][0], 255, 0, 0, 100, true, 0.0f ); //make it double sided
		NDebugOverlay::Triangle( vOutRects[0][2], vOutRects[0][3], vOutRects[0][0], 255, 0, 0, 100, true, 0.0f ); //second tri
		NDebugOverlay::Triangle( vOutRects[0][0], vOutRects[0][3], vOutRects[0][2], 255, 0, 0, 100, true, 0.0f ); //make it double sided

		NDebugOverlay::Triangle( vOutRects[1][0], vOutRects[1][1], vOutRects[1][2], 0, 0, 255, 100, true, 0.0f ); //first tri
		NDebugOverlay::Triangle( vOutRects[1][2], vOutRects[1][1], vOutRects[1][0], 0, 0, 255, 100, true, 0.0f ); //make it double sided
		NDebugOverlay::Triangle( vOutRects[1][2], vOutRects[1][3], vOutRects[1][0], 0, 0, 255, 100, true, 0.0f ); //second tri
		NDebugOverlay::Triangle( vOutRects[1][0], vOutRects[1][3], vOutRects[1][2], 0, 0, 255, 100, true, 0.0f ); //make it double sided

		NDebugOverlay::Box( vec3_origin, mins, maxs, 0, 255, 0, 100, 0.0f );
#endif
		
		bool bVisible = IsBBoxVisible( mins, maxs );
		if (!bVisible)
		{
			// Spotlight's extents aren't in view
			if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				ShutDownLightHandle();
			}

			return;
		}

		float flAlpha = m_flCurrentLinearFloatLightAlpha * ( 1.0f / 255.0f );

#ifdef MAPBASE
		m_FlashlightState.m_fConstantAtten = m_flConstantAtten;
		m_FlashlightState.m_fLinearAtten = m_flLinearAtten;
		m_FlashlightState.m_fQuadraticAtten = m_flQuadraticAtten;
		m_FlashlightState.m_FarZAtten = m_flFarZ;
		m_FlashlightState.m_Color[0] = (m_CurrentLinearFloatLightColor.x * (1.0f / 255.0f) * flAlpha) * m_flCurrentBrightnessScale;
		m_FlashlightState.m_Color[1] = (m_CurrentLinearFloatLightColor.y * (1.0f / 255.0f) * flAlpha) * m_flCurrentBrightnessScale;
		m_FlashlightState.m_Color[2] = (m_CurrentLinearFloatLightColor.z * (1.0f / 255.0f) * flAlpha) * m_flCurrentBrightnessScale;
		m_FlashlightState.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
		m_FlashlightState.m_flShadowSlopeScaleDepthBias = mat_slopescaledepthbias_shadowmap.GetFloat();
		m_FlashlightState.m_flShadowDepthBias = mat_depthbias_shadowmap.GetFloat();
		m_FlashlightState.m_flShadowAtten = m_flShadowAtten;
#else
		m_FlashlightState.m_fQuadraticAtten = 0.0;
		m_FlashlightState.m_fLinearAtten = 100;
		m_FlashlightState.m_fConstantAtten = 0.0f;
		m_FlashlightState.m_FarZAtten = m_flFarZ;
		m_FlashlightState.m_fBrightnessScale = m_flBrightnessScale;
		m_FlashlightState.m_Color[0] = m_CurrentLinearFloatLightColor.x * (1.0f / 255.0f) * flAlpha;
		m_FlashlightState.m_Color[1] = m_CurrentLinearFloatLightColor.y * (1.0f / 255.0f) * flAlpha;
		m_FlashlightState.m_Color[2] = m_CurrentLinearFloatLightColor.z * (1.0f / 255.0f) * flAlpha;
		m_FlashlightState.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
		m_FlashlightState.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();
		m_FlashlightState.m_flShadowDepthBias = g_pMaterialSystemHardwareConfig->GetShadowDepthBias();
#endif
		m_FlashlightState.m_bEnableShadows = m_bEnableShadows;
		m_FlashlightState.m_pSpotlightTexture = m_SpotlightTexture;
		m_FlashlightState.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;

		m_FlashlightState.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality
		state.m_bEnableShadows = m_bEnableShadows;
		state.m_pSpotlightTexture = m_SpotlightTexture;
		state.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;

		state.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality

#ifdef MAPBASE
		m_FlashlightState.m_bAlwaysDraw = m_bAlwaysDraw;
#endif

		if( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE )
		{
			m_LightHandle = g_pClientShadowMgr->CreateFlashlight(m_FlashlightState);

			if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				m_bForceUpdate = false;
			}
		}
		else
		{
			g_pClientShadowMgr->UpdateFlashlightState(m_LightHandle, m_FlashlightState);
			m_bForceUpdate = false;
		}

		g_pClientShadowMgr->GetFrustumExtents( m_LightHandle, m_vecExtentsMin, m_vecExtentsMax );

		m_vecExtentsMin = m_vecExtentsMin - GetAbsOrigin();
		m_vecExtentsMax = m_vecExtentsMax - GetAbsOrigin();
	}

	if( m_bLightOnlyTarget )
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, m_hTargetEntity );
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, NULL );
	}

	g_pClientShadowMgr->SetFlashlightLightWorld( m_LightHandle, m_bLightWorld );

	if ( !asw_perf_wtf.GetBool() && !m_bForceUpdate )
	{
		g_pClientShadowMgr->UpdateProjectedTexture( m_LightHandle, true );
	}
	if (m_bEnableVolumetrics && m_bEnableShadows)
	{
		CViewSetup setup;
		GetShadowViewSetup(setup);

		VMatrix world2View, view2Proj, world2Proj, world2Pixels;
		render->GetMatricesForView(setup, &world2View, &view2Proj, &world2Proj, &world2Pixels);
		VMatrix proj2world;
		MatrixInverseGeneral(world2Proj, proj2world);

		Vector fwd, right, up;
		AngleVectors(setup.angles, &fwd, &right, &up);

		Vector nearFarPlane[8];
		Vector3DMultiplyPositionProjective(proj2world, Vector(-1, -1, 1), nearFarPlane[0]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(1, -1, 1), nearFarPlane[1]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(1, 1, 1), nearFarPlane[2]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(-1, 1, 1), nearFarPlane[3]);

		Vector3DMultiplyPositionProjective(proj2world, Vector(-1, -1, 0), nearFarPlane[4]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(1, -1, 0), nearFarPlane[5]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(1, 1, 0), nearFarPlane[6]);
		Vector3DMultiplyPositionProjective(proj2world, Vector(-1, 1, 0), nearFarPlane[7]);

		m_vecRenderBoundsMin.Init(MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT);
		m_vecRenderBoundsMax.Init(MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT);

		for (int i = 0; i < 8; i++)
		{
			m_vecRenderBoundsMin.x = Min(m_vecRenderBoundsMin.x, nearFarPlane[i].x);
			m_vecRenderBoundsMin.y = Min(m_vecRenderBoundsMin.y, nearFarPlane[i].y);
			m_vecRenderBoundsMin.z = Min(m_vecRenderBoundsMin.y, nearFarPlane[i].z);
			m_vecRenderBoundsMax.x = Max(m_vecRenderBoundsMax.x, nearFarPlane[i].x);
			m_vecRenderBoundsMax.y = Max(m_vecRenderBoundsMax.y, nearFarPlane[i].y);
			m_vecRenderBoundsMax.z = Max(m_vecRenderBoundsMax.y, nearFarPlane[i].z);
		}
	}
}

void C_EnvProjectedTexture::Simulate( void )
{
	UpdateLight();

	BaseClass::Simulate();
}

int C_EnvProjectedTexture::DrawModel(int flags)
{
	if (!m_bState ||
		m_LightHandle == CLIENTSHADOW_INVALID_HANDLE ||
		!m_bEnableVolumetrics ||
		!gstring_volumetrics_enabled.GetBool() ||
		CurrentViewID() != VIEW_MAIN)
	{
		return 0;
	}

	float flDistanceFade = 1.0f;
	if (m_flVolumetricsFadeDistance > 0.0f)
	{
		Vector delta = CurrentViewOrigin() - GetLocalOrigin();
		flDistanceFade = RemapValClamped(delta.Length(),
			m_flVolumetricsFadeDistance + gstring_volumetrics_fade_range.GetFloat(),
			m_flVolumetricsFadeDistance, 0.0f, 1.0f);
	}

	if (flDistanceFade <= 0.0f)
	{
		return 0;
	}

	ITexture *pDepthTexture = NULL;
	const ShadowHandle_t shadowHandle = g_pClientShadowMgr->GetShadowHandle(m_LightHandle);
	const int iNumActiveDepthTextures = g_pClientShadowMgr->GetNumShadowDepthtextures();
	for (int i = 0; i < iNumActiveDepthTextures; i++)
	{
		if (g_pClientShadowMgr->GetShadowDepthHandle(i) == shadowHandle)
		{
			pDepthTexture = g_pClientShadowMgr->GetShadowDepthTex(i);
			break;
		}
	}

	if (pDepthTexture == NULL)
	{
		return 0;
	}

	if (m_pVolmetricMesh == NULL)
	{
		RebuildVolumetricMesh();
	}

	UpdateScreenEffectTexture();

	CViewSetup setup;
	GetShadowViewSetup(setup);

	VMatrix world2View, view2Proj, world2Proj, world2Pixels;
	render->GetMatricesForView(setup, &world2View, &view2Proj, &world2Proj, &world2Pixels);

	VMatrix tmp, shadowToUnit;
	MatrixBuildScale(tmp, 1.0f / 2, 1.0f / -2, 1.0f);
	tmp[0][3] = tmp[1][3] = 0.5f;
	MatrixMultiply(tmp, world2Proj, shadowToUnit);

	m_FlashlightState.m_Color[3] = m_flVolumetricsMultiplier / m_iCurrentVolumetricsSubDiv * flDistanceFade;

	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->SetFlashlightMode(true);
	pRenderContext->SetFlashlightStateEx(m_FlashlightState, shadowToUnit, pDepthTexture);

	matrix3x4_t flashlightTransform;
	AngleMatrix(setup.angles, setup.origin, flashlightTransform);

	pRenderContext->MatrixMode(MATERIAL_MODEL);
	pRenderContext->PushMatrix();
	pRenderContext->LoadMatrix(flashlightTransform);

	pRenderContext->Bind(m_matVolumetricsMaterial);
	m_pVolmetricMesh->Draw();

	pRenderContext->MatrixMode(MATERIAL_MODEL);
	pRenderContext->PopMatrix();
	pRenderContext->SetFlashlightMode(false);
	return 1;
}

void C_EnvProjectedTexture::ClearVolumetricsMesh()
{
	if (m_pVolmetricMesh != NULL)
	{
		CMatRenderContextPtr pRenderContext(materials);
		pRenderContext->DestroyStaticMesh(m_pVolmetricMesh);
		m_pVolmetricMesh = NULL;
	}
}

void C_EnvProjectedTexture::RebuildVolumetricMesh()
{
	if (!m_matVolumetricsMaterial.IsValid())
	{
		m_matVolumetricsMaterial.Init(materials->FindMaterial("engine/light_volumetrics", TEXTURE_GROUP_OTHER));
	}

	ClearVolumetricsMesh();

	CViewSetup setup;
	GetShadowViewSetup(setup);
	setup.origin = vec3_origin;
	setup.angles = vec3_angle;

	VMatrix world2View, view2Proj, world2Proj, world2Pixels;
	render->GetMatricesForView(setup, &world2View, &view2Proj, &world2Proj, &world2Pixels);
	VMatrix proj2world;
	MatrixInverseGeneral(world2Proj, proj2world);

	const Vector origin(vec3_origin);
	QAngle angles(vec3_angle);
	Vector fwd, right, up;
	AngleVectors(angles, &fwd, &right, &up);

	Vector nearPlane[4];
	Vector farPlane[4];
	Vector3DMultiplyPositionProjective(proj2world, Vector(-1, -1, 1), farPlane[0]);
	Vector3DMultiplyPositionProjective(proj2world, Vector(1, -1, 1), farPlane[1]);
	Vector3DMultiplyPositionProjective(proj2world, Vector(1, 1, 1), farPlane[2]);
	Vector3DMultiplyPositionProjective(proj2world, Vector(-1, 1, 1), farPlane[3]);

	Vector3DMultiplyPositionProjective(proj2world, Vector(-1, -1, 0), nearPlane[0]);
	Vector3DMultiplyPositionProjective(proj2world, Vector(1, -1, 0), nearPlane[1]);
	Vector3DMultiplyPositionProjective(proj2world, Vector(1, 1, 0), nearPlane[2]);
	Vector3DMultiplyPositionProjective(proj2world, Vector(-1, 1, 0), nearPlane[3]);

	//DebugDrawLine( farPlane[ 0 ], farPlane[ 1 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( farPlane[ 1 ], farPlane[ 2 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( farPlane[ 2 ], farPlane[ 3 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( farPlane[ 3 ], farPlane[ 0 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( nearPlane[ 0 ], nearPlane[ 1 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( nearPlane[ 1 ], nearPlane[ 2 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( nearPlane[ 2 ], nearPlane[ 3 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( nearPlane[ 3 ], nearPlane[ 0 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( farPlane[ 0 ], nearPlane[ 0 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( farPlane[ 1 ], nearPlane[ 1 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( farPlane[ 2 ], nearPlane[ 2 ], 0, 0, 255, true, -1 );
	//DebugDrawLine( farPlane[ 3 ], nearPlane[ 3 ], 0, 0, 255, true, -1 );

	const Vector vecDirections[3] = {
		(farPlane[0] - vec3_origin),
		(farPlane[1] - farPlane[0]),
		(farPlane[3] - farPlane[0]),
	};

	const VertexFormat_t vertexFormat = VERTEX_POSITION | VERTEX_TEXCOORD_SIZE(0, 2);
	CMatRenderContextPtr pRenderContext(materials);
	m_pVolmetricMesh = pRenderContext->CreateStaticMesh(vertexFormat, TEXTURE_GROUP_OTHER, m_matVolumetricsMaterial);

	const int iCvarSubDiv = gstring_volumetrics_subdiv.GetInt();
	m_iCurrentVolumetricsSubDiv = (iCvarSubDiv > 2) ? iCvarSubDiv : Max(m_iVolumetricsQuality, 3);

	CMeshBuilder meshBuilder;
	meshBuilder.Begin(m_pVolmetricMesh, MATERIAL_TRIANGLES, m_iCurrentVolumetricsSubDiv * 6 - 4);

	for (int x = 1; x < m_iCurrentVolumetricsSubDiv * 2; x++)
	{
		// never 0.0 or 1.0
		float flFracX = x / float(m_iCurrentVolumetricsSubDiv * 2);
		flFracX = powf(flFracX, m_flVolumetricsQualityBias);

		Vector v00 = origin + vecDirections[0] * flFracX;
		Vector v10 = v00 + vecDirections[1] * flFracX;
		Vector v11 = v10 + vecDirections[2] * flFracX;
		Vector v01 = v00 + vecDirections[2] * flFracX;

		meshBuilder.Position3f(XYZ(v00));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(XYZ(v10));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(XYZ(v11));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(XYZ(v00));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(XYZ(v11));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(XYZ(v01));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();
	}

	for (int x = 1; x < m_iCurrentVolumetricsSubDiv; x++)
	{
		// never 0.0 or 1.0
		const float flFracX = x / float(m_iCurrentVolumetricsSubDiv);
		Vector v0 = origin + vecDirections[0] + vecDirections[1] * flFracX;
		Vector v1 = v0 + vecDirections[2];

		meshBuilder.Position3f(XYZ(origin));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(XYZ(v0));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(XYZ(v1));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();
	}

	for (int x = 1; x < m_iCurrentVolumetricsSubDiv; x++)
	{
		// never 0.0 or 1.0
		const float flFracX = x / float(m_iCurrentVolumetricsSubDiv);
		Vector v0 = origin + vecDirections[0] + vecDirections[2] * flFracX;
		Vector v1 = v0 + vecDirections[1];

		meshBuilder.Position3f(XYZ(origin));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(XYZ(v0));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f(XYZ(v1));
		meshBuilder.TexCoord2f(0, 0.0f, 0.0f);
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
}

void C_EnvProjectedTexture::GetShadowViewSetup(CViewSetup &setup)
{
	setup.origin = m_FlashlightState.m_vecLightOrigin;
	QuaternionAngles(m_FlashlightState.m_quatOrientation, setup.angles);
	setup.fov = m_flLightFOV;
	setup.zFar = m_flFarZ;
	setup.zNear = m_flNearZ;
	setup.m_bOrtho = false;
	setup.m_flAspectRatio = 1.0f;
	setup.x = setup.y = 0;
	setup.width = m_FlashlightState.m_pSpotlightTexture ? m_FlashlightState.m_pSpotlightTexture->GetActualWidth() : 512;
	setup.height = m_FlashlightState.m_pSpotlightTexture ? m_FlashlightState.m_pSpotlightTexture->GetActualHeight() : 512;
}

bool C_EnvProjectedTexture::IsBBoxVisible( Vector vecExtentsMin, Vector vecExtentsMax )
{
#ifdef MAPBASE
	if (m_bAlwaysDraw)
		return true;
#endif

	// Z position clamped to the min height (but must be less than the max)
	float flVisibleBBoxMinHeight = MIN( vecExtentsMax.z - 1.0f, m_flVisibleBBoxMinHeight );
	vecExtentsMin.z = MAX( vecExtentsMin.z, flVisibleBBoxMinHeight );

	// Check if the bbox is in the view
	return !engine->CullBox( vecExtentsMin, vecExtentsMax );
}

#else

#ifndef MAPBASE
static ConVar mat_slopescaledepthbias_shadowmap( "mat_slopescaledepthbias_shadowmap", "16", FCVAR_CHEAT );
static ConVar mat_depthbias_shadowmap(	"mat_depthbias_shadowmap", "0.0005", FCVAR_CHEAT  );
#else
static ConVar mat_slopescaledepthbias_shadowmap( "mat_slopescaledepthbias_shadowmap", "4", FCVAR_CHEAT );
static ConVar mat_depthbias_shadowmap(	"mat_depthbias_shadowmap", "0.00001", FCVAR_CHEAT  );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_EnvProjectedTexture : public C_BaseEntity
{
	DECLARE_CLASS( C_EnvProjectedTexture, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged( DataUpdateType_t updateType );
	void	ShutDownLightHandle( void );

	virtual void Simulate();

#ifdef MAPBASE
	void	UpdateLight();
#else
	void	UpdateLight( bool bForceUpdate );
#endif

	C_EnvProjectedTexture();
	~C_EnvProjectedTexture();

private:

	ClientShadowHandle_t m_LightHandle;
#ifdef MAPBASE
	bool m_bForceUpdate;
#endif

	EHANDLE	m_hTargetEntity;

	bool	m_bState;
#ifdef MAPBASE
	bool	m_bAlwaysUpdate;
#endif
	float	m_flLightFOV;
	bool	m_bEnableShadows;
	bool	m_bLightOnlyTarget;
	bool	m_bLightWorld;
	bool	m_bCameraSpace;
	Vector	m_LinearFloatLightColor;
	float	m_flAmbient;
	float	m_flNearZ;
	float	m_flFarZ;
	char	m_SpotlightTextureName[ MAX_PATH ];
	int		m_nSpotlightTextureFrame;
	int		m_nShadowQuality;
};

IMPLEMENT_CLIENTCLASS_DT( C_EnvProjectedTexture, DT_EnvProjectedTexture, CEnvProjectedTexture )
	RecvPropEHandle( RECVINFO( m_hTargetEntity )	),
	RecvPropBool(	 RECVINFO( m_bState )			),
#ifdef MAPBASE
	RecvPropBool(	 RECVINFO( m_bAlwaysUpdate )	),
#endif
	RecvPropFloat(	 RECVINFO( m_flLightFOV )		),
	RecvPropBool(	 RECVINFO( m_bEnableShadows )	),
	RecvPropBool(	 RECVINFO( m_bLightOnlyTarget ) ),
	RecvPropBool(	 RECVINFO( m_bLightWorld )		),
	RecvPropBool(	 RECVINFO( m_bCameraSpace )		),
	RecvPropVector(	 RECVINFO( m_LinearFloatLightColor )		),
	RecvPropFloat(	 RECVINFO( m_flAmbient )		),
	RecvPropString(  RECVINFO( m_SpotlightTextureName ) ),
	RecvPropInt(	 RECVINFO( m_nSpotlightTextureFrame ) ),
	RecvPropFloat(	 RECVINFO( m_flNearZ )	),
	RecvPropFloat(	 RECVINFO( m_flFarZ )	),
	RecvPropInt(	 RECVINFO( m_nShadowQuality )	),
END_RECV_TABLE()

C_EnvProjectedTexture::C_EnvProjectedTexture( void )
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
}

C_EnvProjectedTexture::~C_EnvProjectedTexture( void )
{
	ShutDownLightHandle();
}

void C_EnvProjectedTexture::ShutDownLightHandle( void )
{
	// Clear out the light
	if( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LightHandle );
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_EnvProjectedTexture::OnDataChanged( DataUpdateType_t updateType )
{
#ifdef MAPBASE
	m_bForceUpdate = true;
	UpdateLight();
#else
	UpdateLight( true );
#endif
	BaseClass::OnDataChanged( updateType );
}

#ifndef MAPBASE
void C_EnvProjectedTexture::UpdateLight( bool bForceUpdate )
#else
void C_EnvProjectedTexture::UpdateLight()
#endif
{
#ifndef MAPBASE
	if ( m_bState == false )
	{
		if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
		{
			ShutDownLightHandle();
		}

		return;
	}
#else
	if ( m_bAlwaysUpdate )
	{
		m_bForceUpdate = true;
	}

	if ( m_bState == false )
	{
		// Spotlight's extents aren't in view
		ShutDownLightHandle();

		return;
	}
#endif

#ifdef MAPBASE
	if ( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE || m_hTargetEntity != NULL || m_bForceUpdate )
	{
#endif
	Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
	FlashlightState_t state;

	if ( m_hTargetEntity != NULL )
	{
		if ( m_bCameraSpace )
		{
			const QAngle &angles = GetLocalAngles();

			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			if( pPlayer )
			{
				const QAngle playerAngles = pPlayer->GetAbsAngles();
				
				Vector vPlayerForward, vPlayerRight, vPlayerUp;
				AngleVectors( playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp );

            	matrix3x4_t	mRotMatrix;
				AngleMatrix( angles, mRotMatrix );

				VectorITransform( vPlayerForward, mRotMatrix, vForward );
				VectorITransform( vPlayerRight, mRotMatrix, vRight );
				VectorITransform( vPlayerUp, mRotMatrix, vUp );

				float dist = (m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin()).Length();
				vPos = m_hTargetEntity->GetAbsOrigin() - vForward*dist;

				VectorNormalize( vForward );
				VectorNormalize( vRight );
				VectorNormalize( vUp );
			}
		}
		else
		{
#ifndef MAPBASE
			vForward = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize( vForward );
#else
			// VXP: Fixing targeting
			Vector vecToTarget;
			QAngle vecAngles;
			if (m_hTargetEntity == NULL)
			{
				vecAngles = GetAbsAngles();
			}
			else
			{
				vecToTarget = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
				VectorAngles(vecToTarget, vecAngles);
			}
			AngleVectors(vecAngles, &vForward, &vRight, &vUp);
#endif

			// JasonM - unimplemented
			Assert (0);

			//Quaternion q = DirectionToOrientation( dir );


			//
			// JasonM - set up vRight, vUp
			//

//			VectorNormalize( vRight );
//			VectorNormalize( vUp );
		}
	}
	else
	{
		AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );
	}

	state.m_fHorizontalFOVDegrees = m_flLightFOV;
	state.m_fVerticalFOVDegrees = m_flLightFOV;

	state.m_vecLightOrigin = vPos;
	BasisToQuaternion( vForward, vRight, vUp, state.m_quatOrientation );

	state.m_fQuadraticAtten = 0.0;
	state.m_fLinearAtten = 100;
	state.m_fConstantAtten = 0.0f;
	state.m_Color[0] = m_LinearFloatLightColor.x;
	state.m_Color[1] = m_LinearFloatLightColor.y;
	state.m_Color[2] = m_LinearFloatLightColor.z;
	state.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
	state.m_NearZ = m_flNearZ;
	state.m_FarZ = m_flFarZ;
	state.m_flShadowSlopeScaleDepthBias = mat_slopescaledepthbias_shadowmap.GetFloat();
	state.m_flShadowDepthBias = mat_depthbias_shadowmap.GetFloat();
	state.m_bEnableShadows = m_bEnableShadows;
	state.m_pSpotlightTexture = materials->FindTexture( m_SpotlightTextureName, TEXTURE_GROUP_OTHER, false );
	state.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;

	state.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality

	if( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE )
	{
		m_LightHandle = g_pClientShadowMgr->CreateFlashlight( state );
	}
	else
	{
#ifndef MAPBASE
		if ( m_hTargetEntity != NULL || bForceUpdate == true )
		{
			g_pClientShadowMgr->UpdateFlashlightState( m_LightHandle, state );
		}
#else
		g_pClientShadowMgr->UpdateFlashlightState( m_LightHandle, state );
		m_bForceUpdate = false;
#endif
	}
#ifdef MAPBASE
	}
#endif

	if( m_bLightOnlyTarget )
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, m_hTargetEntity );
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, NULL );
	}

	g_pClientShadowMgr->SetFlashlightLightWorld( m_LightHandle, m_bLightWorld );

#ifndef MAPBASE
	if ( bForceUpdate == false )
	{
		g_pClientShadowMgr->UpdateProjectedTexture( m_LightHandle, true );
	}
#else
	if ( !m_bForceUpdate )
	{
		g_pClientShadowMgr->UpdateProjectedTexture( m_LightHandle, true );
	}
#endif
}

void C_EnvProjectedTexture::Simulate( void )
{
#ifndef MAPBASE
	UpdateLight( false );
#else
	UpdateLight();
#endif

	BaseClass::Simulate();
}

#endif

