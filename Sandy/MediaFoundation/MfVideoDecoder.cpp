/// @file
///	@brief   sandy::mf::MfVideoDecoder
///	@author  (C) 2023 ttsuki

#include "MfVideoDecoder.h"

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>

#include <memory>
#include <chrono>
#include <tuple>
#include <optional>
#include <utility>
#include <thread>
#include <mutex>

#include <xtl/xtl_concurrent_queue.h>
#include <xtw/debug.h>

namespace sandy::mf
{
    class MfSourceReader final
    {
        std::mutex mutex_;
        xtw::com_ptr<IMFSourceReader> reader_;

    public:
        MfSourceReader(_In_ xtw::com_ptr<IMFSourceReader> source_reader)
            : reader_(std::move(source_reader))
        {
            //
        }

        MfSourceReader(_In_ IMFByteStream* stream, _In_opt_ IMFAttributes* attributes = nullptr)
        {
            if (!stream)
            {
                XTW_EXPECT_SUCCESS E_INVALIDARG;
                return;
            }

            xtw::com_ptr<IMFSourceReader> source_reader{};
            if (SUCCEEDED(XTW_EXPECT_SUCCESS MFCreateSourceReaderFromByteStream(stream, attributes, source_reader.put())))
            {
                reader_ = std::move(source_reader);
            }
        }

        MfSourceReader(const MfSourceReader& other) = delete;
        MfSourceReader(MfSourceReader&& other) noexcept = delete;
        MfSourceReader& operator=(const MfSourceReader& other) = delete;
        MfSourceReader& operator=(MfSourceReader&& other) noexcept = delete;

        [[nodiscard]] bool IsReady() const { return static_cast<bool>(reader_); }

        [[nodiscard]] std::tuple<LONGLONG, HRESULT> GetDuration() const
        {
            if (!IsReady()) return {0, XTW_EXPECT_SUCCESS E_UNEXPECTED};

            PROPVARIANT var{};
            HRESULT hr = (XTW_EXPECT_SUCCESS reader_->GetPresentationAttribute(static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &var));
            if (FAILED(hr)) return {0, hr};

            LONGLONG duration{};
            hr = (XTW_EXPECT_SUCCESS PropVariantToInt64(var, &duration));
            PropVariantClear(&var);

            return {duration, hr};
        }

        std::tuple<xtw::com_ptr<IMFMediaType>, HRESULT> GetNativeMediaType(_In_ DWORD stream_index, _In_ DWORD media_type_index = 0)
        {
            if (!IsReady()) return {nullptr, XTW_EXPECT_SUCCESS E_UNEXPECTED};
            std::lock_guard lock(mutex_);
            xtw::com_ptr<IMFMediaType> media_type{};
            HRESULT hr = reader_->GetNativeMediaType(stream_index, media_type_index, media_type.put());
            if (FAILED(hr)) return {nullptr, hr};
            return {std::move(media_type), hr};
        }

        std::tuple<xtw::com_ptr<IMFMediaType>, HRESULT> GetCurrentMediaType(_In_ DWORD stream_index)
        {
            if (!IsReady()) return {nullptr, XTW_EXPECT_SUCCESS E_UNEXPECTED};
            std::lock_guard lock(mutex_);
            xtw::com_ptr<IMFMediaType> media_type{};
            HRESULT hr = (XTW_EXPECT_SUCCESS reader_->GetCurrentMediaType(stream_index, media_type.put()));
            if (FAILED(hr)) return {nullptr, hr};
            return {std::move(media_type), hr};
        }

        HRESULT RequestMediaType(_In_ DWORD stream_index, _In_ IMFMediaType* request)
        {
            if (!IsReady()) return XTW_EXPECT_SUCCESS E_UNEXPECTED;
            std::lock_guard lock(mutex_);
            return XTW_EXPECT_SUCCESS reader_->SetCurrentMediaType(stream_index, nullptr, request);
        }

        HRESULT GetStreamEnabled(_In_ DWORD stream_index)
        {
            if (!IsReady()) return XTW_EXPECT_SUCCESS E_UNEXPECTED;
            std::lock_guard lock(mutex_);
            BOOL selected{};
            HRESULT hr = reader_->GetStreamSelection(stream_index, &selected);
            if (FAILED(hr)) return hr;
            return selected ? S_OK : S_FALSE;
        }

