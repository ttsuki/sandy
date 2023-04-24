/// @file
///	@brief   sandy::mf
///	@author  (C) 2023 ttsuki

#pragma once

#include <Windows.h>
#include <mfidl.h>

#include <xtw/com.h>

#include "./MfSample.h"

namespace sandy::mf
{
    class MfVideoFrameSample : private MfSample
    {
        xtw::com_ptr<IMFVideoMediaType> media_type_;

    public:
        MfVideoFrameSample() = default;
        MfVideoFrameSample(nullptr_t) : MfSample(nullptr) { }
        MfVideoFrameSample(xtw::com_ptr<IMFSample> sample, xtw::com_ptr<IMFVideoMediaType> format) : MfSample(std::move(sample)), media_type_(std::move(format)) { }
        MfVideoFrameSample(const MfVideoFrameSample& other) = default;
        MfVideoFrameSample(MfVideoFrameSample&& other) noexcept = default;
        MfVideoFrameSample& operator=(const MfVideoFrameSample& other) = default;
        MfVideoFrameSample& operator=(MfVideoFrameSample&& other) noexcept = default;
        ~MfVideoFrameSample() = default;

        using MfSample::operator bool;
        using MfSample::Sample;
        using MfSample::Time;
        using MfSample::Duration;
        using MfSample::BufferCount;
        using MfSample::Buffer;

        [[nodiscard]] xtw::com_ptr<IMFVideoMediaType> MediaType() const noexcept { return media_type_; }
        [[nodiscard]] const MFVIDEOFORMAT* Format() const { return media_type_ ? media_type_->GetVideoFormat() : nullptr; }
    };

    bool BitBltVideoFrame(
        const MfVideoFrameSample& source,
        void* destination_A8R8G8B8,
        int destination_width,
        int destination_height,
        int destination_stride);
}
