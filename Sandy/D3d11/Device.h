/// @file
///	@brief   sandy::d3d11::Device
///	@author  (C) 2023 ttsuki

#pragma once
#include <Windows.h>
#include <combaseapi.h>
#include <d3d11.h>

#include <xtw/com.h>

namespace sandy::d3d11
{
    xtw::com_ptr<ID3D11Device> CreateD3d11Device(D3D_FEATURE_LEVEL minimum_feature_level = D3D_FEATURE_LEVEL_11_0);
    xtw::com_ptr<ID3D11Device> DeviceFrom(ID3D11DeviceChild* child);

    struct Device
    {
        const xtw::com_ptr<ID3D11Device> D3dDevice{};
        Device(D3D_FEATURE_LEVEL minimum_feature_level = D3D_FEATURE_LEVEL_11_0);

        Device(ID3D11Device* device);
        Device(xtw::com_ptr<ID3D11Device> device);
        static Device FromChild(ID3D11DeviceChild* child);

        [[nodiscard]] xtw::com_ptr<ID3DBlob> CompileShader(std::string_view source, const char* source_file_name, const char* entry_point_name, const char* target_shader_model, std::string* compiler_error_message, UINT flags) const;
        [[nodiscard]] xtw::com_ptr<ID3D11InputLayout> CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC desc[], size_t count, ID3DBlob* shader_byte_code) const;
        [[nodiscard]] xtw::com_ptr<ID3D11VertexShader> CreateVertexShader(ID3DBlob* shader_byte_code) const;
        [[nodiscard]] xtw::com_ptr<ID3D11GeometryShader> CreateGeometryShader(ID3DBlob* shader_byte_code) const;
        [[nodiscard]] xtw::com_ptr<ID3D11PixelShader> CreatePixelShader(ID3DBlob* shader_byte_code) const;
        [[nodiscard]] xtw::com_ptr<ID3D11Buffer> CreateBuffer(const D3D11_BUFFER_DESC& desc, const D3D11_SUBRESOURCE_DATA initial_data[] = nullptr) const;
        [[nodiscard]] xtw::com_ptr<ID3D11Texture1D> CreateTexture1D(const D3D11_TEXTURE1D_DESC& desc, const D3D11_SUBRESOURCE_DATA initial_data[] = nullptr) const;
        [[nodiscard]] xtw::com_ptr<ID3D11Texture2D> CreateTexture2D(const D3D11_TEXTURE2D_DESC& desc, const D3D11_SUBRESOURCE_DATA initial_data[] = nullptr) const;
        [[nodiscard]] xtw::com_ptr<ID3D11Texture3D> CreateTexture3D(const D3D11_TEXTURE3D_DESC& desc, const D3D11_SUBRESOURCE_DATA initial_data[] = nullptr) const;
        [[nodiscard]] xtw::com_ptr<ID3D11RenderTargetView> CreateRenderTargetView(ID3D11Resource* texture, const D3D11_RENDER_TARGET_VIEW_DESC& desc) const;
        [[nodiscard]] xtw::com_ptr<ID3D11DepthStencilView> CreateDepthStencilView(ID3D11Resource* texture, const D3D11_DEPTH_STENCIL_VIEW_DESC& desc) const;
        [[nodiscard]] xtw::com_ptr<ID3D11ShaderResourceView> CreateShaderResourceView(ID3D11Resource* texture, const D3D11_SHADER_RESOURCE_VIEW_DESC& desc) const;
        [[nodiscard]] xtw::com_ptr<ID3D11UnorderedAccessView> CreateUnorderedAccessView(ID3D11Resource* texture, const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc) const;
        [[nodiscard]] xtw::com_ptr<ID3D11RasterizerState> CreateRasterizerState(const D3D11_RASTERIZER_DESC& desc) const;
        [[nodiscard]] xtw::com_ptr<ID3D11SamplerState> CreateSamplerState(const D3D11_SAMPLER_DESC& desc) const;
        [[nodiscard]] xtw::com_ptr<ID3D11BlendState> CreateBlendState(const D3D11_BLEND_DESC& desc) const;
        [[nodiscard]] xtw::com_ptr<ID3D11DepthStencilState> CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& desc) const;

        [[nodiscard]] xtw::com_ptr<ID3D11DeviceContext> GetImmediateContext() const;
        [[nodiscard]] xtw::com_ptr<ID3D11DeviceContext> CreateDeferredContext() const;
    };
}
