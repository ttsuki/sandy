/// @file
///	@brief   sandy::d3d11::Device
///	@author  (C) 2023 ttsuki

#include "./Device.h"

#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>

#include <xtw/debug.h>

#define EXPECT_SUCCESS XTW_EXPECT_SUCCESS
#define THROW_ON_FAILURE XTW_THROW_ON_FAILURE
using xtw::com_ptr;

namespace sandy::d3d11
{
    static com_ptr<IDXGIFactory> CreateDxgiFactory()
    {
        com_ptr<IDXGIFactory2> factory2{};
        
#ifdef _DEBUG
        {
            com_ptr<IDXGIInfoQueue> info_queue;
            if (SUCCEEDED(EXPECT_SUCCESS DXGIGetDebugInterface1(0, IID_PPV_ARGS(info_queue.put()))))
            {
                THROW_ON_FAILURE CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(factory2.put()));

                info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
                info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
                info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, false);
                info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO, false);
                info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE, false);

                DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                {
                    80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
                };
                DXGI_INFO_QUEUE_FILTER filter = {};
                filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
                filter.DenyList.pIDList = hide;
                info_queue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
            }
        }
#endif

        com_ptr<IDXGIFactory1> factory1 = factory2;
        if (!factory1)
            THROW_ON_FAILURE(CreateDXGIFactory1(IID_PPV_ARGS(factory1.put())));

        return factory1;
    }

    static com_ptr<IDXGIAdapter> GetDxgiAdapterFrom(const com_ptr<IDXGIFactory>& factory)
    {
        if (auto factory6 = factory.as<IDXGIFactory6>())
        {
            com_ptr<IDXGIAdapter1> adapter1{};
            DXGI_ADAPTER_DESC1 desc{};
            for (UINT adapter_index = 0;
                 SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                     adapter_index,
                     DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                     IID_PPV_ARGS(adapter1.put())));
                 adapter_index++)
            {
                THROW_ON_FAILURE(adapter1->GetDesc1(&desc));
                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { continue; }
                return adapter1;
            }
        }

        if (auto factory1 = factory.as<IDXGIFactory1>())
        {
            com_ptr<IDXGIAdapter1> adapter1{};
            DXGI_ADAPTER_DESC1 desc{};
            for (UINT adapter_index = 0;
                 SUCCEEDED(factory1->EnumAdapters1(adapter_index, adapter1.put()));
                 adapter_index++)
            {
                THROW_ON_FAILURE adapter1->GetDesc1(&desc);
                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { continue; }
                return adapter1;
            }
        }

        com_ptr<IDXGIAdapter> adapter{};
        THROW_ON_FAILURE factory->EnumAdapters(0, adapter.put());
        return adapter;
    }

    static com_ptr<ID3D11Device> CreateD3D11DeviceForAdapter(
        const com_ptr<IDXGIAdapter>& adapter,
        D3D_FEATURE_LEVEL minimum_feature_level)
    {
        UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

#ifdef _DEBUG
        // Determine Debug Layer is supported.
        if (SUCCEEDED(::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_NULL, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, nullptr, nullptr, nullptr)))
            creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        // Determine DirectX hardware feature levels this app will support.
        static const D3D_FEATURE_LEVEL feature_levels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1,
        };

        UINT feat_level_count = 0;
        {
            for (; feat_level_count < static_cast<UINT>(std::size(feature_levels)); ++feat_level_count)
            {
                if (feature_levels[feat_level_count] < minimum_feature_level)
                    break;
            }
            if (feat_level_count == 0)
                throw std::out_of_range("minimum_feature_level is too high");
        }

        com_ptr<ID3D11Device> device{};
        D3D_FEATURE_LEVEL level{};

        if (HRESULT hr = ::D3D11CreateDevice(
            adapter.get(),
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            creation_flags,
            feature_levels,
            feat_level_count,
            D3D11_SDK_VERSION,
            device.put(), // Returns the Direct3D D3dDevice created.
            &level,       // Returns feature level of D3dDevice created.
            nullptr       // Returns the D3dDevice immediate context.
        ); FAILED(hr))
        {
            THROW_ON_FAILURE ::D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_WARP,
                nullptr,
                creation_flags,
                feature_levels,
                feat_level_count,
                D3D11_SDK_VERSION,
                device.put(), // Returns the Direct3D D3dDevice created.
                &level,       // Returns feature level of D3dDevice created.
                nullptr       // Returns the D3dDevice immediate context.
            );
        }

