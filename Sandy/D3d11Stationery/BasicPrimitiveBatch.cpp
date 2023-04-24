/// @file
///	@brief   sandy::d3d11::stationery::BasicPrimitiveBatch
///	@author  (C) 2023 ttsuki

#include "BasicPrimitiveBatch.h"

namespace sandy::d3d11
{
    static constexpr char shader_source_code_[] = R"---(

Texture2D Texture0 : register(t0);
SamplerState TexSampler : register(s0);

cbuffer MatrixVars : register(b0)
{
    float4x4 viewProjectionTransposed;
};

struct A2V
{
    float4   pos            : POSITION0;
    float4   colorMul       : COLOR0;
    float4   colorAdd       : COLOR1;
    float4   uv             : TEXCOORD0;
    float4x4 worldTransform : WORLDMATRIX;
};

struct V2P
{
    float4 pos      : SV_POSITION0;
    float4 colorMul : COLOR0;
    float4 colorAdd : COLOR1;
    float4 uv       : TEXCOORD0;
};

struct P2F
{
    float4 fragment : SV_TARGET0;
};

void vsMain(in A2V vertex, out V2P result)
{
    result.pos      = mul(mul(vertex.worldTransform, vertex.pos), viewProjectionTransposed);
    result.colorMul = vertex.colorMul;
    result.uv       = vertex.uv;
    result.colorAdd = vertex.colorAdd;
}

void psMain(in V2P pixel, out P2F result)
{
    float4 texColor0 = Texture0.Sample(TexSampler, pixel.uv.xy);
    result.fragment = pixel.colorMul * texColor0 + pixel.colorAdd;
    if (result.fragment.a < 1.0f / 256.0f) discard; // discard fully transparent pixel
}
)---";

    static constexpr D3D11_INPUT_ELEMENT_DESC input_layout_desc_[]{
        D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        D3D11_INPUT_ELEMENT_DESC{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        D3D11_INPUT_ELEMENT_DESC{"COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        D3D11_INPUT_ELEMENT_DESC{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        D3D11_INPUT_ELEMENT_DESC{"WORLDMATRIX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        D3D11_INPUT_ELEMENT_DESC{"WORLDMATRIX", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        D3D11_INPUT_ELEMENT_DESC{"WORLDMATRIX", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        D3D11_INPUT_ELEMENT_DESC{"WORLDMATRIX", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    };

    BasicPrimitiveBatch::BasicPrimitiveBatch(ID3D11Device* device)
        : factory_(device)
        , vertex_shader_code_(factory_.CompileShader(shader_source_code_, "BasicEffect.hlsl", "vsMain", "vs_4_0", nullptr, 0))
        , pixel_shader_code_(factory_.CompileShader(shader_source_code_, "BasicEffect.hlsl", "psMain", "ps_4_0", nullptr, 0))
        , input_layout_(factory_.CreateInputLayout(input_layout_desc_, std::size(input_layout_desc_), vertex_shader_code_.get()))
        , vertex_shader_(factory_.CreateVertexShader(vertex_shader_code_.get()))
        , pixel_shader_(factory_.CreatePixelShader(pixel_shader_code_.get()))
        , white_texture_(CommonTextures(factory_).White)
        , common_rasterizer_states_(factory_)
        , common_sampler_states_(factory_)
        , common_blend_states_(factory_)
        , common_depth_stencil_states_(factory_)
        , vertex_buffer_(device, 16384)
        , index_buffer_(device, 16384)
        , instance_buffer_(device, 4096)
        , vertex_shader_constant_(device, 1)
    {
        DefaultRasterizerState = common_rasterizer_states_.CullNone;
        DefaultSamplerState = common_sampler_states_.Liner;
        DefaultBlendState = common_blend_states_.AlphaBlend;
        DefaultDepthStencilState = common_depth_stencil_states_.DepthDisabled;
    }

    void BasicPrimitiveBatch::Begin(ID3D11DeviceContext* context, RenderTarget& target)
    {
        UINT u{};
        UINT zero{};

        context_ = context;
        context_->ClearState();

        // Input Assembler
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context_->IASetVertexBuffers(0, 1, vertex_buffer_.Buffer.get_address(), &(u = vertex_buffer_.Stride), &zero);
        context_->IASetVertexBuffers(1, 1, instance_buffer_.Buffer.get_address(), &(u = instance_buffer_.Stride), &zero);
        context_->IASetIndexBuffer(index_buffer_.Buffer.get(), DXGI_FORMAT_R32_UINT, 0);
        context_->IASetInputLayout(input_layout_.get());

        vertex_shader_constant_.Update(
            context_.get(),
            ViewProjectionMatrix{
                Matrix4x4(
                    2.0f / target.ScreenViewport.Width, 0.0f, 0.0f, -1.0f,
                    0.0f, -2.0f / target.ScreenViewport.Height, 0.0f, 1.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f)
            });

        // Vertex Shader Stage
        context_->VSSetShader(vertex_shader_.get(), nullptr, 0);
        context_->VSSetConstantBuffers(0, 1, vertex_shader_constant_.Buffer.get_address());

        // Rasterizer Stage
        context_->RSSetViewports(1, &target.ScreenViewport);
        context_->RSSetState(DefaultRasterizerState.get());

        // Pixel Shader Stage
        context_->PSSetShader(pixel_shader_.get(), nullptr, 0);
        context_->PSSetShaderResources(0, 1, white_texture_.get_address());
        context_->PSSetSamplers(0, 1, DefaultSamplerState.get_address());

        // Output Merger Stage
        context_->OMSetRenderTargets(1, target.RenderTargetView.get_address(), target.DepthStencilView.get());
        context_->OMSetBlendState(DefaultBlendState.get(), colors::Transparent.pointer(), ~0u);
        context_->OMSetDepthStencilState(DefaultDepthStencilState.get(), 0);
    }

    void BasicPrimitiveBatch::End()
    {
        context_ = nullptr;
    }

    void BasicPrimitiveBatch::SetViewProjectionMatrix(const ViewProjectionMatrix& matrix)
    {
        if (!context_) throw std::logic_error("not begin called");

        ViewProjectionMatrix transposed{transpose(matrix.ViewProjection)};

        vertex_shader_constant_.Update(context_.get(), &transposed, 1);
    }

    void BasicPrimitiveBatch::SetTexture(ID3D11ShaderResourceView* texture)
    {
        if (!context_) throw std::logic_error("not begin called");
        context_->PSSetShaderResources(0, 1, (texture ? texture : white_texture_).get_address());
    }

    void BasicPrimitiveBatch::SetSamplerState(ID3D11SamplerState* sampler)
    {
        if (!context_) throw std::logic_error("not begin called");
        context_->PSSetSamplers(0, 1, &sampler);
    }

    void BasicPrimitiveBatch::SetBlendState(ID3D11BlendState* blending, const Color4& blend_factor, uint32_t sample_mask)
    {
        if (!context_) throw std::logic_error("not begin called");
        context_->OMSetBlendState(blending, blend_factor.pointer(), sample_mask);
    }

    void BasicPrimitiveBatch::SetDepthStencilState(ID3D11DepthStencilState* depth_stencil, uint32_t stencil_ref)
    {
        if (!context_) throw std::logic_error("not begin called");
        context_->OMSetDepthStencilState(depth_stencil, stencil_ref);
    }

    void BasicPrimitiveBatch::DrawPrimitive(
        PrimitiveTopology topology,
        const Vertex vertices[], size_t vertex_count,
        const WorldMatrix& instance)
    {
        return DrawPrimitive(topology, vertices, vertex_count, &instance, 1);
    }

    void BasicPrimitiveBatch::DrawPrimitive(
        PrimitiveTopology topology,
        const Vertex vertices[], size_t vertex_count,
        const WorldMatrix instances[], size_t instance_count)
    {
        if (!context_) throw std::logic_error("not begin called");
        auto vertex_index = vertex_buffer_.Append(context_.get(), vertices, vertex_count);
        auto instance_index = instance_buffer_.Append(context_.get(), instances, instance_count);

        context_->IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(topology));
        context_->DrawInstanced(
            static_cast<UINT>(vertex_count),
            static_cast<UINT>(instance_count),
            static_cast<UINT>(vertex_index),
            static_cast<UINT>(instance_index));
    }

    void BasicPrimitiveBatch::DrawPrimitiveIndexed(
        PrimitiveTopology topology,
        const Vertex* vertices, size_t vertex_count,
        const Index* indices, size_t index_count,
        const WorldMatrix& instance)
    {
        return DrawPrimitiveIndexed(topology, vertices, vertex_count, indices, index_count, &instance, 1);
    }

    void BasicPrimitiveBatch::DrawPrimitiveIndexed(
        PrimitiveTopology topology,
        const Vertex vertices[], size_t vertex_count,
        const Index indices[], size_t index_count,
        const WorldMatrix instances[], size_t instance_count)
    {
        if (!context_) throw std::logic_error("not begin called");
        auto vertex_index = vertex_buffer_.Append(context_.get(), vertices, vertex_count);
        auto index_index = index_buffer_.Append(context_.get(), indices, index_count);
        auto instance_index = instance_buffer_.Append(context_.get(), instances, instance_count);

        context_->IASetPrimitiveTopology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(topology));
        context_->DrawIndexedInstanced(
            static_cast<UINT>(index_count),
            static_cast<UINT>(instance_count),
            static_cast<UINT>(index_index),
            static_cast<UINT>(vertex_index),
            static_cast<UINT>(instance_index));
    }
}
