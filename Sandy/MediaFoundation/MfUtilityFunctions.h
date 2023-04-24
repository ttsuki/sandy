/// @file
///	@brief   sandy::mf
///	@author  (C) 2023 ttsuki

#pragma once

#include <Windows.h>
#include <mfobjects.h>
#include <filesystem>

namespace sandy::mf
{
    HRESULT StartupMediaFoundation();
    HRESULT ShutdownMediaFoundation();

    // Stream Helper Functions
    HRESULT CreateMFByteStreamReadFile(const std::filesystem::path& filename, IMFByteStream** out_mf_byte_stream);
    HRESULT CreateMFByteStreamFromIStreamReadOnly(IStream* source_stream, IMFByteStream** out_mf_byte_stream);
    HRESULT CreateMFByteStreamMemoryReadOnly(const void* source_memory, size_t length, IMFByteStream** out_mf_byte_stream);
}
