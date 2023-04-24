/// @file
///	@brief   sandy::d3d11::SwapChain
///	@author  (C) 2023 ttsuki

#pragma once

#include <Windows.h>
#include <combaseapi.h>
#include <d3d11.h>

#include <xtw/com.h>

namespace sandy::d3d11
{
    struct SwapChain
    {
        const HWND TargetWindow{};
        const LONG BackBufferWidth{};
        const LONG BackBufferHeight{};
        const DXGI_FORMAT BackBufferFormat{};
        const xtw::com_ptr<IDXGISwapChain> DxgiSwapChain{};

        struct SwapChainPresentationStatistics
        {
            UINT FrameNumber;
            DXGI_FRAME_STATISTICS FrameStatistics;
            double ImmediatePresentsPerSecond;
        };

        SwapChain(ID3D11Device* device, HWND window, LONG width, LONG height, DXGI_FORMAT back_buffer_format);

        bool GetFullScreenState();
        void SetFullScreenState(bool fullscreen);

        void UpdateBackBufferContent(ID3D11DeviceContext* context, ID3D11Texture2D* source);

        void Present(int sync_interval = 1);
        [[nodiscard]] const SwapChainPresentationStatistics& PresentationStatistics() const noexcept { return presentation_statistics_; }

    private:
        xtw::com_ptr<ID3D11Texture2D> back_buffer_texture_{};
        xtw::com_ptr<ID3D11RenderTargetView> back_buffer_rt_view_{};
        SwapChainPresentationStatistics presentation_statistics_{};

    };
}
