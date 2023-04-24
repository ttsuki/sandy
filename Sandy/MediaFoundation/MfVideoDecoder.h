/// @file
///	@brief   sandy::mf::MfVideoDecoder
///	@author  (C) 2023 ttsuki

// Note: To decode videos with Media Foundation, client may have to install video extensions.
//   MPEG-1/MPEG-2: https://apps.microsoft.com/store/detail/9N95Q1ZZPMH4
//      H.265 HEVC: https://apps.microsoft.com/store/detail/9NMZLZ57R3T7
//             AV1: https://apps.microsoft.com/store/detail/9MVZQVXJBQ9V

#pragma once

#include <Windows.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <xtw/com.h>

#include "MfVideoFrameSample.h"

namespace sandy::mf
{
    class MfVideoDecoder final
    {
        class Impl;
        std::unique_ptr<Impl> impl_;

    public:
        static constexpr DWORD kFirstVideoStreamIndex = static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM);

        MfVideoDecoder(IMFByteStream* stream, int decoder_queue_depth = 10, DWORD stream_index = kFirstVideoStreamIndex);
        MfVideoDecoder(IMFByteStream* stream, IMFAttributes* attributes, int decoder_queue_depth = 10, DWORD stream_index = kFirstVideoStreamIndex);
        MfVideoDecoder(IMFSourceReader* source, int decoder_queue_depth = 10, DWORD stream_index = kFirstVideoStreamIndex);
        MfVideoDecoder(const MfVideoDecoder& other) = delete;
        MfVideoDecoder(MfVideoDecoder&& other) noexcept = delete;
        MfVideoDecoder& operator=(const MfVideoDecoder& other) = delete;
        MfVideoDecoder& operator=(MfVideoDecoder&& other) noexcept = delete;
        ~MfVideoDecoder();

        [[nodiscard]] bool IsReady() const;

        [[nodiscard]] xtw::com_ptr<IMFVideoMediaType> GetMediaType() const;
        [[nodiscard]] const MFVideoInfo& GetVideoInfo() const;
        [[nodiscard]] LONGLONG GetVideoDuration() const;

        [[nodiscard]] bool IsEndOfStream() const;
        void Rewind(bool looping);
        [[nodiscard]] MfVideoFrameSample FetchFrame(LONGLONG current_time);

    };

}
