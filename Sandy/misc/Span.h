/// @file
///	@brief   sandy::span
///	@author  (C) 2023 ttsuki

#pragma once

#include <cstddef>
#include <type_traits>
#include <stdexcept>

namespace sandy
{
    template <class T>
    struct Span1d
    {
        using element_type = T;
        using void_t = std::conditional_t<std::is_const_v<T>, const void, void>;
        using byte_t = std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;

        void_t* pointer{};
        size_t width{};

        constexpr Span1d() = default;
        [[nodiscard]] constexpr size_t size() const noexcept { return width; }
        [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
        [[nodiscard]] constexpr T& cell(size_t x) const { return *static_cast<T*>(slice(x, 1).pointer); }
        [[nodiscard]] constexpr T& operator [](size_t index) const { return this->cell(index); }

        [[nodiscard]] constexpr T* data() const { return static_cast<T*>(pointer); }
        [[nodiscard]] constexpr auto begin() const { return data(); }
        [[nodiscard]] constexpr auto end() const { return data() + width; }

        [[nodiscard]] constexpr Span1d slice(size_t x, size_t w) const
        {
#ifdef _DEBUG
            if (x > this->width) throw std::out_of_range("x");
            if (x + w > this->width) throw std::out_of_range("x + w");
#endif
            return Span1d{reinterpret_cast<byte_t*>(pointer) + sizeof(T) * x, w};
        }

        template <class U>
        [[nodiscard]] constexpr Span1d<U> reinterpret_as() const
        {
            return Span1d<U>{pointer, width * sizeof(T) / sizeof(U)};
        }
    };

    template <class T>
    struct Span2d
    {
        using element_type = T;
        using void_t = std::conditional_t<std::is_const_v<T>, const void, void>;
        using byte_t = std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;

        void_t* pointer{};
        size_t width{};
        size_t height{};
        size_t width_pitch{};

        constexpr Span2d() = default;
        [[nodiscard]] constexpr size_t size() const noexcept { return height; }
        [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
        [[nodiscard]] constexpr Span1d<T> row(size_t y) const { return Span1d<T>{this->slice(0, y, 0, 1).pointer, width}; }
        [[nodiscard]] constexpr Span1d<T> operator [](size_t index) const { return this->row(index); }

        [[nodiscard]] constexpr Span2d<T> slice(size_t x, size_t y, size_t w, size_t h) const
        {
#ifdef _DEBUG
            if (x > this->width) throw std::out_of_range("x");
            if (x + w > this->width) throw std::out_of_range("x + w");
            if (y > this->height) throw std::out_of_range("y");
            if (y + h > this->height) throw std::out_of_range("y + h");
#endif
            return Span2d<T>{reinterpret_cast<byte_t*>(pointer) + width_pitch * y + sizeof(T) * x, w, h, width_pitch};
        }

        template <class U>
        [[nodiscard]] constexpr Span2d<U> reinterpret_as() const
        {
            return Span2d<U>{pointer, width * sizeof(T) / sizeof(U), height, width_pitch};
        }
    };

    template <class T>
    struct Span3d
    {
        using element_type = T;
        using void_t = std::conditional_t<std::is_const_v<T>, const void, void>;
        using byte_t = std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;

        void_t* pointer{};
        size_t width{};
        size_t height{};
        size_t depth{};
        size_t width_pitch{};
        size_t height_pitch{};

        constexpr Span3d() = default;
        [[nodiscard]] constexpr size_t size() const noexcept { return depth; }
        [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
        [[nodiscard]] constexpr Span2d<T> plane(size_t z) const { return Span2d<T>{this->slice(0, 0, z, 0, 0, 1).pointer, width, height, width_pitch}; }
        [[nodiscard]] constexpr Span2d<T> operator [](size_t index) const { return this->plane(index); }

        [[nodiscard]] constexpr Span3d<T> slice(size_t x, size_t y, size_t z, size_t w, size_t h, size_t d) const
        {
#ifdef _DEBUG
            if (x > this->width) throw std::out_of_range("x");
            if (x + w > this->width) throw std::out_of_range("x + w");
            if (y > this->height) throw std::out_of_range("y");
            if (y + h > this->height) throw std::out_of_range("y + h");
            if (z > this->depth) throw std::out_of_range("z");
            if (z + d > this->depth) throw std::out_of_range("z + d");
#endif
            return Span3d<T>{reinterpret_cast<byte_t*>(pointer) + height_pitch * z + width_pitch * y + sizeof(T) * x, w, h, d, width_pitch, height_pitch};
        }

        template <class U>
        [[nodiscard]] constexpr Span3d<U> reinterpret_as() const
        {
            return Span3d<U>{pointer, width * sizeof(T) / sizeof(U), height, depth, width_pitch, height_pitch};
        }
    };

    using ByteSpan1d = Span1d<std::byte>;
    using ByteSpan2d = Span2d<std::byte>;
    using ByteSpan3d = Span3d<std::byte>;

    template <class T> using Span = Span1d<T>;
    using ByteSpan = ByteSpan1d;
}
