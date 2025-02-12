//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef VIEWPOSTPROCESS_H
#define VIEWPOSTPROCESS_H

#if defined( _WIN32 )
#pragma once
#endif

struct PostProcessParameters_t;

void DoEnginePostProcessing( int x, int y, int w, int h, bool bFlashlightIsOn, bool bPostVGui = false );
void DoImageSpaceMotionBlur( const CViewSetup &viewSetup );
void DumpTGAofRenderTarget( const int width, const int height, const char *pFilename );
void DoSSAO(const CViewSetup &viewSet); // crossroads devtest
bool DoSunAndGlowEffects(int x, int y, int w, int h);

void SetRenderTargetAndViewPort( ITexture *rt );

bool IsDepthOfFieldEnabled();
void DoDepthOfField( const CViewSetup &view );

void SetPostProcessParams( const PostProcessParameters_t* pPostProcessParameters );
void SetPostProcessParams( const PostProcessParameters_t* pPostProcessParameters, bool override );

void SetViewFadeParams( byte r, byte g, byte b, byte a, bool bModulate );

#ifdef MAPBASE
void DoObjectMotionBlur(const CViewSetup* pSetup);
#endif // MAPBASE


#endif // VIEWPOSTPROCESS_H
