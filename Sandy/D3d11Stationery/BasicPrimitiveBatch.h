/// @file
///	@brief   sandy::d3d11::stationery::BasicPrimitiveBatch
///	@author  (C) 2023 ttsuki

#pragma once
#include <Windows.h>
#include <combaseapi.h>
#include <d3d11.h>

#include <xtw/com.h>

#include "../d3d11/Buffer.h"
#include "../d3d11/RenderTarget.h"
#include "CommonStateObjects.h"

#include "../misc/Math.h"

namespace sandy::d3d11
{
    enum struct PrimitiveTopology
    {
        PointList = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
        LineList = D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
        LineStrip = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
        TriangleList = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
        TriangleStrip = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
    };

    struct alignas(16) PositionColoredTextured
    {
        PositionVector Position{};
        Color4 Color0{};
        Color4 Color1{};
        Vec2 Texture{};
    };

    struct alignas(16) WorldMatrix
    {
        Matrix4x4 World;
    };

    struct alignas(16) ViewProjectionMatrix
    {
        Matrix4x4 ViewProjection;
    };

    static_assert(std::is_trivially_copyable_v<PositionColoredTextured> && sizeof(PositionColoredTextured) == 64 );
    static_assert(std::is_trivially_copyable_v<WorldMatrix> && sizeof(WorldMatrix) == 64);
    static_assert(std::is_trivially_copyable_v<ViewProjectionMatrix> && sizeof(ViewProjectionMatrix) == 64);

    class BasicPrimitiveBatch
    {
    public:
        using Vertex = PositionColoredTextured;
        using Index = uint32_t;

    private:
        const Device factory_;
        const xtw::com_ptr<ID3DBlob> vertex_shader_code_;
        const xtw::com_ptr<ID3DBlob> pixel_shader_code_;
        const xtw::com_ptr<ID3D11InputLayout> input_layout_;
        const xtw::com_ptr<ID3D11VertexShader> vertex_shader_;
        const xtw::com_ptr<ID3D11PixelShader> pixel_shader_;
        const xtw::com_ptr<ID3D11ShaderResourceView> white_texture_;
        const CommonRasterizerStates common_rasterizer_states_;
        const CommonSamplerState common_sampler_states_;
        const CommonBlendState common_blend_states_;
        const CommonDepthStencilState common_depth_stencil_states_;

        DynamicVertexBuffer<Vertex> vertex_buffer_;
        DynamicVertexBuffer<Index> index_buffer_;
        DynamicVertexBuffer<WorldMatrix> instance_buffer_;
        ConstantBuffer<ViewProjectionMatrix> vertex_shader_constant_;

        xtw::com_ptr<ID3D11DeviceContext> context_;

    public:
        BasicPrimitiveBatch(ID3D11Device* device);
        BasicPrimitiveBatch(const BasicPrimitiveBatch& other) = delete;
        BasicPrimitiveBatch(BasicPrimitiveBatch&& other) noexcept = delete;
        BasicPrimitiveBatch& operator=(const BasicPrimitiveBatch& other) = delete;
        BasicPrimitiveBatch& operator=(BasicPrimitiveBatch&& other) noexcept = delete;
        virtual ~BasicPrimitiveBatch() = default;

        xtw::com_ptr<ID3D11RasterizerState> DefaultRasterizerState = common_rasterizer_states_.CullNone;
        xtw::com_ptr<ID3D11SamplerState> DefaultSamplerState = common_sampler_states_.Liner;
        xtw::com_ptr<ID3D11BlendState> DefaultBlendState = common_blend_states_.AlphaBlend;
        xtw::com_ptr<ID3D11DepthStencilState> DefaultDepthStencilState = common_depth_stencil_states_.DepthDisabled;

        void Begin(ID3D11DeviceContext* context, RenderTarget& target);
        void End();

        // Vertex Shader Stage
        void SetViewProjectionMatrix(const ViewProjectionMatrix& matrix);
        void SetViewProjectionMatrix(const Matrix4x4& view, const Matrix4x4& projection) { return SetViewProjectionMatrix(ViewProjectionMatrix{view * projection}); }

        // Pixel Shader Stage
        void SetTexture(ID3D11ShaderResourceView* texture);
        void SetSamplerState(ID3D11SamplerState* sampler);
        void SetSamplerState_Point() { return this->SetSamplerState(common_sampler_states_.Point.get()); }
        void SetSamplerState_Liner() { return this->SetSamplerState(common_sampler_states_.Liner.get()); }
        void SetSamplerState_Anisotropic() { return this->SetSamplerState(common_sampler_states_.Anisotropic.get()); }

        // Output Merger
        void SetBlendState(ID3D11BlendState* blending, const Color4& blend_factor = colors::Transparent, uint32_t sample_mask = ~0u);
        void SetBlendState_Copy() { return this->SetBlendState(common_blend_states_.Copy.get()); }
        void SetBlendState_AlphaBlend() { return this->SetBlendState(common_blend_states_.AlphaBlend.get()); }
        void SetBlendState_AddBlend() { return this->SetBlendState(common_blend_states_.AddBlend.get()); }
        void SetBlendState_Multiply() { return this->SetBlendState(common_blend_states_.Multiply.get()); }
        void SetBlendState_SubtractiveBlend() { return this->SetBlendState(common_blend_states_.SubtractiveBlend.get()); }
        void SetDepthStencilState(ID3D11DepthStencilState* depth_stencil, uint32_t stencil_ref = 0);
        void SetDepthStencilState_DepthEnabled() { return this->SetDepthStencilState(common_depth_stencil_states_.DepthEnabled.get()); }
        void SetDepthStencilState_DepthDisabled() { return this->SetDepthStencilState(common_depth_stencil_states_.DepthDisabled.get()); }
        void SetDepthStencilState_DepthReadOnly() { return this->SetDepthStencilState(common_depth_stencil_states_.DepthReadOnly.get()); }

        // Draw API
        void DrawPrimitive(PrimitiveTopology topology, const Vertex vertices[], size_t vertex_count, const WorldMatrix& instance);
        void DrawPrimitive(PrimitiveTopology topology, const Vertex vertices[], size_t vertex_count, const WorldMatrix instances[], size_t instance_count);
        void DrawPrimitiveIndexed(PrimitiveTopology topology, const Vertex vertices[], size_t vertex_count, const Index indices[], size_t index_count, const WorldMatrix& instance);
        void DrawPrimitiveIndexed(PrimitiveTopology topology, const Vertex vertices[], size_t vertex_count, const Index indices[], size_t index_count, const WorldMatrix instances[], size_t instance_count);
    };
}
