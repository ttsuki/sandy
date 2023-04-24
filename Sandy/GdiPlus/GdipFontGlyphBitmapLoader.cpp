/// @file
///	@brief   sandy::gdip FontGlyphBitmapLoader
///	@author  (C) 2023 ttsuki

#include "GdipFontGlyphBitmapLoader.h"

#include <Windows.h>
#include <combaseapi.h>

#include <sdkddkver.h> // Windows SDK Version (NTDDI_*)
#ifdef NTDDI_WIN10_FE // Windows SDK 10.0.22000 or later
#  include <gdiplus.h>
#else // fix up platform header
#  if defined(NOMINMAX)
#    define min(a,b) (std::min)((a),(b))
#    define max(a,b) (std::max)((a),(b))
#  endif
#  pragma warning(push)
#  pragma warning(disable: 4458)
#  include <gdiplus.h>
#  pragma warning(pop)
#  if defined NOMINMAX
#    undef min
#    undef max
#  endif
#endif

#pragma comment(lib, "gdiplus.lib")

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <utility>

#include <xtw/debug.h>

#define GDIP_THROW_ON_FAILURE (::xtw::debug::percent_operator_redirection([](::Gdiplus::Status r) { if (r != ::Gdiplus::Status::Ok) throw ::std::runtime_error("gdiplus error: " + ::std::to_string((int)r)); }))%

namespace sandy::gdip
{
    template <class T>
    struct span_t
    {
        T* const beg_;
        T* const end_;
        [[nodiscard]] auto begin() const noexcept { return beg_; }
        [[nodiscard]] auto end() const noexcept { return end_; }
        [[nodiscard]] size_t size() const noexcept { return end_ - beg_; }
        [[nodiscard]] T& operator [](size_t idx) const noexcept { return *(beg_ + idx); }
    };

    static ULONG_PTR gdi_plus_token{};

    void GdipStartup()
    {
        Gdiplus::GdiplusStartupInput i{};
        Gdiplus::GdiplusStartupOutput o{};
        GDIP_THROW_ON_FAILURE Gdiplus::GdiplusStartup(&gdi_plus_token, &i, &o);
    }

    void GdipShutdown()
    {
        Gdiplus::GdiplusShutdown(gdi_plus_token);
    }

#pragma region Path Manipulation API

    struct PathView
    {
        const size_t Count;
        const span_t<Gdiplus::PointF> Points;
        const span_t<BYTE> Types;
        const Gdiplus::RectF BoundingBox;

        PathView(const Gdiplus::PathData& data, int start, int length, bool calculate_bounding_box = false)
            : Count(length)
            , Points{data.Points + start, data.Points + start + length}
            , Types{data.Types + start, data.Types + start + length}
            , BoundingBox{calculate_bounding_box ? GetBounds(Points.beg_, Points.end_ - Points.beg_) : Gdiplus::RectF{}}
        {
            //   
        }

        static Gdiplus::RectF GetBounds(const Gdiplus::PointF points[], size_t count)
        {
            float min_x = std::numeric_limits<float>::max();
            float min_y = std::numeric_limits<float>::max();
            float max_x = std::numeric_limits<float>::lowest();
            float max_y = std::numeric_limits<float>::lowest();

            for (size_t i = 0; i < count; ++i)
            {
                min_x = std::min(points[i].X, min_x);
                min_y = std::min(points[i].Y, min_y);
                max_x = std::max(points[i].X, max_x);
                max_y = std::max(points[i].Y, max_y);
            }

            return Gdiplus::RectF{min_x, min_y, max_x - min_x, max_y - min_y};
        }
    };

    /// Split a path to subpaths.
    static std::vector<PathView> SplitToSubpath(const Gdiplus::PathData& path, bool withBoundingBox = false)
    {
        std::vector<PathView> result;

        auto s = 0;
        auto c = 0;
        for (auto type : span_t<BYTE>{path.Types, path.Types + path.Count})
        {
            c++;
            if ((type & 128) != 0)
            {
                result.emplace_back(path, s, c, withBoundingBox);
                s += c;
                c = 0;
            }
        }

        return result;
    }

    enum struct PathDirection
    {
        Clockwise = 1,
        Unknown = 0,
        CounterClockwise = -1
    };

