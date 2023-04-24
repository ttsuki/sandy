/// @file
///	@brief   sandy::d3d11::SwapChain
///	@author  (C) 2023 ttsuki

#include "SwapChain.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <xtw/com.h>
#include <xtw/debug.h>

namespace sandy::d3d11
{
    static xtw::com_ptr<IDXGISwapChain> CreateSwapChain(
        xtw::com_ptr<IDXGIDevice> device,
        HWND target,
        LONG back_buffer_width,
        LONG back_buffer_height,
        DXGI_FORMAT back_buffer_format)
    {
        xtw::com_ptr<IDXGIAdapter> adapter;
        xtw::com_ptr<IDXGIFactory2> factory2;
        XTW_THROW_ON_FAILURE device->GetAdapter(adapter.put());
        XTW_THROW_ON_FAILURE adapter->GetParent(IID_PPV_ARGS(factory2.put()));

        xtw::com_ptr<IDXGIFactory4> factory4 = factory2.as<IDXGIFactory4>();

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = static_cast<UINT>(back_buffer_width);
        desc.Height = static_cast<UINT>(back_buffer_height);
        desc.Format = back_buffer_format;
        desc.Stereo = FALSE;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 3;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fs_desc = {};
        fs_desc.Windowed = TRUE;

        xtw::com_ptr<IDXGISwapChain1> swap_chain1{};
        XTW_THROW_ON_FAILURE factory2->CreateSwapChainForHwnd(
            device.get(),
            target,
            &desc,
            &fs_desc,
            nullptr,
            swap_chain1.put());

        // Mute Alt + Enter
        XTW_THROW_ON_FAILURE factory2->MakeWindowAssociation(target, DXGI_MWA_NO_ALT_ENTER);

        return swap_chain1;
    }

    SwapChain::SwapChain(ID3D11Device* device, HWND window, LONG width, LONG height, DXGI_FORMAT back_buffer_format)
        : TargetWindow(window)
        , BackBufferWidth(width)
        , BackBufferHeight(height)
        , BackBufferFormat(back_buffer_format)
        , DxgiSwapChain(CreateSwapChain(xtw::com_ptr(device).as<IDXGIDevice>(), window, width, height, back_buffer_format))
    {
        SetFullScreenState(false);
    }

    bool SwapChain::GetFullScreenState()
    {
        BOOL state = false;
        XTW_EXPECT_SUCCESS DxgiSwapChain->GetFullscreenState(&state, nullptr);
        return state;
    }

    void SwapChain::SetFullScreenState(bool fullscreen)
    {
        // Reset swap chain
        back_buffer_rt_view_.reset();
        back_buffer_texture_.reset();
        DxgiSwapChain->SetFullscreenState(fullscreen, nullptr);

        // Re-create back buffer
        XTW_THROW_ON_FAILURE DxgiSwapChain->ResizeBuffers(
            3,
            BackBufferWidth,
            BackBufferHeight,
            BackBufferFormat,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

        // Re-create render target view
        xtw::com_ptr<ID3D11Device> device{};
        XTW_THROW_ON_FAILURE DxgiSwapChain->GetDevice(IID_PPV_ARGS(device.put()));
        XTW_THROW_ON_FAILURE DxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(back_buffer_texture_.put()));
        auto desc = CD3D11_RENDER_TARGET_VIEW_DESC(D3D11_RTV_DIMENSION_TEXTURE2D, BackBufferFormat);
        XTW_THROW_ON_FAILURE device->CreateRenderTargetView(back_buffer_texture_.get(), &desc, back_buffer_rt_view_.put());
    }

    void SwapChain::UpdateBackBufferContent(ID3D11DeviceContext* context, ID3D11Texture2D* source)
    {
        // Copy RenderTargetTexture to BackBuffer
        context->ResolveSubresource(
            back_buffer_texture_.get(), 0,
            source, 0,
            BackBufferFormat);
    }

    void SwapChain::Present(int sync_interval)
    {
        if (HRESULT hr = DxgiSwapChain->Present(sync_interval, 0); SUCCEEDED(hr))
        {
            presentation_statistics_.FrameNumber += 1;

            const auto before = presentation_statistics_.FrameStatistics;
            DxgiSwapChain->GetFrameStatistics(&presentation_statistics_.FrameStatistics);
            const auto after = presentation_statistics_.FrameStatistics;

            LARGE_INTEGER qpf{};
            QueryPerformanceFrequency(&qpf);
            presentation_statistics_.ImmediatePresentsPerSecond = static_cast<double>(qpf.QuadPart) / static_cast<double>(after.SyncQPCTime.QuadPart - before.SyncQPCTime.QuadPart);
        }
        else
        {
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // TODO: Handle Device lost.
                Sleep(1);
            }
            else
            {
                // Try recreate back buffer.
                SetFullScreenState(GetFullScreenState());
                Sleep(1);
            }
        }
    }
}
