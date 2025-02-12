//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "KeyValues.h"
#include "cdll_client_int.h"
#include "view_scene.h"
#include "viewrender.h"
#include "tier0/icommandline.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterialvar.h"
#include "tier0/vprof.h"
#include "keyvalues.h"	
#include "convar.h"

#include "ScreenSpaceEffects.h"

#include "c_basehlplayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------------------
// CScreenSpaceEffectRegistration code
// Used to register and effect with the IScreenSpaceEffectManager
//------------------------------------------------------------------------------
CScreenSpaceEffectRegistration *CScreenSpaceEffectRegistration::s_pHead = NULL;

CScreenSpaceEffectRegistration::CScreenSpaceEffectRegistration( const char *pName, IScreenSpaceEffect *pEffect )
{
	m_pEffectName = pName;
	m_pEffect = pEffect;
	m_pNext = s_pHead;
	s_pHead = this;
}


//------------------------------------------------------------------------------
// CScreenSpaceEffectManager - Implementation of IScreenSpaceEffectManager
//------------------------------------------------------------------------------
class CScreenSpaceEffectManager : public IScreenSpaceEffectManager
{
public:

	virtual void InitScreenSpaceEffects( );
	virtual void ShutdownScreenSpaceEffects( );

	virtual IScreenSpaceEffect *GetScreenSpaceEffect( const char *pEffectName );

	virtual void SetScreenSpaceEffectParams( const char *pEffectName, KeyValues *params );
	virtual void SetScreenSpaceEffectParams( IScreenSpaceEffect *pEffect, KeyValues *params );
    
	virtual void EnableScreenSpaceEffect( const char *pEffectName );
	virtual void EnableScreenSpaceEffect( IScreenSpaceEffect *pEffect );

	virtual void DisableScreenSpaceEffect( const char *pEffectName );
	virtual void DisableScreenSpaceEffect( IScreenSpaceEffect *pEffect );

	virtual void DisableAllScreenSpaceEffects( );

	virtual void RenderEffects( int x, int y, int w, int h );
};

CScreenSpaceEffectManager g_ScreenSpaceEffectManager;
IScreenSpaceEffectManager *g_pScreenSpaceEffects = &g_ScreenSpaceEffectManager;

//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::InitScreenSpaceEffects - Initialise all registered effects
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::InitScreenSpaceEffects( )
{
	if ( CommandLine()->FindParm( "-filmgrain" ) )
	{
		GetScreenSpaceEffect( "filmgrain" )->Enable( true );
	}

	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		IScreenSpaceEffect *pEffect = pReg->m_pEffect;
		if( pEffect )
		{
			bool bIsEnabled = pEffect->IsEnabled( );
			pEffect->Init( );
			pEffect->Enable( bIsEnabled );
		}
	}
}


//----------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::ShutdownScreenSpaceEffects - Shutdown all registered effects
//----------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::ShutdownScreenSpaceEffects( )
{
	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		IScreenSpaceEffect *pEffect = pReg->m_pEffect;
		if( pEffect )
		{
			pEffect->Shutdown( );
		}
	}
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::GetScreenSpaceEffect - Returns a point to the named effect
//---------------------------------------------------------------------------------------
IScreenSpaceEffect *CScreenSpaceEffectManager::GetScreenSpaceEffect( const char *pEffectName )
{
	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		if( !Q_stricmp( pReg->m_pEffectName, pEffectName ) )
		{
			IScreenSpaceEffect *pEffect = pReg->m_pEffect;
			return pEffect;
		}
	}

	Warning( "Could not find screen space effect %s\n", pEffectName );

	return NULL;
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::SetScreenSpaceEffectParams 
//	- Assign parameters to the specified effect
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::SetScreenSpaceEffectParams( const char *pEffectName, KeyValues *params )
{
	IScreenSpaceEffect *pEffect = GetScreenSpaceEffect( pEffectName );
	if( pEffect )
		SetScreenSpaceEffectParams( pEffect, params );
}

