/// @file
///	@brief   sandy::d3d11::Buffer
///	@author  (C) 2023 ttsuki

#pragma once
#include <Windows.h>
#include <combaseapi.h>
#include <d3d11.h>

#include <cstddef>
#include <xtw/com.h>

#include "Device.h"

#include "../misc/Span.h"

namespace sandy::d3d11
{
    struct BufferResource
    {
        const xtw::com_ptr<ID3D11Buffer> Buffer;
        const D3D11_BUFFER_DESC Desc;

        BufferResource(const Device& device, const D3D11_BUFFER_DESC& desc);

        ByteSpan Map(ID3D11DeviceContext* context, D3D11_MAP map); // needs D3D11_CPU_ACCESS_*
        void Unmap(ID3D11DeviceContext* context);                  // needs D3D11_CPU_ACCESS_*

        void Update(ID3D11DeviceContext* context, const void* data, size_t length);                      // Whole
        void UpdatePartial(ID3D11DeviceContext* context, const void* data, size_t index, size_t length); // Partial
    };

    template <class T>
    struct TypedBufferResource : public BufferResource
    {
        using ElementType = T;
        using BufferResource::Buffer;
        using BufferResource::Desc;
        const size_t Count;
        static constexpr inline int Stride = static_cast<int>(sizeof(T));

        TypedBufferResource(const Device& device, const D3D11_BUFFER_DESC& desc)
            : BufferResource(device, desc)
            , Count(desc.ByteWidth / Stride)
        {
            //
        }

        Span<T> Map(ID3D11DeviceContext* context, D3D11_MAP map) // needs D3D11_CPU_ACCESS_*
        {
            return BufferResource::Map(context, map).reinterpret_as<T>();
        }

        void Unmap(ID3D11DeviceContext* context) // needs D3D11_CPU_ACCESS_*
        {
            return BufferResource::Unmap(context);
        }

        void Update(ID3D11DeviceContext* context, const T data[], size_t count) // Whole
        {
            if (count != Count) throw std::out_of_range("count");
            BufferResource::Update(context, data, count * Stride);
        }

        void Update(ID3D11DeviceContext* context, const T& data) // Whole
        {
            this->Update(context, std::addressof(data), 1);
        }

        void UpdatePartial(ID3D11DeviceContext* context, const T data[], size_t index, size_t count) // Partial
        {
            if (index > Count) throw std::out_of_range("index");
            if (index + count > Count) throw std::out_of_range("index + count");
            BufferResource::UpdatePartial(context, data, index * Stride, count * Stride);
        }
    };

    template <class T>
    struct ConstantBuffer : private TypedBufferResource<T>
    {
        using TypedBufferResource<T>::ElementType;
        using TypedBufferResource<T>::Buffer;
        using TypedBufferResource<T>::Desc;
        using TypedBufferResource<T>::Count;
        using TypedBufferResource<T>::Stride;

        ConstantBuffer(Device device, size_t count_of_elements) : TypedBufferResource<T>(
            device,
            CD3D11_BUFFER_DESC{
                static_cast<UINT>(sizeof(T) * count_of_elements),
                D3D11_BIND_CONSTANT_BUFFER,
            })
        {
            //
        }

        using TypedBufferResource<T>::Update;
    };

    template <class T>
    struct VertexBuffer : private TypedBufferResource<T>
    {
        using TypedBufferResource<T>::ElementType;
        using TypedBufferResource<T>::Buffer;
        using TypedBufferResource<T>::Desc;
        using TypedBufferResource<T>::Count;
        using TypedBufferResource<T>::Stride;

        VertexBuffer(Device device, size_t count_of_elements) : TypedBufferResource<T>(
            device,
            CD3D11_BUFFER_DESC{
                static_cast<UINT>(sizeof(T) * count_of_elements),
                D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER,
            })
        {
            //
        }

        using TypedBufferResource<T>::Update;
        using TypedBufferResource<T>::UpdatePartial;
    };

    template <class T>
    struct DynamicVertexBuffer : private TypedBufferResource<T>
    {
        using TypedBufferResource<T>::ElementType;
        using TypedBufferResource<T>::Buffer;
        using TypedBufferResource<T>::Desc;
        using TypedBufferResource<T>::Count;
        using TypedBufferResource<T>::Stride;
        mutable size_t Cursor{};

        DynamicVertexBuffer(Device device, size_t count_of_elements) : TypedBufferResource<T>(
            device,
            CD3D11_BUFFER_DESC{
                static_cast<UINT>(sizeof(T) * count_of_elements),
                D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER,
                D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE,
            })
        {
            //
        }

        Span<T> Map(ID3D11DeviceContext* context, size_t desired_count)
        {
            if (Cursor + desired_count <= Count)
            {
                return TypedBufferResource<T>::Map(context, D3D11_MAP_WRITE_NO_OVERWRITE).slice(Cursor, desired_count);
            }
            else if (desired_count <= Count)
            {
                Cursor = 0;
                return TypedBufferResource<T>::Map(context, D3D11_MAP_WRITE_DISCARD).slice(Cursor, desired_count);
            }
            else
            {
                throw std::out_of_range("count too large");
            }
        }

        size_t Unmap(ID3D11DeviceContext* context, size_t wrote_count)
        {
            TypedBufferResource<T>::Unmap(context);
            return std::exchange(Cursor, Cursor + wrote_count);
        }

        size_t Append(ID3D11DeviceContext* context, const T data[], size_t count)
        {
            auto view = this->Map(context, count);
            for (size_t i = 0; i < count; ++i) view[i] = *data++;
            return this->Unmap(context, count);
        }
    };

    template <class T> using IndexBuffer = VertexBuffer<T>;
    template <class T> using DynamicIndexBuffer = DynamicVertexBuffer<T>;
}