        HRESULT SetStreamEnabled(_In_ DWORD stream_index, _In_ BOOL enabled)
        {
            if (!IsReady()) return XTW_EXPECT_SUCCESS E_UNEXPECTED;
            std::lock_guard lock(mutex_);
            return XTW_EXPECT_SUCCESS reader_->SetStreamSelection(stream_index, enabled);
        }

        HRESULT Seek(_In_ LONGLONG position)
        {
            if (!IsReady()) return XTW_EXPECT_SUCCESS E_UNEXPECTED;
            std::lock_guard lock(mutex_);
            PROPVARIANT var{};
            InitPropVariantFromInt64(position, &var);
            HRESULT hr = (XTW_EXPECT_SUCCESS reader_->SetCurrentPosition(GUID_NULL, var));
            PropVariantClear(&var);
            return hr;
        }

        std::tuple<MfSample, HRESULT> ReadSample(
            _In_ DWORD stream_index,
            _In_ DWORD control_flags,
            _Out_opt_ DWORD* out_actual_stream_index,
            _Out_opt_ DWORD* out_stream_flags,
            _Out_opt_ LONGLONG* out_timestamp)
        {
            if (!IsReady()) return {nullptr, E_UNEXPECTED};
            std::lock_guard lock(mutex_);
            xtw::com_ptr<IMFSample> sample;
            HRESULT hr = (XTW_EXPECT_SUCCESS reader_->ReadSample(stream_index, control_flags, out_actual_stream_index, out_stream_flags, out_timestamp, sample.put()));
            return {std::move(sample), hr};
        }
    };

    class MfVideoDecoder::Impl final
    {
        std::recursive_mutex mutex_{};
        std::unique_ptr<MfSourceReader> source_reader_{};
        size_t decoder_queue_depth_{};
        DWORD stream_index_{};

        bool ready_{};
        xtw::com_ptr<IMFVideoMediaType> video_media_type_{};
        MFVideoInfo video_format_{};
        LONGLONG video_duration_{};

        std::thread worker_thread_{};
        std::atomic_flag running_{};
        std::optional<xtl::concurrent_queue<MfVideoFrameSample>> decoded_frames_{};
        MfVideoFrameSample next_frame_{};

    public:
        Impl(std::unique_ptr<MfSourceReader> source, size_t queue_depth, DWORD stream_index)
            : source_reader_(std::move(source))
            , decoder_queue_depth_(queue_depth)
            , stream_index_(stream_index)
        {
            ready_ = source_reader_->IsReady();

            // Disable all streams except specified at streamIndex.
            if (ready_)
            {
                ready_ &= SUCCEEDED((source_reader_->SetStreamEnabled(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE)));
                ready_ &= SUCCEEDED((source_reader_->SetStreamEnabled(stream_index, TRUE)));
            }

            // Initialize video decoder
            if (ready_)
            {
                if (auto [media_type, _] = source_reader_->GetCurrentMediaType(stream_index); media_type)
                {
                    // Retrieve video format.
                    if (auto video_media_type = media_type.as<IMFVideoMediaType>())
                    {
                        this->video_media_type_ = video_media_type;
                        this->video_format_ = video_media_type->GetVideoFormat()->videoInfo;
                    }

                    // Request video decoder (to NV12)
                    XTW_EXPECT_SUCCESS media_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
                    ready_ &= SUCCEEDED((XTW_EXPECT_SUCCESS source_reader_->RequestMediaType(stream_index, media_type.get())));
                }
                else
                {
                    ready_ = false;
                }
            }

            // Retrieve stream duration.
            if (ready_)
            {
                auto [duration, hr] = source_reader_->GetDuration();
                this->video_duration_ = duration;
                ready_ &= SUCCEEDED(hr);
            }
        }

        Impl(const Impl& other) = delete;
        Impl(Impl&& other) noexcept = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl& operator=(Impl&& other) noexcept = delete;

        ~Impl()
        {
            if (worker_thread_.joinable())
            {
                running_.clear();
                while (decoded_frames_ && decoded_frames_->pop_wait()) {}
                worker_thread_.join();
                next_frame_ = {};
                decoded_frames_.reset();
            }
        }

        bool IsReady() const { return ready_; }
        xtw::com_ptr<IMFVideoMediaType> GetMediaType() const { return video_media_type_; }
        const MFVideoInfo& GetVideoInfo() const { return video_format_; }
        LONGLONG GetVideoDuration() const { return video_duration_; }

        bool IsEndOfStream()
        {
            std::lock_guard lock(mutex_);
            return !decoded_frames_ || decoded_frames_->closed();
        }

