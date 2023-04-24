/// @file
///	@brief   sandy::mf
///	@author  (C) 2023 ttsuki

#include "MfUtilityFunctions.h"

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>

#include <filesystem>

#include <xtw/com.h>
#include <xtw/debug.h>

namespace sandy::mf
{
    HRESULT StartupMediaFoundation()
    {
        return XTW_EXPECT_SUCCESS MFStartup(MF_VERSION, MFSTARTUP_LITE);
    }

    HRESULT ShutdownMediaFoundation()
    {
        return XTW_EXPECT_SUCCESS MFShutdown();
    }

    HRESULT CreateMFByteStreamReadFile(const std::filesystem::path& filename, IMFByteStream** out_mf_byte_stream)
    {
        return XTW_EXPECT_SUCCESS ::MFCreateFile(
            MF_ACCESSMODE_READ,
            MF_OPENMODE_FAIL_IF_NOT_EXIST,
            MF_FILEFLAGS_NONE,
            filename.wstring().c_str(),
            out_mf_byte_stream);
    }

    HRESULT CreateMFByteStreamFromIStreamReadOnly(IStream* source_stream, IMFByteStream** out_mf_byte_stream)
    {
        return XTW_EXPECT_SUCCESS ::MFCreateMFByteStreamOnStream(source_stream, out_mf_byte_stream);
    }

    HRESULT CreateMFByteStreamMemoryReadOnly(const void* source_memory, size_t length, IMFByteStream** out_mf_byte_stream)
    {
        xtw::com_ptr<IStream> source_stream;
        XTW_EXPECT_SUCCESS ::CreateStreamOnHGlobal(NULL, TRUE, source_stream.put());

        if (source_memory && length)
        {
            ULONG written = 0;
            XTW_EXPECT_SUCCESS source_stream->Write(source_memory, static_cast<ULONG>(length), &written);
            XTW_EXPECT_SUCCESS source_stream->Seek(LARGE_INTEGER{}, STREAM_SEEK_SET, NULL);
        }

        return CreateMFByteStreamFromIStreamReadOnly(source_stream.get(), out_mf_byte_stream);
    }
}
