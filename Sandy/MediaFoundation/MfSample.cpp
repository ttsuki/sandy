/// @file
///	@brief   sandy::mf
///	@author  (C) 2023 ttsuki

#include "MfSample.h"

#include <xtw/com.h>
#include <xtw/debug.h>

namespace sandy::mf
{
    LONGLONG MfSample::Time() const
    {
        LONGLONG v{};
        if (sample_) { XTW_EXPECT_SUCCESS sample_->GetSampleTime(&v); }
        return v;
    }

    LONGLONG MfSample::Duration() const
    {
        LONGLONG v{};
        if (sample_) { XTW_EXPECT_SUCCESS sample_->GetSampleDuration(&v); }
        return v;
    }

    DWORD MfSample::BufferCount() const
    {
        DWORD v{};
        if (sample_) { XTW_EXPECT_SUCCESS sample_->GetBufferCount(&v); }
        return v;
    }

    xtw::com_ptr<IMFMediaBuffer> MfSample::Buffer(int index) const
    {
        xtw::com_ptr<IMFMediaBuffer> p{};
        if (sample_) { XTW_EXPECT_SUCCESS sample_->GetBufferByIndex(index, p.put()); }
        return p;
    }
}
