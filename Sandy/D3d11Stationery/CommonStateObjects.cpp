/// @file
///	@brief   sandy::d3d11::stationery
///	@author  (C) 2023 ttsuki

#include "./CommonStateObjects.h"

namespace sandy::d3d11
{
    static xtw::com_ptr<ID3D11ShaderResourceView> CreateTextureDirect(const Device& factory, unsigned int width, unsigned int height, const uint32_t bitmap[])
    {
        constexpr auto fmt = DXGI_FORMAT_B8G8R8A8_UNORM;
        D3D11_SUBRESOURCE_DATA resource = {bitmap, 4 * width, 4 * width * height};
        auto tex = factory.CreateTexture2D(CD3D11_TEXTURE2D_DESC(fmt, width, height, 1, 1), &resource);
        return factory.CreateShaderResourceView(tex.get(), CD3D11_SHADER_RESOURCE_VIEW_DESC(tex.get(), D3D11_SRV_DIMENSION_TEXTURE2D, fmt));
    }

    static xtw::com_ptr<ID3D11ShaderResourceView> Create1x1Texture(const Device& factory, uint32_t color)
    {
        return CreateTextureDirect(factory, 1, 1, &color);
    }

    static constexpr uint32_t test_pattern_bmp[3 * 3]
    {
        0xFFFF0000u, 0xFFFFFF00u, 0xFF00FF00u,
        0xFFFF00FFu, 0xFF00FFFFu, 0xFF000000u,
        0xFF0000FFu, 0xFFFFFFFFu, 0x00FFFFFFu,
    };

    CommonTextures::CommonTextures(const Device& factory)
        : White(Create1x1Texture(factory, 0xFFFFFFFF))
        , Black(Create1x1Texture(factory, 0xFF000000))
        , Transparent(Create1x1Texture(factory, 0x00000000))
        , TestPattern(CreateTextureDirect(factory, 3, 3, test_pattern_bmp))
    {
        //
    }
}