    static PathDirection GetPathDirection(const PathView& path, const std::vector<PathView>& all_paths)
    {
        struct Impl
        {
            /// サブパスの回転方向を求める。
            static PathDirection GetPathDirection(const PathView& path, const std::vector<PathView>& all_paths, size_t depth = 4)
            {
                // パスの各辺について、辺の法線ベクトル方向にちょっとだけ離れた点が
                // Pathの内側なのか外側なのかを求めることで、パスの方向を求める。
                int score = 0;

                const auto step = std::max<size_t>(path.Count / depth, 1);
                const auto splits_per_edge = path.Count < 8 ? 4 : 2;

                for (size_t i = 0; i < path.Count; i += step)
                {
                    auto p1 = path.Points[i];                    // 辺の始点
                    auto p2 = path.Points[(i + 1) % path.Count]; // 辺の終点

                    float dx = (p2.X - p1.X) / static_cast<float>(splits_per_edge);
                    float dy = (p2.Y - p1.Y) / static_cast<float>(splits_per_edge);
                    for (auto j = 1; j < splits_per_edge; j++)
                    {
                        float jx = dx * static_cast<float>(j);
                        float jy = dy * static_cast<float>(j);

                        // normal
                        float nx = -dy * 0.125f;
                        float ny = dx * 0.125f;

                        // clockwise
                        if (PointIsPainted(Gdiplus::PointF(p1.X + jx + nx, p1.Y + jy + ny), all_paths)) score++;

                        // counter clockwise
                        if (PointIsPainted(Gdiplus::PointF(p1.X + jx - nx, p1.Y + jy - ny), all_paths)) score--;
                    }
                }

                return score
                           ? (score > 0 ? PathDirection::Clockwise : PathDirection::CounterClockwise)
                           : (depth < 256 ? GetPathDirection(path, all_paths, depth * 4) : PathDirection::Unknown);
            }

            /// p が allPaths において塗られているか？
            static bool PointIsPainted(const Gdiplus::PointF& p, const std::vector<PathView>& allPaths)
            {
                auto c = 0;
                for (auto&& path : allPaths)
                {
                    if (path.BoundingBox.IsEmptyArea() || path.BoundingBox.Contains(p))
                    {
                        auto prev = *(path.Points.end() - 1);
                        for (auto&& p1 : path.Points)
                        {
                            c += IntersectsHorizontalHalfLineVsLineSeg(p, prev, p1);
                            prev = p1;
                        }
                    }
                }

                return c != 0;
            }

            /// p0からX軸方向に伸ばした半直線と、 線分(p1, p2)との交差方向判定を行う。
            /// 下向きに交わる場合正の値, 上向きに交わる場合負の値, 交差しない場合0が返る。
            static int IntersectsHorizontalHalfLineVsLineSeg(
                const Gdiplus::PointF& p0,
                const Gdiplus::PointF& p1,
                const Gdiplus::PointF& p2)
            {
                float x0 = p0.X, x1 = p1.X, x2 = p2.X;
                float y0 = p0.Y, y1 = p1.Y, y2 = p2.Y;

                if (sign(x2 - x0) <= 0 && sign(x1 - x0) <= 0) return 0;
                if (sign(y2 - y0) < 0 && sign(y1 - y0) <= 0) return 0;
                if (sign(y2 - y0) > 0 && sign(y1 - y0) >= 0) return 0;

                // 少なくとも片方がp0より右側にあり、x = x0 の直線とは交差する。
                auto dir = sign(y2 - y1); // 方向は？
                if (dir == 0) return 0;   // x軸と平行
                if (dir == 1 && sign(y1 - y0) >= 0) return 0;
                if (dir == -1 && sign(y2 - y0) >= 0) return 0;
                if (sign(x2 - x0) > 0 && sign(x1 - x0) > 0) return dir; // 両方右側にある

                // p0を通る直線 x = x0 と、p1とp2を通る直線のとの交点が、x0より狭右にあれば交差する。
                float outer_product = (x2 - x1) * (y0 - y1) - (y2 - y1) * (x0 - x1);
                if (sign(outer_product) * dir > 0) return dir;

                // 交点はない。
                return 0;
            }

            /// eps付き符号判定
            static int sign(float f)
            {
                constexpr float eps = REAL_EPSILON;
                return f < -eps ? -1 : f > eps ? 1 : 0;
            }
        };

        return Impl::GetPathDirection(path, all_paths);
    }

