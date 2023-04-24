/// @file
///	@brief   sandy::SurfaceFormatConverter
///	@author  (C) 2023 ttsuki

#include "SurfaceFormatConverter.h"

#include <cstddef>
#include <cstdint>
#include <algorithm>

#if defined(__RESHARPER__) && !defined(__AVX2__)
#define __AVX2__
#endif

#if !defined(__AVX2__)
#error AVX2 must be enabled.
#endif

#ifdef __AVX2__
#include <ark/xmm.h>
#endif

namespace sandy::mf::sfc
{
#ifdef __AVX2__
    // mul_hadd_dup({a0,a1,a2,a3,a4,a5,a6,a7}, {b0,b1,b2,b3,b4,b5,b6,b7})
    //    := {a0*b0+a1*b1, a0*b0+a1*b1, a2*b2+a3*b3, a2*b2+a3*b3, a4*b4+a5*b5, a4*b4+a5*b5, a6*b6+a7*b7, a6*b6+a7*b7}
    ARKXMM_API mul_hadd_dup(arkxmm::vi16x16 a, arkxmm::vi16x16 b) -> arkxmm::vi16x16
    {
        using namespace arkxmm;
        vi32x8 t = mul_hadd(a, b);    // i32{ a0*b0+a1*b1, a2*b2+a3*b3, a4*b4+a5*b5, a6*b6+a7*b7, }
        vi16x16 u = pack_sat_i(t, t); // i16{ a0*b0+a1*b1, a2*b2+a3*b3, a4*b4+a5*b5, a6*b6+a7*b7, a0*b0+a1*b1, a2*b2+a3*b3, a4*b4+a5*b5, a6*b6+a7*b7, }
        vi16x16 v = unpack_lo(u, u);  // i16{ a0*b0+a1*b1, a0*b0+a1*b1, a2*b2+a3*b3, a2*b2+a3*b3, a4*b4+a5*b5, a4*b4+a5*b5, a6*b6+a7*b7, a6*b6+a7*b7, }
        return v;                     // i16{ a0*b0+a1*b1, a0*b0+a1*b1, a2*b2+a3*b3, a2*b2+a3*b3, a4*b4+a5*b5, a4*b4+a5*b5, a6*b6+a7*b7, a6*b6+a7*b7, }
    }

#endif

