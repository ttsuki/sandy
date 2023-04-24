/// @file
///	@brief   TTsukiGameSDK::DynamicFontAtlas
///	@author  (C) 2023 ttsuki

#include "DynamicFontAtlas.h"

namespace sandy::d3d11::font
{
    struct DynamicFontAtlas::chip_def_t
    {
        FontDef font;
        uint32_t chr;

        [[nodiscard]] operator chip_key_t() const noexcept { return to_key(); }

        [[nodiscard]] chip_key_t to_key() const noexcept
        {
            static_assert(std::is_trivial_v<chip_def_t>);
            static_assert(sizeof(chip_def_t) == sizeof(chip_key_t));
            chip_key_t key;
            memcpy(&key, this, sizeof(chip_def_t));
            return key;
        }
    };

    DynamicFontAtlas::DynamicFontAtlas(ID3D11Device* device, int width, int height)
        : glyph_loader_(std::make_unique<gdip::FontGlyphBitmapLoader>())
        , atlas_(std::make_unique<DynamicTextureAtlas>(device, SIZE{width, height}, DXGI_FORMAT_B8G8R8A8_UNORM))
        , atlas_map_()
        , atlas_size_(static_cast<float>(width), static_cast<float>(height), 1.0f, 1.0f)
    {
        //
    }

    void DynamicFontAtlas::ClearAtlas()
    {
        atlas_map_.clear();
    }

    void DynamicFontAtlas::LoadFontFromSystem(FontID font_id, const wchar_t* family_name, FontStyle style, bool need_to_normalize_path)
    {
        loaded_fonts_[font_id] = gdip::CreateFontFromSystem(family_name, static_cast<gdip::FontStyle>(style), need_to_normalize_path);
    }

    void DynamicFontAtlas::LoadFontFromFile(FontID font_id, const void* font_file_image, size_t font_file_image_length, const wchar_t* family_name, FontStyle style, bool need_to_normalize_path)
    {
        loaded_fonts_[font_id] = gdip::CreateFontFromFile(font_file_image, font_file_image_length, family_name, static_cast<gdip::FontStyle>(style), need_to_normalize_path);
    }

    FontMetrics DynamicFontAtlas::LoadMetric(FontDef font)
    {
        const auto font_desc = loaded_fonts_.at(font.id);
        if (!font_desc) throw std::invalid_argument("font is not loaded");

        return glyph_loader_->GetFontMetricFromDesc(font_desc, static_cast<float>(font.size));
    }

    DynamicFontAtlas::TextureChipDefinition DynamicFontAtlas::LoadCharacterSequence(ID3D11DeviceContext* context, FontDef font, std::wstring_view text)
    {
        const auto font_desc = loaded_fonts_.at(font.id);
        if (!font_desc) throw std::invalid_argument("font is not loaded");

        if (text.length() == 0) return {};

        chip_def_t key = {font, static_cast<uint16_t>(text[0])};

        if (text.length() != 1)
        {
            auto dic_key = std::wstring(text);
            auto it = dictionary_.find(dic_key);
            if (it == dictionary_.end())
                it = dictionary_.emplace(std::move(dic_key), static_cast<uint32_t>(dictionary_.size())).first;
            key.chr = it->second + 65536;
        }

        auto it = atlas_map_.find(key);
        if (it == atlas_map_.end())
        {
            auto bitmap = glyph_loader_->LoadFontGlyphBitmap(font_desc, font.size, text, font.weight);
            if (auto allocated = atlas_->AllocateRect(context, SIZE{bitmap.SourceBlackBox.right - bitmap.SourceBlackBox.left, bitmap.SourceBlackBox.bottom - bitmap.SourceBlackBox.top}))
            {
                gdip::BitBlt32bppArgb(
                    bitmap.Buffer,
                    bitmap.SourceBlackBox,
                    allocated->buffer.pointer,
                    allocated->buffer.width_pitch,
                    POINT{allocated->rect.left, allocated->rect.top},
                    allocated->rect);

                it = atlas_map_.insert_or_assign(
                    key,
                    TextureChipDefinition{
                        Vec2{static_cast<float>(bitmap.DestinationBlackBox.left), static_cast<float>(bitmap.DestinationBlackBox.top)},
                        Vec2{static_cast<float>(bitmap.DestinationBlackBox.right), static_cast<float>(bitmap.DestinationBlackBox.bottom)},
                        Vec2{static_cast<float>(allocated->rect.left), static_cast<float>(allocated->rect.top)} / atlas_size_,
                        Vec2{static_cast<float>(allocated->rect.right), static_cast<float>(allocated->rect.bottom)} / atlas_size_,
                        Vec2{static_cast<float>(bitmap.CellIncrement.cx), static_cast<float>(bitmap.CellIncrement.cy)},
                    }).first;
            }
        }

        return it->second;
    }

