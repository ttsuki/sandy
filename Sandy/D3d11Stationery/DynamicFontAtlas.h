/// @file
///	@brief   TTsukiGameSDK::DynamicFontAtlas
///	@author  (C) 2023 ttsuki


#pragma once
#include <Windows.h>
#include <combaseapi.h>
#include <d3d11.h>

#include <xtw/com.h>
#include <memory>
#include <map>
#include <unordered_map>

#include "../misc/Math.h"
#include "../GdiPlus/GdipFontGlyphBitmapLoader.h"

#include "DynamicTextureAtlas.h"

#include "BasicPrimitiveBatch.h"

namespace sandy::d3d11::font
{
    using FontID = uint8_t;

    enum struct FontStyle
    {
        Regular = gdip::FontStyle::Regular,
        Bold = gdip::FontStyle::Bold,
        Italic = gdip::FontStyle::Italic,
        BoldItalic = gdip::FontStyle::BoldItalic,
    };

    using FontMetrics = gdip::FontMetrics;

    class DynamicFontAtlas
    {
    public:
        struct FontDef
        {
            FontID id;
            uint8_t weight;
            uint16_t size;
        };

        struct TextureChipDefinition
        {
            Vec2 destination_lt; // vertex
            Vec2 destination_rb; // vertex
            Vec2 source_lt;      // uv
            Vec2 source_rb;      // uv
            Vec2 cell_increment;
        };

    private:
        struct chip_def_t;
        using chip_key_t = uint64_t;
        std::array<std::shared_ptr<gdip::FontDesc>, 256> loaded_fonts_{};
        std::unique_ptr<gdip::FontGlyphBitmapLoader> glyph_loader_{};
        std::unique_ptr<DynamicTextureAtlas> atlas_{};
        std::unordered_map<chip_key_t, TextureChipDefinition> atlas_map_{};
        Vec2 atlas_size_{};
        std::unordered_map<std::wstring, uint32_t> dictionary_{};

    public:
        [[nodiscard]] DynamicTextureAtlas* Atlas() const { return atlas_.get(); }

        // ctor
        DynamicFontAtlas(ID3D11Device* device, int width, int height);

        // clear atlas
        void ClearAtlas();

        // load font
        void LoadFontFromSystem(
            FontID font_id,
            const wchar_t* family_name,
            FontStyle style = FontStyle::Regular,
            bool need_to_normalize_path = false);

        // load font
        void LoadFontFromFile(
            FontID font_id,
            const void* font_file_image,
            size_t font_file_image_length,
            const wchar_t* family_name,
            FontStyle style = FontStyle::Regular,
            bool need_to_normalize_path = false);

        // load font metric
        FontMetrics LoadMetric(FontDef font);

        // load character
        TextureChipDefinition LoadCharacterSequence(ID3D11DeviceContext* context, FontDef font, std::wstring_view text);

        // commit
        void CommitAtlasTexture(ID3D11DeviceContext* context);
    };

    struct ColorSet
    {
        Color4 colors[4]{};
        ColorSet(Color4 color0123 = colors::White) : ColorSet{color0123, color0123, color0123, color0123} {}
        ColorSet(Color4 color0, Color4 color3) : ColorSet{color0, leap(color0, color3, 0.5f), color3} {}
        ColorSet(Color4 color0, Color4 color12, Color4 color3) : ColorSet{color0, color12, color12, color3} {}
        ColorSet(Color4 color0, Color4 color1, Color4 color2, Color4 color3) : colors{color0, color1, color2, color3} {}
    };

    class FontPrimitiveBuilder
    {
    public:
        struct TextDecoration
        {
            FontID font_id = 0;
            uint16_t size = 16;
            uint8_t weight = 0;
            ColorSet color{colors::White};
            wchar_t ligature_trigger{L'#'}; // treat `#{...}` as ligature
        };

    private:
        struct Command
        {
            std::wstring text;
            TextDecoration deco;
        };

        std::vector<Command> buffer_{};

    public:
        FontPrimitiveBuilder() = default;
        void Clear() { buffer_.clear(); }
        void AppendText(std::wstring text, TextDecoration deco) { buffer_.push_back({std::move(text), deco}); }

        [[nodiscard]] std::vector<PositionColoredTextured> Build(ID3D11DeviceContext* context, DynamicFontAtlas* atlas) const;
    };
}
