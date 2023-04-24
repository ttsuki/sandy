/// @file
///	@brief   sandy::d3d11::RenderTarget
///	@author  (C) 2023 ttsuki

#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <xtw/com.h>

#include "../misc/Math.h"

namespace sandy::d3d11
{
    struct RenderTarget
    {
        const LONG BufferWidth{};
        const LONG BufferHeight{};
        const DXGI_FORMAT RenderTargetFormat{};
        const DXGI_FORMAT DepthStencilFormat{};
        const D3D11_VIEWPORT ScreenViewport{};
        const xtw::com_ptr<ID3D11Texture2D> RenderTargetTexture{};
        const xtw::com_ptr<ID3D11Texture2D> DepthStencilTexture{};
        const xtw::com_ptr<ID3D11RenderTargetView> RenderTargetView{};
        const xtw::com_ptr<ID3D11DepthStencilView> DepthStencilView{};

        RenderTarget(
            ID3D11Device* device,
            LONG width,
            LONG height,
            DXGI_FORMAT render_target_format = DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT depth_stencil_format = DXGI_FORMAT_D32_FLOAT,
            UINT msaa_sample_count = 1);

        void Clear(ID3D11DeviceContext* context, const Color4& color, FLOAT depth = 1.0f, UINT8 stencil = 0) const;
    };
}
