/// @file
///	@brief    sandy::d3d11::VideoPlaybackTexture
///	@author  (C) 2023 ttsuki

#include "VideoPlaybackTexture.h"

#include <Windows.h>
#include <d3d11.h>
#include <mfapi.h>

#include <memory>
#include <mutex>
#include <chrono>
#include <iostream>
#include <optional>

#include <xtw/com.h>
#include <xtw/debug.h>

#include "../D3d11/Device.h"
#include "../D3d11/UtilityFunctions.h"

namespace sandy::d3d11
{
    xtw::com_ptr<IMFDXGIDeviceManager> CreateMfDxgiDeviceManager(ID3D11Device* device)
    {
        // unlock multithreaded access
        xtw::com_ptr<ID3D10Multithread> multithread{};
        XTW_EXPECT_SUCCESS device->QueryInterface(IID_PPV_ARGS(multithread.put()));
        XTW_EXPECT_SUCCESS multithread->SetMultithreadProtected(TRUE);

        xtw::com_ptr<IMFDXGIDeviceManager> manager;
        UINT reset_token{};
        XTW_EXPECT_SUCCESS MFCreateDXGIDeviceManager(&reset_token, manager.put());
        XTW_EXPECT_SUCCESS manager->ResetDevice(device, reset_token);

        return manager;
    }

