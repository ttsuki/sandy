/// @file
///	@brief   sandy::d3d11::stationery
///	@author  (C) 2023 ttsuki

#pragma once
#include <Windows.h>
#include <combaseapi.h>
#include <d3d11.h>

#include <xtw/com.h>
#include <cstddef>
#include <optional>

#include "../D3d11/Texture.h"
#include "../misc/Span.h"

namespace sandy::d3d11
{
    class IAtlasAlgorithm
    {
    public:
        IAtlasAlgorithm() = default;
        IAtlasAlgorithm(const IAtlasAlgorithm& other) = default;
        IAtlasAlgorithm(IAtlasAlgorithm&& other) noexcept = default;
        IAtlasAlgorithm& operator=(const IAtlasAlgorithm& other) = default;
        IAtlasAlgorithm& operator=(IAtlasAlgorithm&& other) noexcept = default;
        virtual ~IAtlasAlgorithm() = default;

        virtual void Reset(SIZE atlas_size) = 0;
        [[nodiscard]] virtual std::optional<RECT> Reserve(SIZE size) = 0;
    };

    class SimpleAtlasAlgorithm final : public IAtlasAlgorithm
    {
        SIZE texture_size_{};
        LONG current_cy_{};
        POINT cursor_{};

    public:
        SimpleAtlasAlgorithm() = default;

        void Reset(SIZE atlas_size) override
        {
            texture_size_ = atlas_size;
            current_cy_ = 0;
            cursor_ = {};
        }

        [[nodiscard]] std::optional<RECT> Reserve(SIZE size) override
        {
            if (cursor_.x + size.cx < texture_size_.cx && cursor_.y + size.cy < texture_size_.cy)
            {
                // ok
            }
            else if (size.cx < texture_size_.cx && cursor_.y + current_cy_ + size.cy < texture_size_.cy)
            {
                cursor_ = POINT{0, cursor_.y + current_cy_};
                current_cy_ = 0;
            }
            else
            {
                return std::nullopt;
            }

            auto result = RECT{cursor_.x, cursor_.y, cursor_.x + size.cx, cursor_.y + size.cy};
            cursor_.x += size.cx + 1;
            current_cy_ = std::max<LONG>(current_cy_, size.cy + 1);
            return result;
        }
    };

    class DynamicTextureAtlas
    {
    public:
        using PackingAlgorithm = SimpleAtlasAlgorithm;
        const xtw::com_ptr<ID3D11Texture2D> Atlas;
        const D3D11_TEXTURE2D_DESC AtlasDesc;
        const xtw::com_ptr<ID3D11ShaderResourceView> AtlasShaderResourceView;

    private:
        PackingAlgorithm algorithm_;
        const xtw::com_ptr<ID3D11Texture2D> staging_;
        std::optional<RECT> dirty_{};
        ByteSpan2d mapped_{};

    public:
        DynamicTextureAtlas(ID3D11Device* device, SIZE texture_size, DXGI_FORMAT format, PackingAlgorithm algorithm = {});

        struct AllocatedRect
        {
            RECT rect;
            ByteSpan2d buffer;
        };

        void Clear();
        std::optional<AllocatedRect> AllocateRect(ID3D11DeviceContext* context, SIZE size);
        void Commit(ID3D11DeviceContext* context);
    };
}