void CScreenSpaceEffectManager::SetScreenSpaceEffectParams( IScreenSpaceEffect *pEffect, KeyValues *params )
{
	if( pEffect )
		pEffect->SetParameters( params );
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::EnableScreenSpaceEffect
//	- Enables the specified effect
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::EnableScreenSpaceEffect( const char *pEffectName )
{
	IScreenSpaceEffect *pEffect = GetScreenSpaceEffect( pEffectName );
	if( pEffect )
		EnableScreenSpaceEffect( pEffect );
}

void CScreenSpaceEffectManager::EnableScreenSpaceEffect( IScreenSpaceEffect *pEffect )
{
	if( pEffect )
		pEffect->Enable( true );
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::DisableScreenSpaceEffect
//	- Disables the specified effect
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::DisableScreenSpaceEffect( const char *pEffectName )
{
	IScreenSpaceEffect *pEffect = GetScreenSpaceEffect( pEffectName );
	if( pEffect )
		DisableScreenSpaceEffect( pEffect );
}

void CScreenSpaceEffectManager::DisableScreenSpaceEffect( IScreenSpaceEffect *pEffect )
{
	if( pEffect )
		pEffect->Enable( false );
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::DisableAllScreenSpaceEffects
//	- Disables all registered screen space effects
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::DisableAllScreenSpaceEffects( )
{
	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		IScreenSpaceEffect *pEffect = pReg->m_pEffect;
		if( pEffect )
		{
			pEffect->Enable( false );
		}
	}
}


//---------------------------------------------------------------------------------------
// CScreenSpaceEffectManager::RenderEffects
//	- Renders all registered screen space effects
//---------------------------------------------------------------------------------------
void CScreenSpaceEffectManager::RenderEffects( int x, int y, int w, int h )
{
	for( CScreenSpaceEffectRegistration *pReg=CScreenSpaceEffectRegistration::s_pHead; pReg!=NULL; pReg=pReg->m_pNext )
	{
		IScreenSpaceEffect *pEffect = pReg->m_pEffect;
		if( pEffect )
		{
			pEffect->Render( x, y, w, h );
		}
	}
}

//------------------------------------------------------------------------------
// Example post-processing effect
//------------------------------------------------------------------------------
class CExampleEffect : public IScreenSpaceEffect
{
public:
	CExampleEffect( );
   ~CExampleEffect( );

	void Init( );
	void Shutdown( );

	void SetParameters( KeyValues *params );

	void Render( int x, int y, int w, int h );

	void Enable( bool bEnable );
	bool IsEnabled( );

private:

	bool				m_bEnable;

	CMaterialReference	m_Material;
};

ADD_SCREENSPACE_EFFECT( CExampleEffect, exampleeffect );

//------------------------------------------------------------------------------
// CExampleEffect constructor
//------------------------------------------------------------------------------
CExampleEffect::CExampleEffect( )
{
	m_bEnable = false;
}


//------------------------------------------------------------------------------
// CExampleEffect destructor
//------------------------------------------------------------------------------
CExampleEffect::~CExampleEffect( )
{
}


//------------------------------------------------------------------------------
// CExampleEffect init
//------------------------------------------------------------------------------
void CExampleEffect::Init( )
{
	// This is just example code, init your effect material here
	//m_Material.Init( "engine/exampleeffect", TEXTURE_GROUP_OTHER );

	m_bEnable = false;
}


//------------------------------------------------------------------------------
// CExampleEffect shutdown
//------------------------------------------------------------------------------
void CExampleEffect::Shutdown( )
{
	m_Material.Shutdown();
}

//------------------------------------------------------------------------------
// CExampleEffect enable
//------------------------------------------------------------------------------
void CExampleEffect::Enable( bool bEnable )
{
	// This is just example code, don't enable it
	// m_bEnable = bEnable;
}

bool CExampleEffect::IsEnabled( )
{
	return m_bEnable;
}

//------------------------------------------------------------------------------
// CExampleEffect SetParameters
//------------------------------------------------------------------------------
void CExampleEffect::SetParameters( KeyValues *params )
{
	if( params->GetDataType( "example_param" ) == KeyValues::TYPE_STRING )
	{
		// ...
	}
}

//------------------------------------------------------------------------------
// CExampleEffect render
//------------------------------------------------------------------------------
void CExampleEffect::Render( int x, int y, int w, int h )
{
	if ( !IsEnabled() )
		return;

	// Render Effect
	Rect_t actualRect;
	UpdateScreenEffectTexture( 0, x, y, w, h, false, &actualRect );
	ITexture *pTexture = GetFullFrameFrameBufferTexture( 0 );

	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->DrawScreenSpaceRectangle( m_Material, x, y, w, h,
											actualRect.x, actualRect.y, actualRect.x+actualRect.width-1, actualRect.y+actualRect.height-1, 
											pTexture->GetActualWidth(), pTexture->GetActualHeight() );
}

//------------------------------------------------------------------------------
// Health post-processing effects
//------------------------------------------------------------------------------
class CHealthEffects : public IScreenSpaceEffect
{
public:
	CHealthEffects() { };
	~CHealthEffects() { };

	void Init();
	void Shutdown();

	void SetParameters(KeyValues *params) { };

	void Render(int x, int y, int w, int h);

	void Enable(bool bEnable) { m_bEnable = bEnable; }
	bool IsEnabled() { return m_bEnable; }

private:

	bool				m_bEnable;

	float				fDispersionAmount;
	float				fDispersionLerpTo;

	int					iLastHealth;
	bool				bDisableDispersionOneFrame;

	CMaterialReference	m_ChromaticDisp;
};

ADD_SCREENSPACE_EFFECT(CHealthEffects, c17_healthfx);

ConVar r_post_chromatic_dispersion_offset("r_post_chromatic_dispersion_offset", "0.2", FCVAR_CHEAT, "Controls constant chromatic dispersion strength, 0 for off.");
ConVar r_post_chromatic_dispersion_offset_heavydamage("r_post_chromatic_dispersion_offset_heavydamage", "1.5", FCVAR_CHEAT, "Controls constant chromatic dispersion strength when the player takes heavy damage.");
ConVar r_post_chromatic_dispersion_offset_damage("r_post_chromatic_dispersion_offset_damage", "8.0", FCVAR_CHEAT, "Controls constant chromatic dispersion strength when the player takes damage.");

ConVar r_post_healtheffects_debug("r_post_healtheffects_debug", "0", FCVAR_CHEAT);

ConVar r_post_healtheffects("r_post_healtheffects", "1", FCVAR_ARCHIVE);

//------------------------------------------------------------------------------
// CHealthEffects init
//------------------------------------------------------------------------------
void CHealthEffects::Init()
{
	m_ChromaticDisp.Init(materials->FindMaterial("effects/shaders/chromaticDisp", TEXTURE_GROUP_PIXEL_SHADERS, true));

	fDispersionAmount = r_post_chromatic_dispersion_offset.GetFloat();
	fDispersionLerpTo = r_post_chromatic_dispersion_offset.GetFloat();

	iLastHealth = -1;
	bDisableDispersionOneFrame = false;
}


//------------------------------------------------------------------------------
// CHealthEffects shutdown
//------------------------------------------------------------------------------
void CHealthEffects::Shutdown()
{
	m_ChromaticDisp.Shutdown();
}

//------------------------------------------------------------------------------
// CHealthEffects render
//------------------------------------------------------------------------------
void CHealthEffects::Render(int x, int y, int w, int h)
{
	if (!r_post_healtheffects.GetBool() || (IsEnabled() == false))
		return;

	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	fDispersionLerpTo = r_post_chromatic_dispersion_offset.GetFloat();

	if (iLastHealth > pPlayer->GetHealth() /*&& pPlayer->GetHealth() > 15*/)
	{
		//Player took damage.
		if (iLastHealth - pPlayer->GetHealth() >= 20)
		{
			fDispersionAmount = r_post_chromatic_dispersion_offset_heavydamage.GetFloat();
		}
		else
		{
			fDispersionAmount = r_post_chromatic_dispersion_offset_damage.GetFloat();
		}
		bDisableDispersionOneFrame = true;
	}
	iLastHealth = pPlayer->GetHealth();

	if (fDispersionAmount != fDispersionLerpTo && !bDisableDispersionOneFrame)
		fDispersionAmount = FLerp(fDispersionAmount, fDispersionLerpTo, 0.1f);
	else if (bDisableDispersionOneFrame)
		bDisableDispersionOneFrame = false;

	IMaterialVar *var;

	if (fDispersionAmount >= 0.01f)
	{
		var = m_ChromaticDisp->FindVar("$FOCUSOFFSET", NULL);
		var->SetFloatValue(fDispersionAmount);
		var = m_ChromaticDisp->FindVar("$radial", NULL);
		var->SetIntValue(0);
		DrawScreenEffectMaterial(m_ChromaticDisp, x, y, w, h);
		if (r_post_healtheffects_debug.GetBool())
			DevMsg("Dispersion Amount: %.2f\n", fDispersionAmount);
	}
}

//------------------------------------------------------------------------------
// Vignetting post-processing effect
//------------------------------------------------------------------------------
class CVignettingEffect : public IScreenSpaceEffect
{
public:
	CVignettingEffect() { };
	~CVignettingEffect() { };

	void Init();
	void Shutdown();

	void SetParameters(KeyValues *params) { };

	void Render(int x, int y, int w, int h);

	void Enable(bool bEnable) { m_bEnable = bEnable; }
	bool IsEnabled() { return m_bEnable; }

private:

	bool				m_bEnable;

	float				fVignettingAmount;
	float				fVignettingLerpTo;

	CMaterialReference	m_VignetMat;
};

ADD_SCREENSPACE_EFFECT(CVignettingEffect, c17_vignetting);

ConVar r_post_vignetting_darkness("r_post_vignetting_darkness", "2.5", FCVAR_CHEAT, "Controls the vignetting shader's power. 0 for off.");
ConVar r_post_vignettingeffect_debug("r_post_vignettingeffect_debug", "0", FCVAR_CHEAT);

ConVar r_post_vignettingeffect("r_post_vignettingeffect", "1", FCVAR_ARCHIVE);

//------------------------------------------------------------------------------
// CVignettingEffect init
//------------------------------------------------------------------------------
void CVignettingEffect::Init()
{
	m_VignetMat.Init(materials->FindMaterial("effects/shaders/vignetting", TEXTURE_GROUP_PIXEL_SHADERS, true));

	fVignettingAmount = r_post_vignetting_darkness.GetFloat();
	fVignettingLerpTo = r_post_vignetting_darkness.GetFloat();
}


//------------------------------------------------------------------------------
// CVignettingEffect shutdown
//------------------------------------------------------------------------------
void CVignettingEffect::Shutdown()
{
	m_VignetMat.Shutdown();
}

//------------------------------------------------------------------------------
// CVignettingEffect render
//------------------------------------------------------------------------------
void CVignettingEffect::Render(int x, int y, int w, int h)
{
	if (!r_post_vignettingeffect.GetBool() || (IsEnabled() == false))
		return;

	fVignettingLerpTo = r_post_vignetting_darkness.GetFloat();

	if (fVignettingAmount != fVignettingLerpTo)
		fVignettingAmount = FLerp(fVignettingAmount, fVignettingLerpTo, 0.03f);

	IMaterialVar *var;

	if (fVignettingAmount >= 0.01f)
	{
		var = m_VignetMat->FindVar("$VIGNETDARKNESS", NULL);
		var->SetFloatValue(fVignettingAmount);
		DrawScreenEffectMaterial(m_VignetMat, x, y, w, h);
		if (r_post_vignettingeffect_debug.GetBool())
			DevMsg("Vignetting Amount: %.2f\n", fVignettingAmount);
	}
}

//------------------------------------------------------------------------------
// Unsharp post-processing effect
//------------------------------------------------------------------------------
class CUnsharpEffect : public IScreenSpaceEffect
{
public:
	CUnsharpEffect() { };
	~CUnsharpEffect() { };

	void Init();
	void Shutdown();

	void SetParameters(KeyValues *params) { };

	void Render(int x, int y, int w, int h);

	void Enable(bool bEnable) { m_bEnable = bEnable; }
	bool IsEnabled() { return m_bEnable; }

private:

	bool				m_bEnable;

	CTextureReference	m_UnsharpBlurFB;
	CMaterialReference	m_UnsharpBlur;
	CMaterialReference	m_Unsharp;
};

ADD_SCREENSPACE_EFFECT(CUnsharpEffect, c17_unsharp);

ConVar r_post_unsharp("r_post_unsharp", "1", FCVAR_ARCHIVE);
ConVar r_post_unsharp_debug("r_post_unsharp_debug", "0", FCVAR_CHEAT);
ConVar r_post_unsharp_strength("r_post_unsharp_strength", "0.5", FCVAR_CHEAT);
ConVar r_post_unsharp_blursize("r_post_unsharp_blursize", "5.0", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// CUnsharpEffect init
//------------------------------------------------------------------------------
void CUnsharpEffect::Init()
{
	m_UnsharpBlurFB.InitRenderTarget(ScreenWidth() / 2, ScreenHeight() / 2, RT_SIZE_DEFAULT, IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_NONE, false, "_rt_UnsharpBlur");

	m_UnsharpBlur.Init(materials->FindMaterial("effects/shaders/unsharp_blur", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_Unsharp.Init(materials->FindMaterial("effects/shaders/unsharp", TEXTURE_GROUP_PIXEL_SHADERS, true));
}


//------------------------------------------------------------------------------
// CUnsharpEffect shutdown
//------------------------------------------------------------------------------
void CUnsharpEffect::Shutdown()
{
	m_UnsharpBlurFB.Shutdown();
	m_UnsharpBlur.Shutdown();
	m_Unsharp.Shutdown();
}

//------------------------------------------------------------------------------
// CUnsharpEffect render
//------------------------------------------------------------------------------
void CUnsharpEffect::Render(int x, int y, int w, int h)
{
	if (!r_post_unsharp.GetBool() || (IsEnabled() == false))
		return;

	// Grab the render context
	CMatRenderContextPtr pRenderContext(materials);

	// Set to the proper rendering mode.
	pRenderContext->MatrixMode(MATERIAL_VIEW);
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
	pRenderContext->MatrixMode(MATERIAL_PROJECTION);
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	IMaterialVar *var;
	var = m_UnsharpBlur->FindVar("$blursize", NULL);
	var->SetFloatValue(r_post_unsharp_blursize.GetFloat());

	if (r_post_unsharp_debug.GetBool())
	{
		DrawScreenEffectMaterial(m_UnsharpBlur, x, y, w, h);
		return;
	}

	Rect_t actualRect;
	UpdateScreenEffectTexture(0, x, y, w, h, false, &actualRect);
	pRenderContext->PushRenderTargetAndViewport(m_UnsharpBlurFB);
	DrawScreenEffectQuad(m_UnsharpBlur, m_UnsharpBlurFB->GetActualWidth(), m_UnsharpBlurFB->GetActualHeight());
	pRenderContext->PopRenderTargetAndViewport();

	//Restore our state
	pRenderContext->MatrixMode(MATERIAL_VIEW);
	pRenderContext->PopMatrix();
	pRenderContext->MatrixMode(MATERIAL_PROJECTION);
	pRenderContext->PopMatrix();

	var = m_Unsharp->FindVar("$fbblurtexture", NULL);
	var->SetTextureValue(m_UnsharpBlurFB);
	var = m_Unsharp->FindVar("$unsharpstrength", NULL);
	var->SetFloatValue(r_post_unsharp_strength.GetFloat());
	var = m_Unsharp->FindVar("$blursize", NULL);
	var->SetFloatValue(r_post_unsharp_blursize.GetFloat());

	DrawScreenEffectMaterial(m_Unsharp, x, y, w, h);
}

//------------------------------------------------------------------------------
// Water post-processing effects
//------------------------------------------------------------------------------
class CWaterEffects : public IScreenSpaceEffect
{
public:
	CWaterEffects(void) { };

	virtual void Init(void);
	virtual void Shutdown(void);
	virtual void SetParameters(KeyValues *params);
	virtual void Enable(bool bEnable) { m_bEnabled = bEnable; }
	virtual bool IsEnabled() { return m_bEnabled; }

	virtual void Render(int x, int y, int w, int h);

	virtual float GetViscosity() { return fViscosity; }
	virtual float GetAmount() { return fAmount; }
	virtual void SetViscosity(float fNewViscosity) { fViscosity = fNewViscosity; }
	virtual void SetAmount(float fNewAmount) { fAmount = fNewAmount; }
	virtual bool IsUnderwater() { return m_bUnderwater; }

private:
	bool	m_bEnabled;
	bool	m_bUnderwater;

	float	fViscosity;
	float	fAmount;

	CMaterialReference	m_ChromaticDisp;
	CMaterialReference	m_WaterFX;
	CMaterialReference	m_BlurX;
	CMaterialReference	m_BlurY;
};

ADD_SCREENSPACE_EFFECT(CWaterEffects, c17_waterfx);

ConVar r_post_watereffects_underwater_chromaticoffset("r_post_watereffects_underwater_chromaticoffset", "1.0", FCVAR_CHEAT);
ConVar r_post_watereffects_underwater_amount("r_post_watereffects_underwater_amount", "0.1", FCVAR_CHEAT);
ConVar r_post_watereffects_underwater_viscosity("r_post_watereffects_underwater_viscosity", "1.0", FCVAR_CHEAT);
ConVar r_post_watereffects_lerp_viscosity("r_post_watereffects_lerp_viscosity", "0.01", FCVAR_CHEAT);
ConVar r_post_watereffects_lerp_amount("r_post_watereffects_lerp_amount", "0.005", FCVAR_CHEAT);
ConVar r_post_watereffects_underwater_gaussianamount("r_post_watereffects_underwater_gaussianamount", "1.5", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// CWaterEffects init
//------------------------------------------------------------------------------
void CWaterEffects::Init(void)
{
	fViscosity = 0.01;
	fAmount = 0;
	m_bUnderwater = false;

	PrecacheMaterial("effects/shaders/chromaticDisp");
	PrecacheMaterial("effects/shaders/screenwater");
	PrecacheMaterial("effects/shaders/screen_blurx");
	PrecacheMaterial("effects/shaders/screen_blury");

	m_ChromaticDisp.Init(materials->FindMaterial("effects/shaders/chromaticDisp", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_WaterFX.Init(materials->FindMaterial("effects/shaders/screenwater", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_BlurX.Init(materials->FindMaterial("effects/shaders/screen_blurx", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_BlurY.Init(materials->FindMaterial("effects/shaders/screen_blury", TEXTURE_GROUP_PIXEL_SHADERS, true));
}

//------------------------------------------------------------------------------
// CWaterEffects shutdown
//------------------------------------------------------------------------------
void CWaterEffects::Shutdown(void)
{
	m_ChromaticDisp.Shutdown();
	m_WaterFX.Shutdown();
	m_BlurX.Shutdown();
	m_BlurY.Shutdown();
}

//------------------------------------------------------------------------------
// CWaterEffects SetParameters
//------------------------------------------------------------------------------
void CWaterEffects::SetParameters(KeyValues *params)
{
	if (IsUnderwater())
		return;

	float in, temp;

	if (params->FindKey("amount"))
	{
		in = params->GetFloat("amount");
		temp = GetAmount();
		temp += in;
		if (temp > 0.1f)
			temp = 0.1f;

		SetAmount(temp);
	}

	if (params->FindKey("viscosity"))
	{
		in = params->GetFloat("viscosity");
		temp = GetViscosity();
		temp += in;
		if (temp > 1.0f)
			temp = 1.0f;

		SetViscosity(temp);
	}
}

ConVar r_post_watereffects("r_post_watereffects", "1", FCVAR_ARCHIVE);
ConVar r_post_watereffects_debug("r_post_watereffects_debug", "0", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// CWaterEffects render
//------------------------------------------------------------------------------
void CWaterEffects::Render(int x, int y, int w, int h)
{
	if (!r_post_watereffects.GetBool() || (IsEnabled() == false))
		return;

	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	IMaterialVar *var;

	if (pPlayer->GetWaterLevel() >= 3)
	{
		m_bUnderwater = true;
		fViscosity = r_post_watereffects_underwater_viscosity.GetFloat();
		fAmount = r_post_watereffects_underwater_amount.GetFloat();

		//Gaussian Blur the screen
		var = m_BlurX->FindVar("$BLURSIZE", NULL);
		var->SetFloatValue(r_post_watereffects_underwater_gaussianamount.GetFloat());
		var = m_BlurX->FindVar("$RESDIVISOR", NULL);
		var->SetIntValue(1);
		DrawScreenEffectMaterial(m_BlurX, x, y, w, h);
		var = m_BlurY->FindVar("$BLURSIZE", NULL);
		var = m_BlurY->FindVar("$RESDIVISOR", NULL);
		var->SetIntValue(1);
		var->SetFloatValue(r_post_watereffects_underwater_gaussianamount.GetFloat());
		DrawScreenEffectMaterial(m_BlurY, x, y, w, h);

		//Render Chromatic Dispersion
		var = m_ChromaticDisp->FindVar("$FOCUSOFFSET", NULL);
		var->SetFloatValue(r_post_watereffects_underwater_chromaticoffset.GetFloat());
		var = m_ChromaticDisp->FindVar("$radial", NULL);
		var->SetIntValue(0);
		DrawScreenEffectMaterial(m_ChromaticDisp, x, y, w, h);
	}
	else
	{
		m_bUnderwater = false;

		if (fViscosity != 0.01)
			fViscosity = FLerp(fViscosity, 0.01, r_post_watereffects_lerp_viscosity.GetFloat());

		if (fAmount != 0)
			fAmount = FLerp(fAmount, 0, r_post_watereffects_lerp_amount.GetFloat());

		if (fAmount < 0.01)
		{
			if (r_post_watereffects_debug.GetBool())
			{
				DevMsg("Water Effects Stopped.\n");
			}
			return;
		}
	}

	var = m_WaterFX->FindVar("$AMOUNT", NULL);
	var->SetFloatValue(fAmount);
	var = m_WaterFX->FindVar("$VISCOSITY", NULL);
	var->SetFloatValue(fViscosity);
	DrawScreenEffectMaterial(m_WaterFX, x, y, w, h);

	if (r_post_watereffects_debug.GetBool())
	{
		DevMsg("Water Amount: %.2f\n", fAmount);
		DevMsg("Water Viscosity: %.2f\n", fViscosity);
	}
}

class CColorCorrectionEffect : public IScreenSpaceEffect
{
public:
	CColorCorrectionEffect(void) { };

	virtual void Init(void);
	virtual void Shutdown(void);
	virtual void SetParameters(KeyValues *params) {};
	virtual void Enable(bool bEnable) { m_bEnabled = bEnable; }
	virtual bool IsEnabled() { return m_bEnabled; }

	virtual void Render(int x, int y, int w, int h);

private:
	bool				m_bEnabled;

	CMaterialReference	m_Negative;
	CMaterialReference	m_BleachBypass;
	CMaterialReference	m_ColorClipping;
	CMaterialReference	m_CrossProcessing;
	CMaterialReference	m_NextGen;
	CMaterialReference	m_Complements;
	CMaterialReference	m_CubicDistortion;
	CMaterialReference	m_Desaturate;
	CMaterialReference	m_Nightvision;
};

// Color Correction Extensions
ADD_SCREENSPACE_EFFECT(CColorCorrectionEffect, c17_colorcorrection);

ConVar r_post_negative("r_post_negative", "0", FCVAR_CHEAT);

ConVar r_post_bleach_bypass("r_post_bleach_bypass", "1", FCVAR_CHEAT);
ConVar r_post_bleach_bypass_opacity("r_post_bleach_bypass_opacity", "0.01", FCVAR_CHEAT);

ConVar r_post_color_clipping("r_post_color_clipping", "1", FCVAR_CHEAT);
ConVar r_post_color_clipping_mincolor_r("r_post_color_clipping_mincolor_r", "0", FCVAR_CHEAT);
ConVar r_post_color_clipping_mincolor_g("r_post_color_clipping_mincolor_g", "0", FCVAR_CHEAT);
ConVar r_post_color_clipping_mincolor_b("r_post_color_clipping_mincolor_b", "0", FCVAR_CHEAT);
ConVar r_post_color_clipping_mincolor_a("r_post_color_clipping_mincolor_a", "1", FCVAR_CHEAT);
ConVar r_post_color_clipping_maxcolor_r("r_post_color_clipping_maxcolor_r", "1", FCVAR_CHEAT);
ConVar r_post_color_clipping_maxcolor_g("r_post_color_clipping_maxcolor_g", "1", FCVAR_CHEAT);
ConVar r_post_color_clipping_maxcolor_b("r_post_color_clipping_maxcolor_b", "1", FCVAR_CHEAT);
ConVar r_post_color_clipping_maxcolor_a("r_post_color_clipping_maxcolor_a", "1", FCVAR_CHEAT);
ConVar r_post_color_clipping_squish("r_post_color_clipping_squish", "1", FCVAR_CHEAT);

ConVar r_post_cross_processing("r_post_cross_processing", "0", FCVAR_CHEAT);
ConVar r_post_cross_processing_saturation("r_post_cross_processing_saturation", "0.8", FCVAR_CHEAT);
ConVar r_post_cross_processing_contrast("r_post_cross_processing_contrast", "1.0", FCVAR_CHEAT);
ConVar r_post_cross_processing_brightness("r_post_cross_processing_brightness", "0.0", FCVAR_CHEAT);
ConVar r_post_cross_processing_intensity("r_post_cross_processing_intensity", "0.2", FCVAR_CHEAT);

ConVar r_post_complements_guidehue_r("r_post_complements_guidehue_r", "1", FCVAR_CHEAT);
ConVar r_post_complements_guidehue_g("r_post_complements_guidehue_g", "1", FCVAR_CHEAT);
ConVar r_post_complements_guidehue_b("r_post_complements_guidehue_b", "1", FCVAR_CHEAT);
ConVar r_post_complements_amount("r_post_complements_amount", "0.05", FCVAR_CHEAT);
ConVar r_post_complements_concentrate("r_post_complements_concentrate", "1.0", FCVAR_CHEAT);
ConVar r_post_complements_desatcorr("r_post_complements_desatcorr", "1.0", FCVAR_CHEAT);

ConVar r_post_cubic_distortion("r_post_cubic_distortion", "1", FCVAR_CHEAT);
ConVar r_post_cubic_distortion_amount("r_post_cubic_distortion_amount", "-0.02", FCVAR_CHEAT);
ConVar r_post_cubic_distortion_cubicamount("r_post_cubic_distortion_cubicamount", "0.1", FCVAR_CHEAT);

ConVar r_post_desaturate("r_post_desaturate", "1", FCVAR_CHEAT);
ConVar r_post_desaturate_strength("r_post_desaturate_strength", "0.05", FCVAR_CHEAT);

ConVar r_post_nightvision("r_post_nightvision", "0", FCVAR_CHEAT);

void nextgen_callback(IConVar *pConVar, char const *pOldString, float flOldValue);

static ConVar r_post_nextgen("r_post_nextgen", "0", FCVAR_CHEAT, "THE MOST IMPORTANT ASPECT OF CITY17! IT'LL BLOW. YOUR. MIND.", nextgen_callback);

extern ConVar r_post_complements;

void nextgen_callback(IConVar *pConVar, char const *pOldString, float flOldValue)
{
	if (r_post_nextgen.GetBool())
	{
		Msg("Shit just got unreal!\n");
	}
}

void CColorCorrectionEffect::Init(void)
{
	m_Negative.Init(materials->FindMaterial("effects/shaders/negative", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_BleachBypass.Init(materials->FindMaterial("effects/shaders/bleach_bypass", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_ColorClipping.Init(materials->FindMaterial("effects/shaders/color_clipping", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_CrossProcessing.Init(materials->FindMaterial("effects/shaders/cross_processing", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_NextGen.Init(materials->FindMaterial("effects/shaders/nextgen", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_Complements.Init(materials->FindMaterial("effects/shaders/complements", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_CubicDistortion.Init(materials->FindMaterial("effects/shaders/cubic_distortion", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_Desaturate.Init(materials->FindMaterial("effects/shaders/desaturate", TEXTURE_GROUP_PIXEL_SHADERS, true));
	m_Nightvision.Init(materials->FindMaterial("effects/shaders/nightvision", TEXTURE_GROUP_PIXEL_SHADERS, true));
}

void CColorCorrectionEffect::Shutdown(void)
{
	m_Negative.Shutdown();
	m_BleachBypass.Shutdown();
	m_ColorClipping.Shutdown();
	m_CrossProcessing.Shutdown();
	m_NextGen.Shutdown();
	m_Complements.Shutdown();
	m_CubicDistortion.Shutdown();
	m_Desaturate.Shutdown();
	m_Nightvision.Shutdown();
}

ConVar r_post_colorcorrection("r_post_colorcorrection", "1", FCVAR_ARCHIVE);
void CColorCorrectionEffect::Render(int x, int y, int w, int h)
{
	VPROF("CColorCorrectionEffect::Render");

	if (!r_post_colorcorrection.GetBool() || !IsEnabled())
		return;

	IMaterialVar *var;

	// Grab the render context
	CMatRenderContextPtr pRenderContext(materials);

	if (r_post_negative.GetBool())
	{
		DrawScreenEffectMaterial(m_Negative, x, y, w, h);
	}

	if (r_post_bleach_bypass.GetBool())
	{
		var = m_BleachBypass->FindVar("$OPACITY", NULL);
		var->SetFloatValue(r_post_bleach_bypass_opacity.GetFloat());
		DrawScreenEffectMaterial(m_BleachBypass, x, y, w, h);
	}

	if (r_post_color_clipping.GetBool())
	{
		var = m_ColorClipping->FindVar("$mincolor", NULL);
		var->SetVecValue(r_post_color_clipping_mincolor_r.GetFloat(), r_post_color_clipping_mincolor_g.GetFloat(), r_post_color_clipping_mincolor_b.GetFloat(), r_post_color_clipping_mincolor_a.GetFloat());
		var = m_ColorClipping->FindVar("$maxcolor", NULL);
		var->SetVecValue(r_post_color_clipping_maxcolor_r.GetFloat(), r_post_color_clipping_maxcolor_g.GetFloat(), r_post_color_clipping_maxcolor_b.GetFloat(), r_post_color_clipping_maxcolor_a.GetFloat());
		var = m_ColorClipping->FindVar("$squish", NULL);
		var->SetIntValue(r_post_color_clipping_squish.GetInt());
		DrawScreenEffectMaterial(m_ColorClipping, x, y, w, h);
	}

	if (r_post_cross_processing.GetBool())
	{
		var = m_CrossProcessing->FindVar("$MUTABLE_01", NULL);
		var->SetFloatValue(r_post_cross_processing_saturation.GetFloat());
		var = m_CrossProcessing->FindVar("$MUTABLE_02", NULL);
		var->SetFloatValue(r_post_cross_processing_contrast.GetFloat());
		var = m_CrossProcessing->FindVar("$MUTABLE_03", NULL);
		var->SetFloatValue(r_post_cross_processing_brightness.GetFloat());
		var = m_CrossProcessing->FindVar("$MUTABLE_04", NULL);
		var->SetFloatValue(r_post_cross_processing_intensity.GetFloat());
		DrawScreenEffectMaterial(m_CrossProcessing, x, y, w, h);
	}

	if (r_post_nextgen.GetBool())
	{
		DrawScreenEffectMaterial(m_NextGen, x, y, w, h);
	}

	if (r_post_complements.GetBool())
	{
		var = m_Complements->FindVar("$guidehue", NULL);
		var->SetVecValue(r_post_complements_guidehue_r.GetFloat(), r_post_complements_guidehue_g.GetFloat(), r_post_complements_guidehue_b.GetFloat());
		var = m_Complements->FindVar("$amount", NULL);
		var->SetFloatValue(r_post_complements_amount.GetFloat());
		var = m_Complements->FindVar("$concentrate", NULL);
		var->SetFloatValue(r_post_complements_concentrate.GetFloat());
		var = m_Complements->FindVar("$desatcorr", NULL);
		var->SetFloatValue(r_post_complements_desatcorr.GetFloat());
		DrawScreenEffectMaterial(m_Complements, x, y, w, h);
	}

	if (r_post_cubic_distortion.GetBool())
	{
		var = m_CubicDistortion->FindVar("$distortion", NULL);
		var->SetFloatValue(r_post_cubic_distortion_amount.GetFloat());
		var = m_CubicDistortion->FindVar("$cubicdistortion", NULL);
		var->SetFloatValue(r_post_cubic_distortion_cubicamount.GetFloat());
		DrawScreenEffectMaterial(m_CubicDistortion, x, y, w, h);
	}

	if (r_post_desaturate.GetBool())
	{
		var = m_Desaturate->FindVar("$strength", NULL);
		var->SetFloatValue(r_post_desaturate_strength.GetFloat());
		DrawScreenEffectMaterial(m_Desaturate, x, y, w, h);
	}

	if (r_post_nightvision.GetBool())
	{
		DrawScreenEffectMaterial(m_Nightvision, x, y, w, h);
	}
}