    std::unique_ptr<mf::MfVideoDecoder> CreateVideoDecoder(
        IMFDXGIDeviceManager* manager,
        IMFByteStream* stream,
        DWORD stream_index,
        bool enable_d3d11_video_acceleration)
    {
        constexpr int decoder_queue_depth = 4;

        xtw::com_ptr<IMFAttributes> attributes;
        if (enable_d3d11_video_acceleration)
        {
            XTW_EXPECT_SUCCESS MFCreateAttributes(attributes.put(), 0);
            XTW_EXPECT_SUCCESS attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 1);
            XTW_EXPECT_SUCCESS attributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, manager);
            XTW_EXPECT_SUCCESS attributes->SetUINT32(MF_SOURCE_READER_DISABLE_DXVA, 0);
        }

        return std::make_unique<mf::MfVideoDecoder>(stream, attributes.get(), decoder_queue_depth, stream_index);
    }


    struct VideoBltContext
    {
        xtw::com_ptr<ID3D11VideoDevice> video_device;
        D3D11_VIDEO_PROCESSOR_CONTENT_DESC content_desc;
        xtw::com_ptr<ID3D11VideoProcessorEnumerator> video_proc_enum;
        xtw::com_ptr<ID3D11VideoProcessor> processor;

        VideoBltContext(ID3D11Device* device, UINT width, UINT height)
            : video_device(xtw::com_ptr(device).as<ID3D11VideoDevice>())
            , content_desc{
                  D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE,
                  {120000, 1000}, width, height,
                  {120000, 1000}, width, height,
                  D3D11_VIDEO_USAGE_PLAYBACK_NORMAL
              }
        {
            XTW_EXPECT_SUCCESS video_device->CreateVideoProcessorEnumerator(&content_desc, video_proc_enum.put());
            XTW_EXPECT_SUCCESS video_device->CreateVideoProcessor(video_proc_enum.get(), 0, processor.put());
        }

        void VideoBitBlt(ID3D11DeviceContext* context, IMFDXGIBuffer* input, ID3D11Texture2D* output) const
        {
            // Creates ID3D11VideoProcessor{Input,Output}View
            xtw::com_ptr<ID3D11VideoProcessorInputView> input_view;
            xtw::com_ptr<ID3D11VideoProcessorOutputView> output_view;

            {
                xtw::com_ptr<ID3D11Texture2D> input_texture{};
                UINT input_subresource{};
                XTW_EXPECT_SUCCESS input->GetResource(IID_PPV_ARGS(input_texture.put()));
                XTW_EXPECT_SUCCESS input->GetSubresourceIndex(&input_subresource);

                auto input_texture_desc = util::GetDesc(input_texture.get());
                D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC input_desc{};
                input_desc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
                input_desc.Texture2D.MipSlice = 0;
                input_desc.Texture2D.ArraySlice = input_subresource;
                XTW_EXPECT_SUCCESS video_device->CreateVideoProcessorInputView(input_texture.get(), video_proc_enum.get(), &input_desc, input_view.put());
            }

            {
                D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC output_desc{};
                output_desc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
                output_desc.Texture2D.MipSlice = 0;
                XTW_EXPECT_SUCCESS video_device->CreateVideoProcessorOutputView(output, video_proc_enum.get(), &output_desc, output_view.put());
            }

            D3D11_VIDEO_PROCESSOR_STREAM stream0{};
            stream0.Enable = TRUE;
            stream0.pInputSurface = input_view.get();

            const auto input_rect = RECT{0, 0, static_cast<LONG>(content_desc.InputWidth), static_cast<LONG>(content_desc.InputHeight)};
            const auto output_rect = RECT{0, 0, static_cast<LONG>(content_desc.OutputWidth), static_cast<LONG>(content_desc.OutputHeight)};
            const auto video_context = xtw::com_ptr(context).as<ID3D11VideoContext>();
            video_context->VideoProcessorSetStreamFrameFormat(processor.get(), 0, D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);
            video_context->VideoProcessorSetStreamOutputRate(processor.get(), 0, D3D11_VIDEO_PROCESSOR_OUTPUT_RATE_NORMAL, false, nullptr);
            video_context->VideoProcessorSetStreamSourceRect(processor.get(), 0, TRUE, &input_rect);
            video_context->VideoProcessorSetStreamDestRect(processor.get(), 0, TRUE, &input_rect);
            video_context->VideoProcessorSetOutputTargetRect(processor.get(), TRUE, &output_rect);
            XTW_EXPECT_SUCCESS video_context->VideoProcessorBlt(processor.get(), output_view.get(), 0, 1, &stream0);
        }
    };

    class VideoPlaybackTexture::Impl
    {
        std::unique_ptr<mf::MfVideoDecoder> decoder_{};
        unsigned int video_width_{1};
        unsigned int video_height_{1};
        Duration video_duration_{0};

        xtw::com_ptr<ID3D11Texture2D> offscreen_texture_{};
        xtw::com_ptr<ID3D11Texture2D> render_target_texture_{};
        xtw::com_ptr<ID3D11ShaderResourceView> render_target_texture_srv_{};
        std::optional<VideoBltContext> video_blt_context_{};

        bool need_to_clear_texture_{};
        bool playing_{};
        PresentationClock::time_point play_started_at_{};

    public:
        Impl(ID3D11Device* device, std::unique_ptr<mf::MfVideoDecoder> decoder)
            : decoder_(std::move(decoder))
        {
            if (decoder_->IsReady())
            {
                video_width_ = static_cast<unsigned int>(decoder_->GetVideoInfo().dwWidth);
                video_height_ = static_cast<unsigned int>(decoder_->GetVideoInfo().dwHeight);
                video_duration_ = Duration{decoder_->GetVideoDuration()};
                decoder_->Rewind(false);
            }

            // allocates target textures
            {
                offscreen_texture_ = Device(device).CreateTexture2D(
                    CD3D11_TEXTURE2D_DESC{
                        DXGI_FORMAT_B8G8R8A8_UNORM, video_width_, video_height_, 1, 1,
                        0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_WRITE,
                        1, 0, 0
                    });

                render_target_texture_ = Device(device).CreateTexture2D(
                    CD3D11_TEXTURE2D_DESC{
                        DXGI_FORMAT_B8G8R8A8_UNORM, video_width_, video_height_, 1, 1,
                        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, D3D11_USAGE_DEFAULT, 0,
                        1, 0, 0
                    });

                render_target_texture_srv_ = Device(device).CreateShaderResourceView(
                    render_target_texture_.get(),
                    CD3D11_SHADER_RESOURCE_VIEW_DESC{render_target_texture_.get(), D3D11_SRV_DIMENSION_TEXTURE2D,});
            }

            // allocates VideoBltContext
            video_blt_context_.emplace(device, video_width_, video_height_);

            // reserve to clear texture on next update
            need_to_clear_texture_ = true;
        }

        unsigned int GetVideoWidth() const { return video_width_; }
        unsigned int GetVideoHeight() const { return video_height_; }
        Duration GetVideoDuration() const { return video_duration_; }
        Duration GetCurrentPosition() const { return playing_ ? std::chrono::duration_cast<Duration>(PresentationClock::now() - play_started_at_) : Duration{}; }

        void Rewind(bool loop)
        {
            if (decoder_->IsReady())
            {
                decoder_->Rewind(loop);
            }

            playing_ = false;
            need_to_clear_texture_ = true;
        }

        void Play()
        {
            if (decoder_->IsReady() && !playing_)
            {
                play_started_at_ = PresentationClock::now();
                playing_ = true;
            }
        }

        xtw::com_ptr<ID3D11ShaderResourceView> UpdateTexture(ID3D11DeviceContext* context)
        {
            if (playing_)
            {
                Duration now = GetCurrentPosition();

                // Render.
                if (const auto frame_to_render = decoder_->FetchFrame(now.count()))
                {
                    if ( // If DXVA supported, get IMFDXGIBuffer and Blt to the render target via D3D11Video.
                        xtw::com_ptr<IMFDXGIBuffer> dxgi_buffer{};
                        video_blt_context_ && SUCCEEDED(frame_to_render.Buffer(0)->QueryInterface(IID_PPV_ARGS(dxgi_buffer.put()))))
                    {
                        video_blt_context_->VideoBitBlt(context, dxgi_buffer.get(), render_target_texture_.get());
                        need_to_clear_texture_ = false;
                    }
                    else // Otherwise use CPU BitBlit to offscreen surface and transfer it to render target.
                    {
                        if (D3D11_MAPPED_SUBRESOURCE locked{};
                            SUCCEEDED(XTW_EXPECT_SUCCESS context->Map(
                                offscreen_texture_.get(),
                                D3D11CalcSubresource(0, 0, 0),
                                D3D11_MAP_WRITE,
                                0, &locked)))
                        {
                            mf::BitBltVideoFrame(frame_to_render, locked.pData, static_cast<int>(video_width_), static_cast<int>(video_height_), static_cast<int>(locked.RowPitch));
                            context->Unmap(offscreen_texture_.get(), D3D11CalcSubresource(0, 0, 0));

                            D3D11_BOX box{0, 0, 0, video_width_, video_height_, 1};
                            context->CopySubresourceRegion(
                                render_target_texture_.get(), D3D11CalcSubresource(0, 0, 0), 0, 0, 0,
                                offscreen_texture_.get(), D3D11CalcSubresource(0, 0, 0), &box);

                            need_to_clear_texture_ = false;
                        }
                    }
                }
            }

            if (need_to_clear_texture_)
            {
                if (D3D11_MAPPED_SUBRESOURCE locked{};
                    SUCCEEDED(XTW_EXPECT_SUCCESS context->Map(
                        offscreen_texture_.get(),
                        D3D11CalcSubresource(0, 0, 0),
                        D3D11_MAP_WRITE,
                        0, &locked)))
                {
                    memset(locked.pData, 0, static_cast<size_t>(locked.RowPitch) * video_height_);
                    context->Unmap(offscreen_texture_.get(), D3D11CalcSubresource(0, 0, 0));

                    D3D11_BOX box{0, 0, 0, video_width_, video_height_, 1};
                    context->CopySubresourceRegion(
                        render_target_texture_.get(), D3D11CalcSubresource(0, 0, 0), 0, 0, 0,
                        offscreen_texture_.get(), D3D11CalcSubresource(0, 0, 0), &box);

                    need_to_clear_texture_ = false;
                }
            }

            return render_target_texture_srv_;
        }
    };

    VideoPlaybackTexture::VideoPlaybackTexture(ID3D11Device* device, std::unique_ptr<mf::MfVideoDecoder> decoder) : impl_(std::make_unique<Impl>(device, std::move(decoder))) { }
    VideoPlaybackTexture::~VideoPlaybackTexture() = default;

    unsigned VideoPlaybackTexture::GetVideoWidth() const { return impl_->GetVideoWidth(); }
    unsigned VideoPlaybackTexture::GetVideoHeight() const { return impl_->GetVideoHeight(); }
    VideoPlaybackTexture::Duration VideoPlaybackTexture::GetVideoDuration() const { return impl_->GetVideoDuration(); }
    VideoPlaybackTexture::Duration VideoPlaybackTexture::GetCurrentPosition() const { return impl_->GetCurrentPosition(); }
    VideoPlaybackTexture& VideoPlaybackTexture::Rewind(bool loop) { return impl_->Rewind(loop), *this; }
    VideoPlaybackTexture& VideoPlaybackTexture::Play() { return impl_->Play(), *this; }
    xtw::com_ptr<ID3D11ShaderResourceView> VideoPlaybackTexture::UpdateTexture(ID3D11DeviceContext* context) { return impl_->UpdateTexture(context); }
}