    /// Create outlined font path
    static std::unique_ptr<Gdiplus::GraphicsPath> CreateFontOutlinePath(
        const std::wstring_view& text,
        const Gdiplus::FontFamily* fontFamily,
        const Gdiplus::FontStyle& fontStyle,
        const float& emSizeInPixel,
        const Gdiplus::Point& origin,
        const Gdiplus::StringFormat* stringFormat,
        const Gdiplus::Pen* widenPen,
        bool normalizeDirection)
    {
        float flatness = 0.025f;

        auto srcPath = std::make_unique<Gdiplus::GraphicsPath>(Gdiplus::FillMode::FillModeWinding);

        GDIP_THROW_ON_FAILURE srcPath->AddString(
            text.data(),
            static_cast<int>(text.size()),
            fontFamily, fontStyle, emSizeInPixel, origin, stringFormat);

        if (!normalizeDirection && !widenPen)
        {
            return srcPath;
        }

        Gdiplus::PathData src_path_data;
        GDIP_THROW_ON_FAILURE srcPath->GetPathData(&src_path_data);
        if (src_path_data.Count == 0)
            return srcPath;

        std::vector<PathDirection> directions;
        if (normalizeDirection)
        {
            const auto allPaths = SplitToSubpath(src_path_data, true);
            for (auto&& p : allPaths)
                directions.push_back(GetPathDirection(p, allPaths));
        }

        if (widenPen == nullptr)
        {
            GDIP_THROW_ON_FAILURE srcPath->Flatten(nullptr, flatness);

            Gdiplus::PathData pathData;
            GDIP_THROW_ON_FAILURE srcPath->GetPathData(&pathData);

            auto allPaths = SplitToSubpath(pathData);

            for (size_t i = 0; i < directions.size(); i++)
                if (directions[i] == PathDirection::CounterClockwise)
                    std::reverse(allPaths[i].Points.begin(), allPaths[i].Points.end());

            return std::make_unique<Gdiplus::GraphicsPath>(
                pathData.Points,
                pathData.Types,
                pathData.Count,
                srcPath->GetFillMode());
        }
        else
        {
            GDIP_THROW_ON_FAILURE srcPath->Widen(widenPen, nullptr, flatness);

            Gdiplus::PathData pathData;
            GDIP_THROW_ON_FAILURE srcPath->GetPathData(&pathData);
            auto widenPaths = SplitToSubpath(pathData);

            // make the outer path
            std::vector<Gdiplus::PointF> selectedPoints;
            std::vector<BYTE> selectedTypes;
            for (size_t i = 0; i < widenPaths.size() / 2; i++)
            {
                auto d = normalizeDirection && directions[i] == PathDirection::CounterClockwise ? 1 : 0;
                auto p = widenPaths[i * 2 + d];
                std::copy(p.Types.begin(), p.Types.end(), std::back_insert_iterator(selectedTypes));
                std::copy(p.Points.begin(), p.Points.end(), std::back_insert_iterator(selectedPoints));
            }

            return std::make_unique<Gdiplus::GraphicsPath>(
                selectedPoints.data(),
                selectedTypes.data(),
                static_cast<int>(selectedPoints.size()),
                srcPath->GetFillMode());
        }
    }

#pragma endregion

    static FontMetrics FontMetricsFromFamily(const Gdiplus::FontFamily* family, Gdiplus::FontStyle style, float em_height)
    {
        auto unit = em_height / static_cast<float>(family->GetEmHeight(style));
        return FontMetrics
        {
            static_cast<float>(family->GetCellAscent(style)) * unit,
            static_cast<float>(family->GetCellDescent(style)) * unit,
            static_cast<float>(family->GetLineSpacing(style)) * unit
        };
    }

    class FontDesc
    {
        std::unique_ptr<Gdiplus::FontFamily> family_{};
        Gdiplus::FontStyle style_{};
        bool need_to_normalize_path_{};
        Gdiplus::Font font_;

    public:
        FontDesc(const Gdiplus::FontFamily* family, Gdiplus::FontStyle style = Gdiplus::FontStyleRegular, bool need_to_normalize_path = false)
            : family_(family->Clone())
            , style_(style)
            , need_to_normalize_path_(need_to_normalize_path)
            , font_(family_.get(), 256.0f, style, Gdiplus::UnitPixel)
        {
            GDIP_THROW_ON_FAILURE font_.GetLastStatus();
        }

        [[nodiscard]] const Gdiplus::FontFamily* Family() const { return family_.get(); }
        [[nodiscard]] Gdiplus::FontStyle Style() const { return style_; }
        [[nodiscard]] bool NeedToNormalizePath() const { return need_to_normalize_path_; }
        [[nodiscard]] FontMetrics Metric(float em_height) const { return FontMetricsFromFamily(family_.get(), style_, em_height); }
        [[nodiscard]] const Gdiplus::Font* Font() const { return &font_; }
    };

