/// @file
///	@brief   sandy::d3d11::Buffer
///	@author  (C) 2023 ttsuki

#include "./Buffer.h"

#include <Windows.h>
#include <d3d11.h>
#include <cstddef>
#include <xtw/com.h>
#include <xtw/debug.h>

#include "Device.h"
#include "UtilityFunctions.h"

#include "../misc/Span.h"

namespace sandy::d3d11
{
    BufferResource::BufferResource(const Device& device, const D3D11_BUFFER_DESC& desc)
        : Buffer(device.CreateBuffer(desc))
        , Desc(util::GetDesc(Buffer))
    {
        //
    }

    void BufferResource::Update(ID3D11DeviceContext* context, const void* data, size_t length)
    {
        if (length != Desc.ByteWidth) throw std::out_of_range("length");
        context->UpdateSubresource(Buffer.get(), 0, nullptr, data, Desc.ByteWidth, Desc.ByteWidth);
    }

    void BufferResource::UpdatePartial(ID3D11DeviceContext* context, const void* data, size_t index, size_t length)
    {
        if (index < Desc.ByteWidth) throw std::out_of_range("index");
        if (length + length < Desc.ByteWidth) throw std::out_of_range("index + length");
        const D3D11_BOX box = {static_cast<UINT>(index), 0, 0, static_cast<UINT>(index + length), 1, 1};
        context->UpdateSubresource(Buffer.get(), 0, &box, data, Desc.ByteWidth, Desc.ByteWidth);
    }

    Span<std::byte> BufferResource::Map(ID3D11DeviceContext* context, D3D11_MAP map)
    {
        D3D11_MAPPED_SUBRESOURCE res{};
        XTW_THROW_ON_FAILURE context->Map(Buffer.get(), 0, map, 0, &res);
        return {res.pData, Desc.ByteWidth};
    }

    void BufferResource::Unmap(ID3D11DeviceContext* context)
    {
        context->Unmap(Buffer.get(), 0);
    }
}
