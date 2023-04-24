/// @file
///	@brief   sandy::d3d11::stationery
///	@author  (C) 2023 ttsuki

#include "DynamicTextureAtlas.h"

#include "../D3d11/Device.h"
#include "../D3d11/Texture.h"
#include "../misc/Span.h"

namespace sandy::d3d11
{
    DynamicTextureAtlas::DynamicTextureAtlas(ID3D11Device* device, SIZE texture_size, DXGI_FORMAT format, PackingAlgorithm algorithm)
        : Atlas(Device(device).CreateTexture2D(DYNAMIC_TEXTURE_DESC{format, static_cast<UINT>(texture_size.cx), static_cast<UINT>(texture_size.cy)}))
        , AtlasDesc(TextureDescFrom(Atlas))
        , AtlasShaderResourceView(AllocateShaderResourceView(Atlas))
        , algorithm_(std::move(algorithm))
        , staging_(Device(device).CreateTexture2D(STAGING_TEXTURE_DESC{format, static_cast<UINT>(texture_size.cx), static_cast<UINT>(texture_size.cy)}))
        , dirty_()
    {
        algorithm_.Reset(texture_size);
    }

    void DynamicTextureAtlas::Clear()
    {
        algorithm_.Reset({static_cast<LONG>(AtlasDesc.Width), static_cast<LONG>(AtlasDesc.Height)});
    }

    std::optional<DynamicTextureAtlas::AllocatedRect> DynamicTextureAtlas::AllocateRect(ID3D11DeviceContext* context, SIZE size)
    {
        std::optional<RECT> allocated = algorithm_.Reserve(size);
        if (!allocated)
        {
            return std::nullopt;
        }

        // if not mapped yet, map.
        if (!dirty_)
        {
            mapped_ = TextureContext(context).MapTexture(D3D11_MAP_WRITE, staging_.get(), 0);
        }

        // update dirty rectangle.
        RECT d = dirty_ ? *dirty_ : *allocated;
        d.left = std::min<LONG>(d.left, allocated->left);
        d.top = std::min<LONG>(d.top, allocated->top);
        d.right = std::max<LONG>(d.right, allocated->right);
        d.bottom = std::max<LONG>(d.bottom, allocated->bottom);
        dirty_ = d;

        return AllocatedRect{*allocated, mapped_};
    }

    void DynamicTextureAtlas::Commit(ID3D11DeviceContext* context)
    {
        if (dirty_)
        {
            context->Unmap(staging_.get(), 0);

            D3D11_BOX box{
                static_cast<UINT>(dirty_->left), static_cast<UINT>(dirty_->top), 0,
                static_cast<UINT>(dirty_->right), static_cast<UINT>(dirty_->bottom), 1
            };

            context->CopySubresourceRegion(
                Atlas.get(), 0,
                dirty_->left, dirty_->top, 0,
                staging_.get(), 0, &box);

            dirty_ = std::nullopt; // unmapped
        }
    }
}
