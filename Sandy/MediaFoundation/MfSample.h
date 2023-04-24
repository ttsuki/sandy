/// @file
///	@brief   sandy::mf
///	@author  (C) 2023 ttsuki

#pragma once

#include <Windows.h>
#include <mfidl.h>

#include <xtw/com.h>

namespace sandy::mf
{
    class MfSample
    {
        xtw::com_ptr<IMFSample> sample_;

    public:
        MfSample() = default;
        MfSample(nullptr_t) { }
        MfSample(xtw::com_ptr<IMFSample> sample) : sample_(std::move(sample)) { }
        MfSample(const MfSample& other) = default;
        MfSample(MfSample&& other) noexcept = default;
        MfSample& operator=(const MfSample& other) = default;
        MfSample& operator=(MfSample&& other) noexcept = default;
        ~MfSample() = default;

        [[nodiscard]] explicit operator bool() const { return static_cast<bool>(sample_); }
        [[nodiscard]] xtw::com_ptr<IMFSample> Sample() const noexcept { return sample_; }
        [[nodiscard]] LONGLONG Time() const;
        [[nodiscard]] LONGLONG Duration() const;
        [[nodiscard]] DWORD BufferCount() const;
        [[nodiscard]] xtw::com_ptr<IMFMediaBuffer> Buffer(int index) const;
    };
}
