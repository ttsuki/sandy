/// @file
///	@brief   sandy::SurfaceFormatConverter
///	@author  (C) 2023 ttsuki

#pragma once

#include <cstddef>

namespace sandy::mf::sfc
{
    void TransformImage_NV12_BT601_to_A8R8G8B8(
        void* dst, ptrdiff_t dst_stride,
        const void* src_luma, const void* src_chroma, ptrdiff_t src_stride,
        size_t image_width, size_t image_height);

    void TransformImage_NV12_BT709_to_A8R8G8B8(
        void* dst, ptrdiff_t dst_stride,
        const void* src_luma, const void* src_chroma, ptrdiff_t src_stride,
        size_t image_width, size_t image_height);
}