    std::shared_ptr<FontDesc> CreateFontFromFile(const void* font_file_data, size_t font_file_length, const wchar_t* name, FontStyle style, bool need_to_normalize_path)
    {
        Gdiplus::PrivateFontCollection pfc{};
        GDIP_THROW_ON_FAILURE pfc.AddMemoryFont(font_file_data, static_cast<INT>(font_file_length));
        Gdiplus::FontFamily f(name, &pfc);
        return std::make_shared<FontDesc>(&f, static_cast<Gdiplus::FontStyle>(style), need_to_normalize_path);
    }

    std::shared_ptr<FontDesc> CreateFontFromSystem(const wchar_t* name, FontStyle style, bool need_to_normalize_path)
    {
        Gdiplus::FontFamily f(name);
        return std::make_shared<FontDesc>(&f, static_cast<Gdiplus::FontStyle>(style), need_to_normalize_path);
    }

    std::shared_ptr<FontDesc> CreateFontFromFamily(const Gdiplus::FontFamily* font_family, FontStyle style, bool need_to_normalize_path)
    {
        return std::make_shared<FontDesc>(font_family, static_cast<Gdiplus::FontStyle>(style), need_to_normalize_path);
    }

    class FontGlyphBitmapLoader::impl
    {
        std::unique_ptr<Gdiplus::StringFormat> string_format_;
        std::unique_ptr<Gdiplus::Brush> white_brush_;
        Gdiplus::Rect bmp_size_;
        std::unique_ptr<Gdiplus::Bitmap> bmp_;
        std::unique_ptr<Gdiplus::Graphics> g_;

    public:
        impl()
        {
            string_format_ = std::unique_ptr<Gdiplus::StringFormat>(Gdiplus::StringFormat::GenericTypographic()->Clone());
            string_format_->SetFormatFlags(string_format_->GetFormatFlags() | Gdiplus::StringFormatFlags::StringFormatFlagsMeasureTrailingSpaces);
            white_brush_ = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color::White);
            bmp_size_ = Gdiplus::Rect{0, 0, 256, 256};
            bmp_ = std::make_unique<Gdiplus::Bitmap>(bmp_size_.Width, bmp_size_.Height, PixelFormat32bppARGB);
            g_ = std::unique_ptr<Gdiplus::Graphics>(Gdiplus::Graphics::FromImage(bmp_.get()));
        }

