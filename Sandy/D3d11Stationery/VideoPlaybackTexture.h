/// @file
///	@brief   TTsukiGameSDK::Direct3D11
///	@author  (C) 2023 ttsuki

#pragma once

#include <Windows.h>
#include <d3d11.h>

#include <memory>
#include <mutex>
#include <chrono>

#include <xtw/com.h>

#include "../MediaFoundation/MfVideoDecoder.h"

namespace sandy::d3d11
{
    xtw::com_ptr<IMFDXGIDeviceManager> CreateMfDxgiDeviceManager(ID3D11Device* device);

    std::unique_ptr<mf::MfVideoDecoder> CreateVideoDecoder(
        IMFDXGIDeviceManager* manager,
        IMFByteStream* stream,
        DWORD stream_index = mf::MfVideoDecoder::kFirstVideoStreamIndex,
        bool enable_d3d11_video_acceleration = true);

    class VideoPlaybackTexture
    {
        class Impl;
        std::unique_ptr<Impl> impl_;

    public:
        using PresentationClock = std::chrono::high_resolution_clock;
        using Duration = std::chrono::duration<LONGLONG, std::ratio<1, 10000000>>;

        VideoPlaybackTexture(ID3D11Device* device, std::unique_ptr<mf::MfVideoDecoder> decoder);
        VideoPlaybackTexture(const VideoPlaybackTexture& other) = delete;
        VideoPlaybackTexture(VideoPlaybackTexture&& other) noexcept = delete;
        VideoPlaybackTexture& operator=(const VideoPlaybackTexture& other) = delete;
        VideoPlaybackTexture& operator=(VideoPlaybackTexture&& other) noexcept = delete;
        ~VideoPlaybackTexture();

        [[nodiscard]] unsigned int GetVideoWidth() const;
        [[nodiscard]] unsigned int GetVideoHeight() const;
        [[nodiscard]] Duration GetVideoDuration() const;
        [[nodiscard]] Duration GetCurrentPosition() const;
        VideoPlaybackTexture& Rewind(bool loop = false);
        VideoPlaybackTexture& Play();
        xtw::com_ptr<ID3D11ShaderResourceView> UpdateTexture(ID3D11DeviceContext* context);
    };
}