#ifdef _DEBUG
        if (com_ptr<ID3D11Debug> d3d_debug = device.as<ID3D11Debug>())
        {
            if (com_ptr<ID3D11InfoQueue> d3d_info_queue = d3d_debug.as<ID3D11InfoQueue>())
            {
                d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
                d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, false);
                d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_INFO, false);
                d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_MESSAGE, false);

                D3D11_MESSAGE_ID hide[] =
                {
                    D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                };
                D3D11_INFO_QUEUE_FILTER filter{};
                filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
                filter.DenyList.pIDList = hide;
                d3d_info_queue->AddStorageFilterEntries(&filter);
            }
        }
#endif

        return device;
    }

    com_ptr<ID3D11Device> CreateD3d11Device(D3D_FEATURE_LEVEL minimum_feature_level)
    {
        auto factory = CreateDxgiFactory();
        auto adapter = GetDxgiAdapterFrom(factory);
        auto device = CreateD3D11DeviceForAdapter(adapter, minimum_feature_level);
        return device;
    }

    com_ptr<ID3D11Device> DeviceFrom(ID3D11DeviceChild* child)
    {
        com_ptr<ID3D11Device> device;
        child->GetDevice(device.put());
        return device;
    }

    Device::Device(D3D_FEATURE_LEVEL minimum_feature_level) : Device(CreateD3d11Device(minimum_feature_level)) { }
    Device::Device(ID3D11Device* device): D3dDevice(device) { }
    Device::Device(com_ptr<ID3D11Device> device): D3dDevice(std::move(device)) { }

    Device Device::FromChild(ID3D11DeviceChild* child)
    {
        com_ptr<ID3D11Device> device;
        child->GetDevice(device.put());
        return Device(device);
    }

    com_ptr<ID3DBlob> Device::CompileShader(
        std::string_view source,
        const char* source_file_name,
        const char* entry_point_name,
        const char* target_shader_model,
        std::string* compiler_error_message,
        UINT flags) const
    {
        com_ptr<ID3DBlob> byte_code;
        com_ptr<ID3DBlob> error_message;

        HRESULT hr = ::D3DCompile(
            source.data(), source.size(),
            source_file_name,
            /*defines*/ nullptr,
            /*include*/ nullptr,
            entry_point_name,
            target_shader_model,
            flags,
            0,
            byte_code.put(),
            error_message.put());

        if (error_message)
        {
            size_t sz = error_message->GetBufferSize();
            void* msg = error_message->GetBufferPointer();
            std::string buf;
            buf.assign(static_cast<const char*>(msg), sz);

            //WIN32PP_DEBUG_LOG("CompileShader: ") << buf.c_str();

            if (compiler_error_message)
                *compiler_error_message = buf.c_str();
        }

        THROW_ON_FAILURE hr;
        return byte_code;
    }

    com_ptr<ID3D11InputLayout> Device::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC desc[], size_t count, ID3DBlob* shader_byte_code) const
    {
        com_ptr<ID3D11InputLayout> ret{};
        THROW_ON_FAILURE D3dDevice->CreateInputLayout(desc, static_cast<UINT>(count), shader_byte_code->GetBufferPointer(), shader_byte_code->GetBufferSize(), ret.put());
        return ret;
    }

    com_ptr<ID3D11VertexShader> Device::CreateVertexShader(ID3DBlob* shader_byte_code) const
    {
        com_ptr<ID3D11VertexShader> ret{};
        THROW_ON_FAILURE D3dDevice->CreateVertexShader(shader_byte_code->GetBufferPointer(), shader_byte_code->GetBufferSize(), nullptr, ret.put());
        return ret;
    }

    com_ptr<ID3D11GeometryShader> Device::CreateGeometryShader(ID3DBlob* shader_byte_code) const
    {
        com_ptr<ID3D11GeometryShader> ret{};
        THROW_ON_FAILURE D3dDevice->CreateGeometryShader(shader_byte_code->GetBufferPointer(), shader_byte_code->GetBufferSize(), nullptr, ret.put());
        return ret;
    }

    com_ptr<ID3D11PixelShader> Device::CreatePixelShader(ID3DBlob* shader_byte_code) const
    {
        com_ptr<ID3D11PixelShader> ret{};
        THROW_ON_FAILURE D3dDevice->CreatePixelShader(shader_byte_code->GetBufferPointer(), shader_byte_code->GetBufferSize(), nullptr, ret.put());
        return ret;
    }

    com_ptr<ID3D11Buffer> Device::CreateBuffer(const D3D11_BUFFER_DESC& desc, const D3D11_SUBRESOURCE_DATA initial_data[]) const
    {
        com_ptr<ID3D11Buffer> ret{};
        THROW_ON_FAILURE D3dDevice->CreateBuffer(&desc, initial_data, ret.put());
        return ret;
    }

    com_ptr<ID3D11Texture1D> Device::CreateTexture1D(const D3D11_TEXTURE1D_DESC& desc, const D3D11_SUBRESOURCE_DATA initial_data[]) const
    {
        com_ptr<ID3D11Texture1D> ret{};
        THROW_ON_FAILURE D3dDevice->CreateTexture1D(&desc, initial_data, ret.put());
        return ret;
    }

    com_ptr<ID3D11Texture2D> Device::CreateTexture2D(const D3D11_TEXTURE2D_DESC& desc, const D3D11_SUBRESOURCE_DATA initial_data[]) const
    {
        com_ptr<ID3D11Texture2D> ret{};
        THROW_ON_FAILURE D3dDevice->CreateTexture2D(&desc, initial_data, ret.put());
        return ret;
    }

    com_ptr<ID3D11Texture3D> Device::CreateTexture3D(const D3D11_TEXTURE3D_DESC& desc, const D3D11_SUBRESOURCE_DATA initial_data[]) const
    {
        com_ptr<ID3D11Texture3D> ret{};
        THROW_ON_FAILURE D3dDevice->CreateTexture3D(&desc, initial_data, ret.put());
        return ret;
    }

    com_ptr<ID3D11RenderTargetView> Device::CreateRenderTargetView(ID3D11Resource* texture, const D3D11_RENDER_TARGET_VIEW_DESC& desc) const
    {
        com_ptr<ID3D11RenderTargetView> ret{};
        THROW_ON_FAILURE D3dDevice->CreateRenderTargetView(texture, &desc, ret.put());
        return ret;
    }

    com_ptr<ID3D11DepthStencilView> Device::CreateDepthStencilView(ID3D11Resource* texture, const D3D11_DEPTH_STENCIL_VIEW_DESC& desc) const
    {
        com_ptr<ID3D11DepthStencilView> ret{};
        THROW_ON_FAILURE D3dDevice->CreateDepthStencilView(texture, &desc, ret.put());
        return ret;
    }

    com_ptr<ID3D11ShaderResourceView> Device::CreateShaderResourceView(ID3D11Resource* texture, const D3D11_SHADER_RESOURCE_VIEW_DESC& desc) const
    {
        com_ptr<ID3D11ShaderResourceView> ret{};
        THROW_ON_FAILURE D3dDevice->CreateShaderResourceView(texture, &desc, ret.put());
        return ret;
    }

    com_ptr<ID3D11UnorderedAccessView> Device::CreateUnorderedAccessView(ID3D11Resource* texture, const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc) const
    {
        com_ptr<ID3D11UnorderedAccessView> ret{};
        THROW_ON_FAILURE D3dDevice->CreateUnorderedAccessView(texture, &desc, ret.put());
        return ret;
    }

    com_ptr<ID3D11RasterizerState> Device::CreateRasterizerState(const D3D11_RASTERIZER_DESC& desc) const
    {
        com_ptr<ID3D11RasterizerState> ret{};
        THROW_ON_FAILURE D3dDevice->CreateRasterizerState(&desc, ret.put());
        return ret;
    }

    com_ptr<ID3D11SamplerState> Device::CreateSamplerState(const D3D11_SAMPLER_DESC& desc) const
    {
        com_ptr<ID3D11SamplerState> ret{};
        THROW_ON_FAILURE D3dDevice->CreateSamplerState(&desc, ret.put());
        return ret;
    }

    com_ptr<ID3D11BlendState> Device::CreateBlendState(const D3D11_BLEND_DESC& desc) const
    {
        com_ptr<ID3D11BlendState> ret{};
        THROW_ON_FAILURE D3dDevice->CreateBlendState(&desc, ret.put());
        return ret;
    }

    com_ptr<ID3D11DepthStencilState> Device::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc) const
    {
        com_ptr<ID3D11DepthStencilState> ret{};
        THROW_ON_FAILURE D3dDevice->CreateDepthStencilState(&desc, ret.put());
        return ret;
    }

    com_ptr<ID3D11DeviceContext> Device::GetImmediateContext() const
    {
        com_ptr<ID3D11DeviceContext> p;
        D3dDevice->GetImmediateContext(p.put());
        return p;
    }

    com_ptr<ID3D11DeviceContext> Device::CreateDeferredContext() const
    {
        com_ptr<ID3D11DeviceContext> p;
        THROW_ON_FAILURE D3dDevice->CreateDeferredContext(0, p.put());
        return p;
    }
}
