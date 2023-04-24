/// @file
///	@brief   sandy::mf
///	@author  (C) 2023 ttsuki

#include "MfVideoFrameSample.h"

#include <mfapi.h>

#include <xtw/debug.h>

#include "SurfaceFormatConverter.h"

namespace sandy::mf
{
    bool BitBltVideoFrame(
        const MfVideoFrameSample& source,
        void* destination_A8R8G8B8,
        int destination_width,
        int destination_height,
        int destination_stride)
    {
        BYTE* const dst = static_cast<BYTE*>(destination_A8R8G8B8);
        LONG const dst_stride = static_cast<LONG>(destination_stride);

        if (!source) return false;

        // get media type
        const auto src_media_type = source.MediaType();
        if (!src_media_type) return (XTW_EXPECT_SUCCESS E_INVALIDARG), false;

        // check media subtype
        GUID src_format_subtype{};
        XTW_EXPECT_SUCCESS src_media_type->GetGUID(MF_MT_SUBTYPE, &src_format_subtype);
        if (src_format_subtype != MFVideoFormat_NV12) return (XTW_EXPECT_SUCCESS E_INVALIDARG), false; // not supported format.

        // check format
        const MFVIDEOFORMAT* const src_video_format = src_media_type->GetVideoFormat();
        const DWORD width = std::min<DWORD>(destination_width, src_video_format->videoInfo.dwWidth);
        const DWORD height = std::min<DWORD>(destination_height, src_video_format->videoInfo.dwHeight);
        const auto blt_function = src_video_format->videoInfo.ColorPrimaries == MFVideoPrimaries_BT709
            ? sfc::TransformImage_NV12_BT709_to_A8R8G8B8
            : sfc::TransformImage_NV12_BT601_to_A8R8G8B8;

        // get buffer
        const auto src_buffer = source.Buffer(0);
        if (!src_buffer) return (XTW_EXPECT_SUCCESS E_INVALIDARG), false;

        if (const xtw::com_ptr<IMF2DBuffer> src_buffer_2d = src_buffer.as<IMF2DBuffer>())
        {
            // lock
            BYTE* src{};
            LONG src_stride{};
            if (auto hr = XTW_EXPECT_SUCCESS src_buffer_2d->Lock2D(&src, &src_stride); FAILED(hr)) return false;

            // blt
            const auto src_luma = src;
            const auto src_chroma = src_luma + static_cast<ptrdiff_t>(src_stride) * src_video_format->videoInfo.dwHeight;
            blt_function(dst, dst_stride, src_luma, src_chroma, src_stride, width, height);

            // unlock
            XTW_EXPECT_SUCCESS src_buffer_2d->Unlock2D();
        }
        else
        {
            // lock
            BYTE* src{};
            DWORD length = 0;
            if (auto hr = XTW_EXPECT_SUCCESS src_buffer->Lock(&src, nullptr, &length); FAILED(hr)) return false;

            // get stride
            LONG src_stride{};
            if (UINT32 s{}; SUCCEEDED(src_media_type->GetUINT32(MF_MT_DEFAULT_STRIDE, &s)))
                src_stride = static_cast<LONG>(s);
            else if (LONG stride{}; SUCCEEDED(MFGetStrideForBitmapInfoHeader(src_format_subtype.Data1, src_video_format->videoInfo.dwWidth, &stride)))
                src_stride = static_cast<LONG>(stride);
            else
                src_stride = static_cast<LONG>(src_video_format->videoInfo.dwWidth);

            // blt
            const auto src_luma = src;
            const auto src_chroma = src_luma + static_cast<ptrdiff_t>(src_stride) * src_video_format->videoInfo.dwHeight;
            blt_function(dst, dst_stride, src_luma, src_chroma, src_stride, width, std::min<UINT>(height, length / src_stride));

            // unlock
            XTW_EXPECT_SUCCESS src_buffer->Unlock();
        }

        return true;
    }
}
