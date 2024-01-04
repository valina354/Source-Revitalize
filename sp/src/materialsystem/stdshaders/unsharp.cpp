﻿//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#include "unsharp_vs20.inc"
#include "unsharp_ps20.inc"
#include "unsharp_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( Unsharp, "Help for Unsharp" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "" )
		SHADER_PARAM( FBBLURTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_UnsharpBlur", "" )
		SHADER_PARAM( UNSHARPSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "1.0", "" )
		SHADER_PARAM( BLURSIZE, SHADER_PARAM_TYPE_FLOAT, "0.5", "" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[FBTEXTURE]->IsDefined() )
		{
			LoadTexture( FBTEXTURE );
		}

		if( params[FBBLURTEXTURE]->IsDefined() )
		{
			LoadTexture( FBBLURTEXTURE );
		}
	}

	// ----------------------------------------------------------------------------
	// We want this shader to operate on the frame buffer itself. Therefore,
	// we need to set this to true.
	// ----------------------------------------------------------------------------
	bool NeedsFullFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame /* = true */) const
	{
		return true;
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			Assert( 0 );
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// Pre-cache shaders
			DECLARE_STATIC_VERTEX_SHADER( unsharp_vs20 );
			SET_STATIC_VERTEX_SHADER( unsharp_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( unsharp_ps20b );
				SET_STATIC_PIXEL_SHADER( unsharp_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( unsharp_ps20 );
				SET_STATIC_PIXEL_SHADER( unsharp_ps20 );
			}
		}

		DYNAMIC_STATE
		{
			int nWidth, nHeight;
			pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );
			float v_const0[4]={ float((nWidth/2)*(nHeight/2)), 0.0f, 0.0f, 0.0f }; //Window Size
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, v_const0 );

			float v_const1[4]={ params[BLURSIZE]->GetFloatValue(), 0.0f, 0.0f, 0.0f }; //Blur Size
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, v_const1 );
			
			float g_const0[4] = { params[UNSHARPSTRENGTH]->GetFloatValue(), 0.0f, 0.0f, 0.0f };
			pShaderAPI->SetPixelShaderConstant( 0, g_const0 );

			BindTexture( SHADER_SAMPLER0, FBTEXTURE, -1 );
			BindTexture( SHADER_SAMPLER1, FBBLURTEXTURE, -1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( unsharp_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( unsharp_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( unsharp_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( unsharp_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( unsharp_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( unsharp_ps20 );
			}
		}
		Draw();
	}
END_SHADER