        [[nodiscard]] LoadedBitmap LoadFontGlyphBitmap(const FontHandle& font, float font_size_em, std::wstring_view text, float widen, LineJoin join)
        {
            Gdiplus::Point origin{32, 32};
            std::unique_ptr<Gdiplus::Pen> pen{};

            if (widen > 0)
            {
                pen = std::make_unique<Gdiplus::Pen>(white_brush_.get(), widen);
                pen->SetLineJoin(static_cast<Gdiplus::LineJoin>(join));
                pen->SetMiterLimit(1000.0f);
                origin = origin + Gdiplus::Point(static_cast<int>(widen) + 1, static_cast<int>(widen) + 1);
            }

            auto path = CreateFontOutlinePath(
                text,
                font->Family(),
                font->Style(),
                font_size_em,
                origin,
                string_format_.get(),
                pen.get(),
                font->NeedToNormalizePath());

            Gdiplus::Size cell_inc = [&]
            {
                Gdiplus::RectF box{};
                GDIP_THROW_ON_FAILURE g_->MeasureString(
                    text.data(), static_cast<INT>(text.length()),
                    font->Font(), Gdiplus::PointF{}, string_format_.get(),
                    &box);

                const float unit = font_size_em / font->Font()->GetSize();
                return Gdiplus::Size{
                    static_cast<int>(std::round(box.Width * unit)),
                    static_cast<int>(std::round(box.Height * unit))
                };
            }();


            Gdiplus::PathData path_data;
            GDIP_THROW_ON_FAILURE path->GetPathData(&path_data);
            if (path_data.Count == 0)
            {
                return LoadedBitmap{
                    reinterpret_cast<GdipBitmap*>(bmp_.get()),
                    RECT{},
                    RECT{},
                    SIZE{cell_inc.Width, cell_inc.Height}
                };
            }

            Gdiplus::RectF bounds_float{};
            GDIP_THROW_ON_FAILURE path->GetBounds(&bounds_float);

            Gdiplus::Rect src_black_box{
                static_cast<int>(std::floorf(bounds_float.X)),
                static_cast<int>(std::floorf(bounds_float.Y)),
                static_cast<int>(std::ceilf(bounds_float.Width)),
                static_cast<int>(std::ceilf(bounds_float.Height))
            };

            if (!bmp_size_.Contains(src_black_box))
            {
                // resize buffer
                {
                    Gdiplus::Rect size = bmp_size_;
                    while (size.Width < src_black_box.Width) size.Width *= 2;
                    while (size.Height < src_black_box.Height) size.Height *= 2;
                    if (!size.Equals(bmp_size_))
                    {
                        bmp_size_ = size;
                        g_.reset();
                        bmp_ = std::make_unique<Gdiplus::Bitmap>(bmp_size_.Width, bmp_size_.Height, PixelFormat32bppARGB);
                        g_ = std::unique_ptr<Gdiplus::Graphics>(Gdiplus::Graphics::FromImage(bmp_.get()));
                    }
                }

                // translate
                {
                    auto translate = Gdiplus::Point(-src_black_box.X, -src_black_box.Y);
                    origin = origin + translate;
                    auto transform = Gdiplus::Matrix(1.0f, 0.0f, 0.0f, 1.0f, static_cast<float>(translate.X), static_cast<float>(translate.Y));
                    GDIP_THROW_ON_FAILURE path->Transform(&transform);
                    src_black_box.Offset(translate);
                }
            }

            Gdiplus::Rect dst_black_box{
                src_black_box.X - origin.X,
                src_black_box.Y - origin.Y,
                src_black_box.Width,
                src_black_box.Height
            };

            // render
            GDIP_THROW_ON_FAILURE g_->Clear(Gdiplus::Color::Transparent);
            GDIP_THROW_ON_FAILURE g_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            GDIP_THROW_ON_FAILURE g_->FillPath(white_brush_.get(), path.get());

            return LoadedBitmap{
                reinterpret_cast<GdipBitmap*>(bmp_.get()),
                RECT{src_black_box.GetLeft(), src_black_box.GetTop(), src_black_box.GetRight(), src_black_box.GetBottom()},
                RECT{dst_black_box.GetLeft(), dst_black_box.GetTop(), dst_black_box.GetRight(), dst_black_box.GetBottom()},
                SIZE{cell_inc.Width, cell_inc.Height}
            };
        }

