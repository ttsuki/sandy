/// @file
///	@brief   sandy::d3d11::RenderTarget
///	@author  (C) 2023 ttsuki

#include "RenderTarget.h"

#include <Windows.h>
#include <d3d11.h>
#include <xtw/com.h>
#include "../misc/Math.h"

#include "Device.h"

namespace sandy::d3d11
{
    RenderTarget::RenderTarget(ID3D11Device* device, LONG width, LONG height, DXGI_FORMAT render_target_format, DXGI_FORMAT depth_stencil_format, UINT msaa_sample_count)
        : BufferWidth(width)
        , BufferHeight(height)
        , RenderTargetFormat(render_target_format)
        , DepthStencilFormat(depth_stencil_format)
        , ScreenViewport(D3D11_VIEWPORT{0.0f, 0.0f, static_cast<float>(BufferWidth), static_cast<float>(BufferHeight), 0.0f, 1.0f})
        , RenderTargetTexture(Device(device).CreateTexture2D(CD3D11_TEXTURE2D_DESC(RenderTargetFormat, BufferWidth, BufferHeight, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, msaa_sample_count, 0, 0)))
        , DepthStencilTexture(Device(device).CreateTexture2D(CD3D11_TEXTURE2D_DESC(DepthStencilFormat, BufferWidth, BufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL, D3D11_USAGE_DEFAULT, 0, msaa_sample_count, 0, 0)))
        , RenderTargetView(Device(device).CreateRenderTargetView(RenderTargetTexture.get(), CD3D11_RENDER_TARGET_VIEW_DESC(RenderTargetTexture.get(), msaa_sample_count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D, RenderTargetFormat)))
        , DepthStencilView(Device(device).CreateDepthStencilView(DepthStencilTexture.get(), CD3D11_DEPTH_STENCIL_VIEW_DESC(DepthStencilTexture.get(), msaa_sample_count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D, DepthStencilFormat)))
    {
        //
    }

    void RenderTarget::Clear(ID3D11DeviceContext* context, const Color4& color, FLOAT depth, UINT8 stencil) const
    {
        context->ClearRenderTargetView(RenderTargetView.get(), color.pointer());
        context->ClearDepthStencilView(DepthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
        context->OMSetRenderTargets(1, RenderTargetView.get_address(), DepthStencilView.get());
        context->RSSetViewports(1, &ScreenViewport);
    }
}
