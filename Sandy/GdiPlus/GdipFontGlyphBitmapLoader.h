/// @file
///	@brief   sandy::gdip FontBitmapLoader
///	@author  (C) 2023 ttsuki

#pragma once
#include <Windows.h>

#include <memory>
#include <string_view>

namespace sandy::gdip
{
    void GdipStartup();
    void GdipShutdown();

    struct FontMetrics
    {
        float Ascent;
        float Descent;
        float LineSpacing;
    };

    enum struct FontStyle
    {
        Regular = 0 /* Gdiplus::FontStyleRegular */,
        Bold = 1 /* Gdiplus::FontStyleBold */,
        Italic = 2 /* Gdiplus::FontStyleItalic */,
        BoldItalic = 3 /* Gdiplus::FontStyleBoldItalic */,
    };

    class FontDesc;
    using FontHandle = std::shared_ptr<FontDesc>;

    FontHandle CreateFontFromFile(
        const void* font_file_data,
        size_t font_file_length,
        const wchar_t* name,
        FontStyle style = FontStyle::Regular,
        bool need_to_normalize_path = false);

    FontHandle CreateFontFromSystem(
        const wchar_t* name,
        FontStyle style = FontStyle::Regular,
        bool need_to_normalize_path = false);

    class GdipBitmap;

    class FontGlyphBitmapLoader
    {
        class impl;
        std::unique_ptr<impl> impl_;

    public:
        FontGlyphBitmapLoader();
        FontGlyphBitmapLoader(const FontGlyphBitmapLoader& other) = delete;
        FontGlyphBitmapLoader(FontGlyphBitmapLoader&& other) noexcept = delete;
        FontGlyphBitmapLoader& operator=(const FontGlyphBitmapLoader& other) = delete;
        FontGlyphBitmapLoader& operator=(FontGlyphBitmapLoader&& other) noexcept = delete;
        ~FontGlyphBitmapLoader();

        struct LoadedBitmap
        {
            GdipBitmap* Buffer;
            RECT SourceBlackBox;
            RECT DestinationBlackBox;
            SIZE CellIncrement;
        };

        enum struct LineJoin
        {
            Miter = 0 /* Gdiplus::LineJoin::LineJoinMiter */,
            Bevel = 1 /* Gdiplus::LineJoin::LineJoinBevel */,
            Round = 2 /* Gdiplus::LineJoin::LineJoinRound */,
            MiterClipped = 3 /* Gdiplus::LineJoin::LineJoinMiterClipped */,
        };

        [[nodiscard]] LoadedBitmap LoadFontGlyphBitmap(
            FontHandle font,
            float font_size_em,
            const std::wstring_view& text,
            float widen = 0,
            LineJoin join = LineJoin::MiterClipped);

        [[nodiscard]] FontMetrics GetFontMetricFromDesc(
            FontHandle font,
            float font_size_em);
    };

    void BitBlt32bppArgb(
        GdipBitmap* src_bitmap,
        const RECT& src_rect,
        void* dst_bitmap,
        size_t dst_pitch,
        const POINT& dst_pos,
        const RECT& dst_clip);

    void BitBlt8bppAlpha(
        GdipBitmap* src_bitmap,
        const RECT& src_rect,
        void* dst_bitmap,
        size_t dst_pitch,
        const POINT& dst_pos,
        const RECT& dst_clip);
}
