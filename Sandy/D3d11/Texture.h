/// @file
///	@brief   sandy::d3d11::Texture
///	@author  (C) 2023 ttsuki

#pragma once
#include <d3d11.h>
#include <xtw/com.h>

#include "../misc/Span.h"
#include "./UtilityFunctions.h"

namespace sandy::d3d11
{
    static inline D3D11_TEXTURE1D_DESC TextureDescFrom(ID3D11Texture1D* tex) { return util::GetDesc(tex); }
    static inline D3D11_TEXTURE2D_DESC TextureDescFrom(ID3D11Texture2D* tex) { return util::GetDesc(tex); }
    static inline D3D11_TEXTURE3D_DESC TextureDescFrom(ID3D11Texture3D* tex) { return util::GetDesc(tex); }
    static inline D3D11_TEXTURE1D_DESC TextureDescFrom(const xtw::com_ptr<ID3D11Texture1D>& tex) { return util::GetDesc(tex); }
    static inline D3D11_TEXTURE2D_DESC TextureDescFrom(const xtw::com_ptr<ID3D11Texture2D>& tex) { return util::GetDesc(tex); }
    static inline D3D11_TEXTURE3D_DESC TextureDescFrom(const xtw::com_ptr<ID3D11Texture3D>& tex) { return util::GetDesc(tex); }

    xtw::com_ptr<ID3D11ShaderResourceView> AllocateShaderResourceView(const xtw::com_ptr<ID3D11Texture1D>& texture);
    xtw::com_ptr<ID3D11ShaderResourceView> AllocateShaderResourceView(const xtw::com_ptr<ID3D11Texture2D>& texture);
    xtw::com_ptr<ID3D11ShaderResourceView> AllocateShaderResourceView(const xtw::com_ptr<ID3D11Texture3D>& texture);


    xtw::com_ptr<ID3D11Texture2D> LoadTextureFromFileInMemory(
        ID3D11Device* device,
        const void* file_image, size_t file_image_length,
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
        UINT bind_flag = D3D11_BIND_SHADER_RESOURCE,
        UINT cpu_access_flags = 0);


    // common texture description templates
    struct DYNAMIC_TEXTURE_DESC : CD3D11_TEXTURE2D_DESC
    {
        DYNAMIC_TEXTURE_DESC(
            DXGI_FORMAT format, UINT width, UINT height,
            UINT arraySize = 1, UINT mipLevels = 1,
            UINT bindFlags = D3D11_BIND_SHADER_RESOURCE,
            D3D11_USAGE usage = D3D11_USAGE_DYNAMIC,
            UINT cpuAccessFlags = D3D11_CPU_ACCESS_WRITE,
            UINT sampleCount = 1,
            UINT sampleQuality = 0,
            UINT miscFlags = 0) : CD3D11_TEXTURE2D_DESC(format, width, height, arraySize, mipLevels, bindFlags, usage, cpuAccessFlags, sampleCount, sampleQuality, miscFlags) {}
    };

    struct STAGING_TEXTURE_DESC : CD3D11_TEXTURE2D_DESC
    {
        STAGING_TEXTURE_DESC(
            DXGI_FORMAT format, UINT width, UINT height,
            UINT arraySize = 1, UINT mipLevels = 1,
            UINT bindFlags = 0,
            D3D11_USAGE usage = D3D11_USAGE_STAGING,
            UINT cpuAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
            UINT sampleCount = 1,
            UINT sampleQuality = 0,
            UINT miscFlags = 0) : CD3D11_TEXTURE2D_DESC(format, width, height, arraySize, mipLevels, bindFlags, usage, cpuAccessFlags, sampleCount, sampleQuality, miscFlags) {}
    };

    struct RENDER_TARGET_TEXTURE_DESC : CD3D11_TEXTURE2D_DESC
    {
        RENDER_TARGET_TEXTURE_DESC(
            DXGI_FORMAT format, UINT width, UINT height,
            UINT arraySize = 1, UINT mipLevels = 1,
            UINT bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
            D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
            UINT cpuAccessFlags = 0,
            UINT sampleCount = 1,
            UINT sampleQuality = 0,
            UINT miscFlags = 0) : CD3D11_TEXTURE2D_DESC(format, width, height, arraySize, mipLevels, bindFlags, usage, cpuAccessFlags, sampleCount, sampleQuality, miscFlags) {}
    };

    struct DEPTH_STENCIL_TEXTURE_DESC : CD3D11_TEXTURE2D_DESC
    {
        DEPTH_STENCIL_TEXTURE_DESC(
            DXGI_FORMAT format, UINT width, UINT height,
            UINT arraySize = 1, UINT mipLevels = 1,
            UINT bindFlags = D3D11_BIND_DEPTH_STENCIL,
            D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
            UINT cpuAccessFlags = 0,
            UINT sampleCount = 1,
            UINT sampleQuality = 0,
            UINT miscFlags = 0) : CD3D11_TEXTURE2D_DESC(format, width, height, arraySize, mipLevels, bindFlags, usage, cpuAccessFlags, sampleCount, sampleQuality, miscFlags) {}
    };

    struct TextureContext
    {
        const xtw::com_ptr<ID3D11DeviceContext> DeviceContext;
        explicit TextureContext(xtw::com_ptr<ID3D11DeviceContext> ctx) : DeviceContext(std::move(ctx)) {}

        void UpdateTexture(ID3D11Texture2D* texture, UINT subresource, const RECT& dst, const void* src, size_t src_pitch) const;

        ByteSpan1d MapTexture(D3D11_MAP map, ID3D11Texture1D* texture, UINT subresource = 0) const;
        ByteSpan2d MapTexture(D3D11_MAP map, ID3D11Texture2D* texture, UINT subresource = 0) const;
        ByteSpan3d MapTexture(D3D11_MAP map, ID3D11Texture3D* texture, UINT subresource = 0) const;

        void UnmapTexture(ID3D11Texture1D* texture, UINT subresource = 0) const;
        void UnmapTexture(ID3D11Texture2D* texture, UINT subresource = 0) const;
        void UnmapTexture(ID3D11Texture3D* texture, UINT subresource = 0) const;
    };
}