        [[nodiscard]] FontMetrics GetFontMetricFromDesc(const FontHandle& font, float font_size_em)
        {
            return font->Metric(font_size_em);
        }
    };

    FontGlyphBitmapLoader::FontGlyphBitmapLoader() : impl_(std::make_unique<impl>()) { }
    FontGlyphBitmapLoader::~FontGlyphBitmapLoader() = default;

    /// Load bitmap
    FontGlyphBitmapLoader::LoadedBitmap FontGlyphBitmapLoader::LoadFontGlyphBitmap(
        FontHandle font, float font_size_em, const std::wstring_view& text, float widen, LineJoin join)
    {
        return impl_->LoadFontGlyphBitmap(font, font_size_em, text, widen, join);
    }

    FontMetrics FontGlyphBitmapLoader::GetFontMetricFromDesc(FontHandle font, float font_size_em)
    {
        return impl_->GetFontMetricFromDesc(font, font_size_em);
    }

    static void ClipRect(Gdiplus::Rect& src_rect, Gdiplus::Point& dst_pos, const Gdiplus::Rect& dst_clip)
    {
        if (dst_pos.X < dst_clip.X)
        {
            src_rect.X -= dst_pos.X - dst_clip.X;
            src_rect.Width -= -dst_pos.X - dst_clip.X;
            dst_pos.X = dst_clip.X;
        }

        if (dst_pos.Y < dst_clip.Y)
        {
            src_rect.Y -= dst_pos.Y - dst_clip.Y;
            src_rect.Height -= -dst_pos.Y - dst_clip.Y;
            dst_pos.Y = dst_clip.Y;
        }

        src_rect.Width = std::clamp(src_rect.Width, 0, dst_clip.X + dst_clip.Width - dst_pos.X);
        src_rect.Height = std::clamp(src_rect.Height, 0, dst_clip.Y + dst_clip.Height - dst_pos.Y);
    }

    void BitBlt32bppArgb(
        GdipBitmap* src_bitmap_,
        const RECT& src_rect_,
        void* dst_bitmap,
        size_t dst_pitch,
        const POINT& dst_pos_,
        const RECT& dst_clip_)
    {
        Gdiplus::Bitmap* src_bitmap = reinterpret_cast<Gdiplus::Bitmap*>(src_bitmap_);
        Gdiplus::Rect src_rect = Gdiplus::Rect(src_rect_.left, src_rect_.top, src_rect_.right - src_rect_.left, src_rect_.bottom - src_rect_.top);
        Gdiplus::Point dst_pos = Gdiplus::Point(dst_pos_.x, dst_pos_.y);
        Gdiplus::Rect dst_clip = Gdiplus::Rect(dst_clip_.left, dst_clip_.top, dst_clip_.right - dst_clip_.left, dst_clip_.bottom - dst_clip_.top);

        ClipRect(src_rect, dst_pos, dst_clip);
        if (src_rect.Width * src_rect.Height == 0) return;

        Gdiplus::BitmapData locked{};
        Gdiplus::Rect whole{0, 0, static_cast<int>(src_bitmap->GetWidth()), static_cast<int>(src_bitmap->GetHeight())};
        GDIP_THROW_ON_FAILURE src_bitmap->LockBits(&whole, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, &locked);

        {
            const auto pSrc = static_cast<const uint32_t*>(locked.Scan0);
            const auto pDst = static_cast<uint32_t*>(dst_bitmap);
            const auto srcX = src_rect.X;
            const auto srcY = src_rect.Y;
            const auto srcP = static_cast<size_t>(locked.Stride / 4);
            const auto dstX = dst_pos.X;
            const auto dstY = dst_pos.Y;
            const auto dstP = static_cast<size_t>(dst_pitch / 4);
            const auto cpyW = src_rect.Width;
            const auto cpyH = src_rect.Height;

            for (auto y = 0; y < cpyH; y++)
            {
                const auto dst_base = pDst + (dstY + y) * dstP + dstX;
                const auto src_base = pSrc + (srcY + y) * srcP + srcX;
                for (auto x = 0; x < cpyW; x++)
                    dst_base[x] = src_base[x] | 0x00FFFFFFu;
            }
        }

        GDIP_THROW_ON_FAILURE src_bitmap->UnlockBits(&locked);
    }

    void BitBlt8bppAlpha(
        GdipBitmap* src_bitmap_,
        const RECT& src_rect_,
        void* dst_bitmap,
        size_t dst_pitch,
        const POINT& dst_pos_,
        const RECT& dst_clip_)
    {
        Gdiplus::Bitmap* src_bitmap = reinterpret_cast<Gdiplus::Bitmap*>(src_bitmap_);
        Gdiplus::Rect src_rect = Gdiplus::Rect(src_rect_.left, src_rect_.top, src_rect_.right - src_rect_.left, src_rect_.bottom - src_rect_.top);
        Gdiplus::Point dst_pos = Gdiplus::Point(dst_pos_.x, dst_pos_.y);
        Gdiplus::Rect dst_clip = Gdiplus::Rect(dst_clip_.left, dst_clip_.top, dst_clip_.right - dst_clip_.left, dst_clip_.bottom - dst_clip_.top);

        ClipRect(src_rect, dst_pos, dst_clip);
        if (src_rect.Width * src_rect.Height == 0) return;

        Gdiplus::BitmapData locked{};
        Gdiplus::Rect whole{0, 0, static_cast<int>(src_bitmap->GetWidth()), static_cast<int>(src_bitmap->GetHeight())};
        GDIP_THROW_ON_FAILURE src_bitmap->LockBits(&whole, Gdiplus::ImageLockMode::ImageLockModeRead, PixelFormat32bppARGB, &locked);

        {
            const auto pSrc = static_cast<const uint32_t*>(locked.Scan0);
            const auto pDst = static_cast<uint8_t*>(dst_bitmap);
            const auto srcX = src_rect.X;
            const auto srcY = src_rect.Y;
            const auto srcP = static_cast<size_t>(locked.Stride / 4);
            const auto dstX = dst_pos.X;
            const auto dstY = dst_pos.Y;
            const auto dstP = static_cast<size_t>(dst_pitch);
            const auto cpyW = src_rect.Width;
            const auto cpyH = src_rect.Height;

            for (auto y = 0; y < cpyH; y++)
            {
                const auto dst_base = pDst + (dstY + y) * dstP + dstX;
                const auto src_base = pSrc + (srcY + y) * srcP + srcX;
                for (auto x = 0; x < cpyW; x++)
                    dst_base[x] = static_cast<uint8_t>(src_base[x] >> 24);
            }
        }

        GDIP_THROW_ON_FAILURE src_bitmap->UnlockBits(&locked);
    }
}