    void DynamicFontAtlas::CommitAtlasTexture(ID3D11DeviceContext* context)
    {
        atlas_->Commit(context);
    }

    std::vector<PositionColoredTextured> FontPrimitiveBuilder::Build(ID3D11DeviceContext* context, DynamicFontAtlas* atlas) const
    {
        std::vector<PositionColoredTextured> result;
        {
            size_t total_count = 0;
            for (const auto& command : buffer_)
                total_count += command.text.size();
            result.reserve(total_count * 10);
        }

        Vec2 cursor = {};

        for (const auto& command : buffer_)
        {
            const auto& deco = command.deco;
            const auto& text = std::wstring_view(command.text);
            const auto count = text.size();
            auto font_def = DynamicFontAtlas::FontDef{deco.font_id, deco.weight, deco.size};

            auto metric = atlas->LoadMetric(font_def);

            for (size_t i = 0; i < count; ++i)
            {
                auto chr = std::wstring_view(&text[i], 1);

                // treat `#{...}` as ligature
                if (text[i] == deco.ligature_trigger && text.length() > i + 1 && text[i + 1] == '{')
                {
                    for (size_t s = i + 2, j = s; j < count; j++)
                    {
                        if (text[j] == '}')
                        {
                            chr = text.substr(s, j - s);
                            i = j;
                            break;
                        }
                    }
                }

                auto chip = atlas->LoadCharacterSequence(context, font_def, chr);
                Vec2 dest_lt = chip.destination_lt;
                Vec2 dest_rt = arkxmm::insert_element<0, 0>(chip.destination_lt.v, chip.destination_rb.v);
                Vec2 dest_lb = arkxmm::insert_element<1, 1>(chip.destination_lt.v, chip.destination_rb.v);
                Vec2 dest_rb = chip.destination_rb;
                Vec2 source_lt = chip.source_lt;
                Vec2 source_rt = arkxmm::insert_element<0, 0>(chip.source_lt.v, chip.source_rb.v);
                Vec2 source_lb = arkxmm::insert_element<1, 1>(chip.source_lt.v, chip.source_rb.v);
                Vec2 source_rb = chip.source_rb;
                Vec2 cell_inc = Vec2(chip.cell_increment.x(), 0.0f);

                PositionColoredTextured v[10]{
                    {cursor + dest_lt, deco.color.colors[0], colors::Transparent, source_lt},
                    {cursor + dest_lt, deco.color.colors[0], colors::Transparent, source_lt},
                    {cursor + dest_rt, deco.color.colors[0], colors::Transparent, source_rt},
                    {cursor + (dest_lt + dest_lb) / 2, deco.color.colors[1], colors::Transparent, (source_lt + source_lb) / 2},
                    {cursor + (dest_rt + dest_rb) / 2, deco.color.colors[1], colors::Transparent, (source_rt + source_rb) / 2},
                    {cursor + (dest_lt + dest_lb) / 2, deco.color.colors[2], colors::Transparent, (source_lt + source_lb) / 2},
                    {cursor + (dest_rt + dest_rb) / 2, deco.color.colors[2], colors::Transparent, (source_rt + source_rb) / 2},
                    {cursor + dest_lb, deco.color.colors[3], colors::Transparent, source_lb},
                    {cursor + dest_rb, deco.color.colors[3], colors::Transparent, source_rb},
                    {cursor + dest_rb, deco.color.colors[3], colors::Transparent, source_rb},
                };

                if (text[i] == '\n')
                {
                    cursor = Vec2{0, cursor.y() + metric.LineSpacing};
                }
                else
                {
                    result.insert(result.end(), std::begin(v), std::end(v));
                    cursor += cell_inc;
                }
            }
        }

        atlas->CommitAtlasTexture(context);
        return result;
    }
}