    template <int kYrgb,
              int kUr, int kUg, int kUb,
              int kVr, int kVg, int kVb>
    static void TransformImage_NV12_to_A8R8G8B8(
        void* dst, ptrdiff_t dst_stride,
        const void* src_luma_plain, const void* src_chroma_plain, ptrdiff_t src_stride,
        size_t image_width, size_t image_height)
    {
        size_t height = image_height & ~1;
        size_t width = image_width & ~1;

#ifdef __AVX2__

        if (size_t rounded_up = width + 31 & ~31;
            (width & 31) != 0 &&
            static_cast<size_t>(std::abs(src_stride)) >= rounded_up * 1 &&
            static_cast<size_t>(std::abs(dst_stride)) >= rounded_up * 4)
        {
            width = rounded_up;
        }

        if (size_t rounded_up = width + 15 & ~15;
            (width & 15) != 0 &&
            static_cast<size_t>(std::abs(src_stride)) >= rounded_up * 1 &&
            static_cast<size_t>(std::abs(dst_stride)) >= rounded_up * 4)
        {
            width = rounded_up;
        }

#endif

        for (size_t y = 0; y < (height & ~1); y += 2)
        {
            constexpr int kPreShift = 3;
            constexpr int kPostShift = 8 - kPreShift;

            static constexpr int16_t kRGBy = kYrgb >> kPreShift;
            static constexpr int16_t kRu = kUr >> kPreShift;
            static constexpr int16_t kRv = kVr >> kPreShift;
            static constexpr int16_t kGu = kUg >> kPreShift;
            static constexpr int16_t kGv = kVg >> kPreShift;
            static constexpr int16_t kBu = kUb >> kPreShift;
            static constexpr int16_t kBv = kVb >> kPreShift;
            static constexpr int16_t kRoundOffset = (1 << kPostShift) / 2;

            using byte_t = uint8_t;

            size_t x = 0;
            auto* src_luma0 = static_cast<const byte_t*>(src_luma_plain) + src_stride * (y + 0);
            auto* src_luma1 = static_cast<const byte_t*>(src_luma_plain) + src_stride * (y + 1);
            auto* src_chroma = static_cast<const byte_t*>(src_chroma_plain) + src_stride * (y / 2);
            auto* dst_bgra0 = static_cast<byte_t*>(dst) + dst_stride * (y + 0);
            auto* dst_bgra1 = static_cast<byte_t*>(dst) + dst_stride * (y + 1);

#ifdef __AVX2__

            for (; x < (width & ~31); x += 32)
            {
                using namespace arkxmm;

                vu8x32 ze = zero<vu8x32>();
                vi16x16 ky = i16x16(kRGBy);
                vi16x16 kcr = i16x16(kRu, kRv, kRu, kRv, kRu, kRv, kRu, kRv);
                vi16x16 kcg = i16x16(kGu, kGv, kGu, kGv, kGu, kGv, kGu, kGv);
                vi16x16 kcb = i16x16(kBu, kBv, kBu, kBv, kBu, kBv, kBu, kBv);

                vu8x32 y0 = permute32<0, 2, 4, 6, 1, 3, 5, 7>(load_u<vu8x32>(src_luma0 + x));
                vu8x32 y1 = permute32<0, 2, 4, 6, 1, 3, 5, 7>(load_u<vu8x32>(src_luma1 + x));
                vu8x32 c0 = permute32<0, 2, 4, 6, 1, 3, 5, 7>(load_u<vu8x32>(src_chroma + x));

                vi16x16 y00 = reinterpret<vi16x16>(unpack_lo(y0, ze)) - 16;
                vi16x16 y01 = reinterpret<vi16x16>(unpack_hi(y0, ze)) - 16;
                vi16x16 y10 = reinterpret<vi16x16>(unpack_lo(y1, ze)) - 16;
                vi16x16 y11 = reinterpret<vi16x16>(unpack_hi(y1, ze)) - 16;
                vi16x16 c00 = reinterpret<vi16x16>(unpack_lo(c0, ze)) - 128;
                vi16x16 c01 = reinterpret<vi16x16>(unpack_hi(c0, ze)) - 128;

                vi16x16 y00rgb = y00 * ky;
                vi16x16 y01rgb = y01 * ky;
                vi16x16 y10rgb = y10 * ky;
                vi16x16 y11rgb = y11 * ky;
                vi16x16 c00r = mul_hadd_dup(c00, kcr);
                vi16x16 c00g = mul_hadd_dup(c00, kcg);
                vi16x16 c00b = mul_hadd_dup(c00, kcb);
                vi16x16 c01r = mul_hadd_dup(c01, kcr);
                vi16x16 c01g = mul_hadd_dup(c01, kcg);
                vi16x16 c01b = mul_hadd_dup(c01, kcb);

                vi16x16 r00 = (y00rgb + c00r /* + kRoundOffset */) >> kPostShift;
                vi16x16 g00 = (y00rgb + c00g /* + kRoundOffset */) >> kPostShift;
                vi16x16 b00 = (y00rgb + c00b /* + kRoundOffset */) >> kPostShift;
                vi16x16 r01 = (y01rgb + c01r /* + kRoundOffset */) >> kPostShift;
                vi16x16 g01 = (y01rgb + c01g /* + kRoundOffset */) >> kPostShift;
                vi16x16 b01 = (y01rgb + c01b /* + kRoundOffset */) >> kPostShift;
                vi16x16 r10 = (y10rgb + c00r /* + kRoundOffset */) >> kPostShift;
                vi16x16 g10 = (y10rgb + c00g /* + kRoundOffset */) >> kPostShift;
                vi16x16 b10 = (y10rgb + c00b /* + kRoundOffset */) >> kPostShift;
                vi16x16 r11 = (y11rgb + c01r /* + kRoundOffset */) >> kPostShift;
                vi16x16 g11 = (y11rgb + c01g /* + kRoundOffset */) >> kPostShift;
                vi16x16 b11 = (y11rgb + c01b /* + kRoundOffset */) >> kPostShift;

                vu8x32 r0 = pack_sat_u(r00, r01);
                vu8x32 g0 = pack_sat_u(g00, g01);
                vu8x32 b0 = pack_sat_u(b00, b01);
                vu8x32 r1 = pack_sat_u(r10, r11);
                vu8x32 g1 = pack_sat_u(g10, g11);
                vu8x32 b1 = pack_sat_u(b10, b11);
                vu8x32 a0 = u8x32(255);

                vu32x8 bgra00 = reinterpret<vu32x8>(unpack_lo(reinterpret<vu16x16>(unpack_lo(b0, g0)), reinterpret<vu16x16>(unpack_lo(r0, a0))));
                vu32x8 bgra01 = reinterpret<vu32x8>(unpack_hi(reinterpret<vu16x16>(unpack_lo(b0, g0)), reinterpret<vu16x16>(unpack_lo(r0, a0))));
                vu32x8 bgra02 = reinterpret<vu32x8>(unpack_lo(reinterpret<vu16x16>(unpack_hi(b0, g0)), reinterpret<vu16x16>(unpack_hi(r0, a0))));
                vu32x8 bgra03 = reinterpret<vu32x8>(unpack_hi(reinterpret<vu16x16>(unpack_hi(b0, g0)), reinterpret<vu16x16>(unpack_hi(r0, a0))));
                vu32x8 bgra10 = reinterpret<vu32x8>(unpack_lo(reinterpret<vu16x16>(unpack_lo(b1, g1)), reinterpret<vu16x16>(unpack_lo(r1, a0))));
                vu32x8 bgra11 = reinterpret<vu32x8>(unpack_hi(reinterpret<vu16x16>(unpack_lo(b1, g1)), reinterpret<vu16x16>(unpack_lo(r1, a0))));
                vu32x8 bgra12 = reinterpret<vu32x8>(unpack_lo(reinterpret<vu16x16>(unpack_hi(b1, g1)), reinterpret<vu16x16>(unpack_hi(r1, a0))));
                vu32x8 bgra13 = reinterpret<vu32x8>(unpack_hi(reinterpret<vu16x16>(unpack_hi(b1, g1)), reinterpret<vu16x16>(unpack_hi(r1, a0))));

                store_u<vu32x8>(dst_bgra0 + sizeof(vu32x8) * 0, bgra00);
                store_u<vu32x8>(dst_bgra0 + sizeof(vu32x8) * 1, bgra01);
                store_u<vu32x8>(dst_bgra0 + sizeof(vu32x8) * 2, bgra02);
                store_u<vu32x8>(dst_bgra0 + sizeof(vu32x8) * 3, bgra03);
                store_u<vu32x8>(dst_bgra1 + sizeof(vu32x8) * 0, bgra10);
                store_u<vu32x8>(dst_bgra1 + sizeof(vu32x8) * 1, bgra11);
                store_u<vu32x8>(dst_bgra1 + sizeof(vu32x8) * 2, bgra12);
                store_u<vu32x8>(dst_bgra1 + sizeof(vu32x8) * 3, bgra13);

                dst_bgra0 += sizeof(vu32x8) * 4;
                dst_bgra1 += sizeof(vu32x8) * 4;
            }

            for (; x < (width & ~15); x += 16)
            {
                using namespace arkxmm;

                vi16x16 ky = i16x16(kRGBy);
                vi16x16 kcr = i16x16(kRu, kRv, kRu, kRv, kRu, kRv, kRu, kRv);
                vi16x16 kcg = i16x16(kGu, kGv, kGu, kGv, kGu, kGv, kGu, kGv);
                vi16x16 kcb = i16x16(kBu, kBv, kBu, kBv, kBu, kBv, kBu, kBv);

                vu8x16 y0 = shuffle32<0, 2, 1, 3>(load_u<vu8x16>(src_luma0 + x));
                vu8x16 y1 = shuffle32<0, 2, 1, 3>(load_u<vu8x16>(src_luma1 + x));
                vu8x16 c0 = shuffle32<0, 2, 1, 3>(load_u<vu8x16>(src_chroma + x));

                vi16x16 y00 = convert_cast<vi16x16>(y0) - 16;
                vi16x16 y10 = convert_cast<vi16x16>(y1) - 16;
                vi16x16 c00 = convert_cast<vi16x16>(c0) - 128;

                vi16x16 y00rgb = y00 * ky;
                vi16x16 y10rgb = y10 * ky;
                vi16x16 c00r = mul_hadd_dup(c00, kcr);
                vi16x16 c00g = mul_hadd_dup(c00, kcg);
                vi16x16 c00b = mul_hadd_dup(c00, kcb);

                vi16x16 r00 = (y00rgb + c00r /* + kRoundOffset */) >> kPostShift;
                vi16x16 g00 = (y00rgb + c00g /* + kRoundOffset */) >> kPostShift;
                vi16x16 b00 = (y00rgb + c00b /* + kRoundOffset */) >> kPostShift;
                vi16x16 r10 = (y10rgb + c00r /* + kRoundOffset */) >> kPostShift;
                vi16x16 g10 = (y10rgb + c00g /* + kRoundOffset */) >> kPostShift;
                vi16x16 b10 = (y10rgb + c00b /* + kRoundOffset */) >> kPostShift;

                vu8x32 r0 = pack_sat_u(r00, r00);
                vu8x32 g0 = pack_sat_u(g00, g00);
                vu8x32 b0 = pack_sat_u(b00, b00);
                vu8x32 r1 = pack_sat_u(r10, r10);
                vu8x32 g1 = pack_sat_u(g10, g10);
                vu8x32 b1 = pack_sat_u(b10, b10);
                vu8x32 a0 = u8x32(255);

                vu32x8 bgra00 = reinterpret<vu32x8>(unpack_lo(reinterpret<vu16x16>(unpack_lo(b0, g0)), reinterpret<vu16x16>(unpack_lo(r0, a0))));
                vu32x8 bgra01 = reinterpret<vu32x8>(unpack_hi(reinterpret<vu16x16>(unpack_lo(b0, g0)), reinterpret<vu16x16>(unpack_lo(r0, a0))));
                vu32x8 bgra10 = reinterpret<vu32x8>(unpack_lo(reinterpret<vu16x16>(unpack_lo(b1, g1)), reinterpret<vu16x16>(unpack_lo(r1, a0))));
                vu32x8 bgra11 = reinterpret<vu32x8>(unpack_hi(reinterpret<vu16x16>(unpack_lo(b1, g1)), reinterpret<vu16x16>(unpack_lo(r1, a0))));

                store_u<vu32x8>(dst_bgra0 + sizeof(vu32x8) * 0, bgra00);
                store_u<vu32x8>(dst_bgra0 + sizeof(vu32x8) * 1, bgra01);
                store_u<vu32x8>(dst_bgra1 + sizeof(vu32x8) * 0, bgra10);
                store_u<vu32x8>(dst_bgra1 + sizeof(vu32x8) * 1, bgra11);

                dst_bgra0 += sizeof(vu32x8) * 2;
                dst_bgra1 += sizeof(vu32x8) * 2;
            }

#endif

            for (; x < (width & ~1); x += 2)
            {
                int y00 = static_cast<int>(src_luma0[x + 0]) - 16;
                int y01 = static_cast<int>(src_luma0[x + 1]) - 16;
                int y10 = static_cast<int>(src_luma1[x + 0]) - 16;
                int y11 = static_cast<int>(src_luma1[x + 1]) - 16;
                int cb = static_cast<int>(src_chroma[x + 0]) - 128;
                int cr = static_cast<int>(src_chroma[x + 1]) - 128;

                dst_bgra0[0 + 0] = static_cast<byte_t>(std::clamp((kRGBy * y00 + kBu * cb + kBv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra0[0 + 1] = static_cast<byte_t>(std::clamp((kRGBy * y00 + kGu * cb + kGv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra0[0 + 2] = static_cast<byte_t>(std::clamp((kRGBy * y00 + kRu * cb + kRv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra0[0 + 3] = static_cast<byte_t>(255);

                dst_bgra0[4 + 0] = static_cast<byte_t>(std::clamp((kRGBy * y01 + kBu * cb + kBv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra0[4 + 1] = static_cast<byte_t>(std::clamp((kRGBy * y01 + kGu * cb + kGv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra0[4 + 2] = static_cast<byte_t>(std::clamp((kRGBy * y01 + kRu * cb + kRv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra0[4 + 3] = static_cast<byte_t>(255);

                dst_bgra1[0 + 0] = static_cast<byte_t>(std::clamp((kRGBy * y10 + kBu * cb + kBv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra1[0 + 1] = static_cast<byte_t>(std::clamp((kRGBy * y10 + kGu * cb + kGv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra1[0 + 2] = static_cast<byte_t>(std::clamp((kRGBy * y10 + kRu * cb + kRv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra1[0 + 3] = static_cast<byte_t>(255);

                dst_bgra1[4 + 0] = static_cast<byte_t>(std::clamp((kRGBy * y11 + kBu * cb + kBv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra1[4 + 1] = static_cast<byte_t>(std::clamp((kRGBy * y11 + kGu * cb + kGv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra1[4 + 2] = static_cast<byte_t>(std::clamp((kRGBy * y11 + kRu * cb + kRv * cr /* + kRoundOffset */) >> kPostShift, 0, 255));
                dst_bgra1[4 + 3] = static_cast<byte_t>(255);

                dst_bgra0 += 8;
                dst_bgra1 += 8;
            }
        }
    }

    void TransformImage_NV12_BT601_to_A8R8G8B8(
        void* dst, ptrdiff_t dst_stride,
        const void* src_luma, const void* src_chroma, ptrdiff_t src_stride,
        size_t image_width, size_t image_height)
    {
        //BT.601
        constexpr int kYrgb = static_cast<int>(1.164 * 256);
        constexpr int kUr = static_cast<int>(+0.000 * 256);
        constexpr int kVr = static_cast<int>(+1.596 * 256);
        constexpr int kUg = static_cast<int>(-0.391 * 256);
        constexpr int kVg = static_cast<int>(-0.813 * 256);
        constexpr int kUb = static_cast<int>(+2.018 * 256);
        constexpr int kVb = static_cast<int>(+0.000 * 256);

        return TransformImage_NV12_to_A8R8G8B8<
            kYrgb,
            kUr, kUg, kUb,
            kVr, kVg, kVb>(
            dst, dst_stride,
            src_luma, src_chroma, src_stride,
            image_width, image_height);
    }

    void TransformImage_NV12_BT709_to_A8R8G8B8(
        void* dst, ptrdiff_t dst_stride,
        const void* src_luma, const void* src_chroma, ptrdiff_t src_stride,
        size_t image_width, size_t image_height)
    {
        //BT.709
        constexpr int kYrgb = static_cast<int>(1.164 * 256);
        constexpr int kUr = static_cast<int>(+0.000 * 256);
        constexpr int kVr = static_cast<int>(+1.793 * 256);
        constexpr int kUg = static_cast<int>(-0.213 * 256);
        constexpr int kVg = static_cast<int>(-0.533 * 256);
        constexpr int kUb = static_cast<int>(+2.112 * 256);
        constexpr int kVb = static_cast<int>(+0.000 * 256);

        return TransformImage_NV12_to_A8R8G8B8<
            kYrgb,
            kUr, kUg, kUb,
            kVr, kVg, kVb>(
            dst, dst_stride,
            src_luma, src_chroma, src_stride,
            image_width, image_height);
    }
}