        void Rewind(bool looping)
        {
            if (!IsReady()) return;
            std::lock_guard lock(mutex_);

            if (worker_thread_.joinable())
            {
                running_.clear();
                while (decoded_frames_ && decoded_frames_->pop_wait()) {}
                worker_thread_.join();
                next_frame_ = {};
                decoded_frames_.reset();
            }

            XTW_EXPECT_SUCCESS source_reader_->Seek(0);

            {
                running_.test_and_set();
                decoded_frames_.emplace(decoder_queue_depth_);
                worker_thread_ = std::thread([this, looping]
                {
                    CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

                    DWORD loop_count = 0;

                    xtw::com_ptr<IMFVideoMediaType> frame_media_type_;
                    while (running_.test_and_set())
                    {
                        if (decoded_frames_->size() == decoded_frames_->capacity())
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            continue;
                        }

                        DWORD flags{};
                        auto [sample, hr] = source_reader_->ReadSample(stream_index_, 0, nullptr, &flags, nullptr);
                        XTW_EXPECT_SUCCESS hr;
                        if (FAILED(hr)) break; // ERROR

                        if (!frame_media_type_ || (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED))
                        {
                            auto [media_type, hr2] = source_reader_->GetCurrentMediaType(stream_index_);
                            XTW_EXPECT_SUCCESS hr2;
                            if (FAILED(hr2)) break; // ERROR
                            frame_media_type_ = media_type.as<IMFVideoMediaType>();
                        }

                        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
                        {
                            if (looping)
                            {
                                source_reader_->Seek(0); // rewind
                                ++loop_count;
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }

                        if (sample)
                        {
                            sample.Sample()->SetSampleTime(sample.Time() + video_duration_ * loop_count);
                            decoded_frames_->push(MfVideoFrameSample(sample.Sample(), frame_media_type_));
                        }
                    }

                    decoded_frames_->close();
                    CoUninitialize();
                });
            }

            // Wait for first frame decoded
            for (auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(5000);
                 std::chrono::high_resolution_clock::now() < timeout && decoded_frames_->empty();
                 std::this_thread::yield())
                continue;
        }

        MfVideoFrameSample FetchFrame(LONGLONG current_time)
        {
            if (!IsReady()) return nullptr;
            std::lock_guard lock(mutex_);

            // Find sample_ for the current time.
            MfVideoFrameSample current_frame{};
            while (decoded_frames_ && !decoded_frames_->closed())
            {
                if (!next_frame_)
                {
                    if (auto t = decoded_frames_->try_pop())
                    {
                        next_frame_ = *t;
                    }
                }

                if (!next_frame_ || next_frame_.Time() > current_time)
                {
                    break;
                }

                current_frame = std::exchange(next_frame_, {});
            }

            return current_frame;
        }
    };

    MfVideoDecoder::MfVideoDecoder(IMFByteStream* stream, int decoder_queue_depth, DWORD stream_index) : impl_(std::make_unique<Impl>(std::make_unique<MfSourceReader>(stream, nullptr), decoder_queue_depth, stream_index)) {}
    MfVideoDecoder::MfVideoDecoder(IMFByteStream* stream, IMFAttributes* attributes, int decoder_queue_depth, DWORD stream_index) : impl_(std::make_unique<Impl>(std::make_unique<MfSourceReader>(stream, attributes), decoder_queue_depth, stream_index)) {}
    MfVideoDecoder::MfVideoDecoder(IMFSourceReader* source, int decoder_queue_depth, DWORD stream_index) : impl_(std::make_unique<Impl>(std::make_unique<MfSourceReader>(source), decoder_queue_depth, stream_index)) {}
    MfVideoDecoder::~MfVideoDecoder() = default;
    bool MfVideoDecoder::IsReady() const { return impl_->IsReady(); }
    xtw::com_ptr<IMFVideoMediaType> MfVideoDecoder::GetMediaType() const { return impl_->GetMediaType(); }
    const MFVideoInfo& MfVideoDecoder::GetVideoInfo() const { return impl_->GetVideoInfo(); }
    LONGLONG MfVideoDecoder::GetVideoDuration() const { return impl_->GetVideoDuration(); }
    bool MfVideoDecoder::IsEndOfStream() const { return impl_->IsEndOfStream(); }
    void MfVideoDecoder::Rewind(bool looping) { return impl_->Rewind(looping); }
    MfVideoFrameSample MfVideoDecoder::FetchFrame(LONGLONG current_time) { return impl_->FetchFrame(current_time); }
}
