/// @file
///	@brief   sandy::d3d11::Texture
///	@author  (C) 2023 ttsuki

#include "Texture.h"

#include <directxtk/WICTextureLoader.h>
#include <xtw/debug.h>

#include "Device.h"

namespace sandy::d3d11
{
    xtw::com_ptr<ID3D11ShaderResourceView> AllocateShaderResourceView(const xtw::com_ptr<ID3D11Texture1D>& texture)
    {
        return Device::FromChild(texture.get())
            .CreateShaderResourceView(
                texture.get(),
                CD3D11_SHADER_RESOURCE_VIEW_DESC{texture.get(), D3D11_SRV_DIMENSION_TEXTURE1D, TextureDescFrom(texture).Format});
    }

    xtw::com_ptr<ID3D11ShaderResourceView> AllocateShaderResourceView(const xtw::com_ptr<ID3D11Texture2D>& texture)
    {
        return Device::FromChild(texture.get())
            .CreateShaderResourceView(
                texture.get(),
                CD3D11_SHADER_RESOURCE_VIEW_DESC{texture.get(), D3D11_SRV_DIMENSION_TEXTURE2D, TextureDescFrom(texture).Format});
    }

    xtw::com_ptr<ID3D11ShaderResourceView> AllocateShaderResourceView(const xtw::com_ptr<ID3D11Texture3D>& texture)
    {
        return Device::FromChild(texture.get())
            .CreateShaderResourceView(
                texture.get(),
                CD3D11_SHADER_RESOURCE_VIEW_DESC{texture.get(), TextureDescFrom(texture).Format});
    }

    xtw::com_ptr<ID3D11Texture2D> LoadTextureFromFileInMemory(
        ID3D11Device* device,
        const void* file_image,
        size_t file_image_length,
        D3D11_USAGE usage,
        UINT bind_flag,
        UINT cpu_access_flags)
    {
        xtw::com_ptr<ID3D11Resource> res{};
        XTW_THROW_ON_FAILURE DirectX::CreateWICTextureFromMemoryEx(
            device,
            static_cast<const uint8_t*>(file_image),
            file_image_length,
            0,
            usage,
            bind_flag,
            cpu_access_flags,
            0,
            DirectX::WIC_LOADER_DEFAULT,
            res.put(),
            nullptr);

        xtw::com_ptr<ID3D11Texture2D> texture{};
        XTW_THROW_ON_FAILURE res->QueryInterface(IID_PPV_ARGS(texture.put()));

        return texture;
    }

    void TextureContext::UpdateTexture(ID3D11Texture2D* texture, UINT subresource, const RECT& dst, const void* src, size_t src_pitch) const
    {
        D3D11_BOX box{
            static_cast<UINT>(dst.left),
            static_cast<UINT>(dst.top),
            0,
            static_cast<UINT>(dst.right),
            static_cast<UINT>(dst.bottom),
            1
        };
        DeviceContext->UpdateSubresource(texture, subresource, &box, src, static_cast<UINT>(src_pitch), 0);
    }

    ByteSpan1d TextureContext::MapTexture(D3D11_MAP map, ID3D11Texture1D* texture, UINT subresource) const
    {
        D3D11_MAPPED_SUBRESOURCE res;
        XTW_THROW_ON_FAILURE DeviceContext->Map(texture, subresource, map, 0, &res);
        auto desc = util::GetDesc(texture);
        return {res.pData, desc.Width};
    }

    ByteSpan2d TextureContext::MapTexture(D3D11_MAP map, ID3D11Texture2D* texture, UINT subresource) const
    {
        D3D11_MAPPED_SUBRESOURCE res;
        XTW_THROW_ON_FAILURE DeviceContext->Map(texture, subresource, map, 0, &res);
        auto desc = util::GetDesc(texture);
        return {res.pData, desc.Width, desc.Height, res.RowPitch};
    }

    ByteSpan3d TextureContext::MapTexture(D3D11_MAP map, ID3D11Texture3D* texture, UINT subresource) const
    {
        D3D11_MAPPED_SUBRESOURCE res;
        XTW_THROW_ON_FAILURE DeviceContext->Map(texture, subresource, map, 0, &res);
        auto desc = util::GetDesc(texture);
        return {res.pData, desc.Width, desc.Height, desc.Depth, res.RowPitch, res.DepthPitch};
    }

    void TextureContext::UnmapTexture(ID3D11Texture1D* texture, UINT subresource) const { DeviceContext->Unmap(texture, subresource); }
    void TextureContext::UnmapTexture(ID3D11Texture2D* texture, UINT subresource) const { DeviceContext->Unmap(texture, subresource); }
    void TextureContext::UnmapTexture(ID3D11Texture3D* texture, UINT subresource) const { DeviceContext->Unmap(texture, subresource); }
}
