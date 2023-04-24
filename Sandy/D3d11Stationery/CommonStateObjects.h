/// @file
///	@brief   sandy::d3d11::stationery
///	@author  (C) 2023 ttsuki

#pragma once
#include <Windows.h>
#include <combaseapi.h>
#include <d3d11.h>

#include <xtw/com.h>

#include "../D3d11/Device.h"

namespace sandy::d3d11
{
    struct CommonRasterizerStates
    {
        const xtw::com_ptr<ID3D11RasterizerState> CullNone;
        const xtw::com_ptr<ID3D11RasterizerState> CullCW;
        const xtw::com_ptr<ID3D11RasterizerState> CullCCW;
        const xtw::com_ptr<ID3D11RasterizerState> WireFrame;

        explicit CommonRasterizerStates(const Device& factory, bool depth_clip_enabled = true, bool scissor_enabled = false, bool multi_sample_enabled = false, bool antialiased_line_enabled = false)
            : CullNone(factory.CreateRasterizerState({D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE, D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP, D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, depth_clip_enabled, scissor_enabled, multi_sample_enabled, antialiased_line_enabled}))
            , CullCW(factory.CreateRasterizerState({D3D11_FILL_SOLID, D3D11_CULL_FRONT, FALSE, D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP, D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, depth_clip_enabled, scissor_enabled, multi_sample_enabled, antialiased_line_enabled}))
            , CullCCW(factory.CreateRasterizerState({D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE, D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP, D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, depth_clip_enabled, scissor_enabled, multi_sample_enabled, antialiased_line_enabled}))
            , WireFrame(factory.CreateRasterizerState({D3D11_FILL_WIREFRAME, D3D11_CULL_BACK, FALSE, D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP, D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, depth_clip_enabled, scissor_enabled, multi_sample_enabled, antialiased_line_enabled}))
        {
            //
        }
    };

    struct CommonSamplerState
    {
        const xtw::com_ptr<ID3D11SamplerState> Point;
        const xtw::com_ptr<ID3D11SamplerState> Liner;
        const xtw::com_ptr<ID3D11SamplerState> Anisotropic;

        explicit CommonSamplerState(const Device& factory)
            : Point(factory.CreateSamplerState({D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_DEFAULT_MIP_LOD_BIAS, D3D11_DEFAULT_MAX_ANISOTROPY, D3D11_COMPARISON_NEVER, {0.0f, 0.0f, 0.0f, 0.0f}, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()}))
            , Liner(factory.CreateSamplerState({D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_DEFAULT_MIP_LOD_BIAS, D3D11_DEFAULT_MAX_ANISOTROPY, D3D11_COMPARISON_NEVER, {0.0f, 0.0f, 0.0f, 0.0f}, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()}))
            , Anisotropic(factory.CreateSamplerState({D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_DEFAULT_MIP_LOD_BIAS, D3D11_DEFAULT_MAX_ANISOTROPY, D3D11_COMPARISON_NEVER, {0.0f, 0.0f, 0.0f, 0.0f}, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()}))
        {
            //
        }
    };

    struct CommonBlendState
    {
        const xtw::com_ptr<ID3D11BlendState> Copy;
        const xtw::com_ptr<ID3D11BlendState> AlphaBlend;
        const xtw::com_ptr<ID3D11BlendState> AddBlend;
        const xtw::com_ptr<ID3D11BlendState> Multiply;
        const xtw::com_ptr<ID3D11BlendState> SubtractiveBlend;

        explicit CommonBlendState(const Device& factory, bool alpha_to_coverage = false)
            : Copy(factory.CreateBlendState({alpha_to_coverage, FALSE, {{TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL}}}))
            , AlphaBlend(factory.CreateBlendState({alpha_to_coverage, FALSE, {{TRUE, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_OP_MAX, D3D11_COLOR_WRITE_ENABLE_ALL}}}))
            , AddBlend(factory.CreateBlendState({alpha_to_coverage, FALSE, {{TRUE, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_OP_MAX, D3D11_COLOR_WRITE_ENABLE_ALL}}}))
            , Multiply(factory.CreateBlendState({alpha_to_coverage, FALSE, {{TRUE, D3D11_BLEND_ZERO, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL}}}))
            , SubtractiveBlend(factory.CreateBlendState({alpha_to_coverage, FALSE, {{TRUE, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_OP_REV_SUBTRACT, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_OP_MAX, D3D11_COLOR_WRITE_ENABLE_ALL}}}))
        {
            //
        }
    };

    struct CommonDepthStencilState
    {
        const xtw::com_ptr<ID3D11DepthStencilState> DepthEnabled;
        const xtw::com_ptr<ID3D11DepthStencilState> DepthDisabled;
        const xtw::com_ptr<ID3D11DepthStencilState> DepthReadOnly;

        explicit CommonDepthStencilState(const Device& factory)
            : DepthEnabled(factory.CreateDepthStencilState({TRUE, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS, FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK, {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS}, {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS}}))
            , DepthDisabled(factory.CreateDepthStencilState({FALSE, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_ALWAYS, FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK, {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS}, {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS}}))
            , DepthReadOnly(factory.CreateDepthStencilState({TRUE, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_LESS, FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK, {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS}, {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS}}))
        {
            //
        }
    };

    struct CommonTextures
    {
        const xtw::com_ptr<ID3D11ShaderResourceView> White;
        const xtw::com_ptr<ID3D11ShaderResourceView> Black;
        const xtw::com_ptr<ID3D11ShaderResourceView> Transparent;
        const xtw::com_ptr<ID3D11ShaderResourceView> TestPattern;

        explicit CommonTextures(const Device& factory);
    };
}
