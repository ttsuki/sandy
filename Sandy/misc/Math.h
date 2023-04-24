/// @file
///	@brief   sandy::math
///	@author  (C) 2023 ttsuki

#pragma once

#if !defined(DIRECTX_MATH_VERSION) //< if DirectXMath is not included
#include <DirectXMath.h>
#endif

#include <cmath>
#include <algorithm>

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#include <ark/xmm.h>
#pragma pop_macro("min")
#pragma pop_macro("max")

namespace sandy
{
    struct Vec2
    {
        arkxmm::vf32x4 v;
        using vector_bit_mask = std::integral_constant<uint8_t, 0b0011>;
        Vec2(arkxmm::vf32x4 v = arkxmm::zero<arkana::xmm::vf32x4>()) noexcept : v{v} {}
        Vec2(float x, float y, float z = 0.0f, float w = 0.0f) noexcept : v{arkxmm::f32x4(x, y, z, w)} {}
        [[nodiscard]] float x() const noexcept { return arkxmm::extract_element<0>(v); }
        [[nodiscard]] float y() const noexcept { return arkxmm::extract_element<1>(v); }
    };

    struct Vec3
    {
        arkxmm::vf32x4 v;
        using vector_bit_mask = std::integral_constant<uint8_t, 0b0111>;
        Vec3(arkxmm::vf32x4 v = arkxmm::zero<arkana::xmm::vf32x4>()) noexcept : v{v} {}
        Vec3(float x, float y, float z, float w = 0.0f) : v{arkxmm::f32x4(x, y, z, w)} {}
        [[nodiscard]] float x() const noexcept { return arkxmm::extract_element<0>(v); }
        [[nodiscard]] float y() const noexcept { return arkxmm::extract_element<1>(v); }
        [[nodiscard]] float z() const noexcept { return arkxmm::extract_element<2>(v); }
    };

    struct Vec4
    {
        arkxmm::vf32x4 v;
        using vector_bit_mask = std::integral_constant<uint8_t, 0b1111>;
        Vec4(arkxmm::vf32x4 v = arkxmm::zero<arkana::xmm::vf32x4>()) noexcept : v{v} {}
        Vec4(float x, float y, float z, float w) : v{arkxmm::f32x4(x, y, z, w)} {}
        [[nodiscard]] float x() const noexcept { return arkxmm::extract_element<0>(v); }
        [[nodiscard]] float y() const noexcept { return arkxmm::extract_element<1>(v); }
        [[nodiscard]] float z() const noexcept { return arkxmm::extract_element<2>(v); }
        [[nodiscard]] float w() const noexcept { return arkxmm::extract_element<3>(v); }
    };

    struct PositionVector
    {
        arkxmm::vf32x4 v;
        PositionVector(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f) noexcept : v{arkxmm::f32x4(x, y, z, w)} { }
        PositionVector(arkxmm::vf32x4 v) noexcept : v{v} { }
        PositionVector(Vec2 xy, float z = 0.0f, float w = 1.0f) noexcept : v{arkxmm::vf32x4{_mm_movelh_ps(xy.v.v, arkxmm::f32x4(z, w, 0.0f, 0.0f).v)}} { }
        PositionVector(Vec3 xyz, float w = 1.0f) noexcept : v{arkxmm::insert_element<3>(xyz.v, w)} { }
    };

    struct NormalVector
    {
        arkxmm::vf32x4 v;
        NormalVector(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) noexcept : v{arkxmm::f32x4(x, y, z, w)} { }
        NormalVector(arkxmm::vf32x4 v) noexcept : v{v} { }
        NormalVector(Vec2 xy, float z = 0.0f, float w = 0.0f) noexcept : v{arkxmm::vf32x4{_mm_movelh_ps(xy.v.v, arkxmm::f32x4(z, w, z, w).v)}} { }
        NormalVector(Vec3 xyz, float w = 0.0f) noexcept : v{arkxmm::insert_element<3>(xyz.v, w)} { }
    };

    template <class T> ARKXMM_API operator +(T a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T> { return T{arkxmm::operator+(a.v, b.v)}; }
    template <class T> ARKXMM_API operator -(T a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T> { return T{arkxmm::operator-(a.v, b.v)}; }
    template <class T> ARKXMM_API operator *(T a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T> { return T{arkxmm::operator*(a.v, b.v)}; }
    template <class T> ARKXMM_API operator *(T a, float b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T> { return T{arkxmm::operator*(a.v, b)}; }
    template <class T> ARKXMM_API operator *(float a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T> { return T{arkxmm::operator*(b.v, a)}; }
    template <class T> ARKXMM_API operator /(T a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T> { return T{arkxmm::operator/(a.v, b.v)}; }
    template <class T> ARKXMM_API operator /(T a, float b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T> { return T{arkxmm::operator/(a.v, b)}; }
    template <class T> ARKXMM_API operator +=(T& a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T&> { return a = a + b; }
    template <class T> ARKXMM_API operator -=(T& a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T&> { return a = a - b; }
    template <class T> ARKXMM_API operator *=(T& a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T&> { return a = a * b; }
    template <class T> ARKXMM_API operator *=(T& a, float b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T&> { return a = a * b; }
    template <class T> ARKXMM_API operator /=(T& a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T&> { return a = a / b; }
    template <class T> ARKXMM_API operator /=(T& a, float b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T&> { return a = a / b; }

    template <class T> ARKXMM_API inner_production_v(T a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, T> { return T{arkxmm::dot<T::vector_bit_mask::value>(a.v, b.v)}; }

    template <class T> ARKXMM_API dot(T a, T b) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, float>
    {
        return arkana::xmm::extract_element<0>(inner_production_v(a, b));
    }

    ARKXMM_API cross(Vec2 a, Vec2 b) noexcept -> float
    {
        constexpr auto _ = 0;
        auto a_xy = a.v;                              // a.x, a.y
        auto b_yx = arkxmm::shuffle<1, 0, _, _>(b.v); // b.y, b.x
        auto u = a_xy * b_yx;                         // a.x*b.y, a.y*b.x
        auto v = arkxmm::shuffle<1, _, _, _>(u);      // a.y*b.x
        return arkxmm::extract_element<0>(u - v);     // a.x*b.y - a.y*b.x
    }

    ARKXMM_API cross(Vec3 a, Vec3 b) noexcept -> Vec3
    {
        constexpr auto _ = 3;
        auto a_yzx = arkxmm::shuffle<1, 2, 0, _>(a.v); // a.y, a.z, a.x
        auto b_yzx = arkxmm::shuffle<1, 2, 0, _>(b.v); // b.y, b.z, b.x
        auto a_zxy = arkxmm::shuffle<2, 0, 1, _>(a.v); // a.z, a.x, a.y
        auto b_zxy = arkxmm::shuffle<2, 0, 1, _>(b.v); // b.z, b.x, b.y
        return Vec3{a_yzx * b_zxy - a_zxy * b_yzx};
    }

    template <class T> ARKXMM_API length_sq(T a) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, float> { return arkana::xmm::extract_element<0>(arkxmm::dot<T::vector_bit_mask::value>(a.v, a.v)); }
    template <class T> ARKXMM_API length(T a) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, float> { return arkana::xmm::extract_element<0>(arkxmm::sqrt(arkxmm::dot<T::vector_bit_mask::value>(a.v, a.v))); }
    template <class T> ARKXMM_API normal(T a) noexcept -> std::enable_if_t<T::vector_bit_mask::value != 0, float> { return T{a.v / arkxmm::sqrt(arkxmm::dot<T::vector_bit_mask::value>(a.v, a.v))}; }

    struct Matrix4x4
    {
        arkxmm::vf32x4 m0 = arkxmm::zero<arkxmm::vf32x4>();
        arkxmm::vf32x4 m1 = arkxmm::zero<arkxmm::vf32x4>();
        arkxmm::vf32x4 m2 = arkxmm::zero<arkxmm::vf32x4>();
        arkxmm::vf32x4 m3 = arkxmm::zero<arkxmm::vf32x4>();

        Matrix4x4() noexcept = default;

        Matrix4x4(Vec4 m0, Vec4 m1, Vec4 m2, Vec4 m3) noexcept
            : m0(m0.v), m1(m1.v), m2(m2.v), m3(m3.v) { }

        Matrix4x4(const float p[16]) noexcept
            : m0(arkxmm::load_u<arkxmm::vf32x4>(p + 0))
            , m1(arkxmm::load_u<arkxmm::vf32x4>(p + 4))
            , m2(arkxmm::load_u<arkxmm::vf32x4>(p + 8))
            , m3(arkxmm::load_u<arkxmm::vf32x4>(p + 12)) { }

        Matrix4x4(
            float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33) noexcept
            : m0(arkxmm::f32x4(m00, m01, m02, m03))
            , m1(arkxmm::f32x4(m10, m11, m12, m13))
            , m2(arkxmm::f32x4(m20, m21, m22, m23))
            , m3(arkxmm::f32x4(m30, m31, m32, m33)) { }

        explicit Matrix4x4(DirectX::XMMATRIX m) noexcept
            : m0({m.r[0]}), m1({m.r[1]}), m2({m.r[2]}), m3({m.r[3]}) { }
    };

    ARKXMM_API transpose(Matrix4x4 a) noexcept -> Matrix4x4;
    ARKXMM_API inverse(Matrix4x4 a) noexcept -> Matrix4x4;

    ARKXMM_API operator +(Matrix4x4 a, Matrix4x4 b) noexcept -> Matrix4x4 { return Matrix4x4{a.m0 + b.m0, a.m1 + b.m1, a.m2 + b.m2, a.m3 + b.m3}; }
    ARKXMM_API operator -(Matrix4x4 a, Matrix4x4 b) noexcept -> Matrix4x4 { return Matrix4x4{a.m0 - b.m0, a.m1 - b.m1, a.m2 - b.m2, a.m3 - b.m3}; }
    ARKXMM_API operator *(Matrix4x4 a, Matrix4x4 b) noexcept -> Matrix4x4;
    ARKXMM_API operator +=(Matrix4x4& a, Matrix4x4 b) noexcept -> Matrix4x4& { return a = a + b; }
    ARKXMM_API operator -=(Matrix4x4& a, Matrix4x4 b) noexcept -> Matrix4x4& { return a = a - b; }
    ARKXMM_API operator *=(Matrix4x4& a, Matrix4x4 b) noexcept -> Matrix4x4& { return a = a * b; }

    namespace matrix4x4
    {
        ARKXMM_API Zero() noexcept -> Matrix4x4 { return {}; };

        ARKXMM_API Identity() noexcept -> Matrix4x4
        {
            return {
                {1.0f, 0.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f},
                {0.0f, 0.0f, 0.0f, 1.0f},
            };
        }

        // World
        ARKXMM_API ScaleTranslate(Vec2 scale, Vec2 translate) noexcept -> Matrix4x4;
        ARKXMM_API ScaleTranslate(Vec3 scale, Vec3 translate) noexcept -> Matrix4x4;
        ARKXMM_API ScaleRollTranslate(Vec2 scale, float roll, Vec2 translate) noexcept -> Matrix4x4;
        ARKXMM_API ScaleRollTranslate(Vec3 scale, float roll, Vec3 translate) noexcept -> Matrix4x4;
        ARKXMM_API ScaleYawPitchRollTranslate(Vec3 scale, Vec3 rotate, Vec3 translate) noexcept -> Matrix4x4;

        // View
        ARKXMM_API LookTo(Vec3 camera_position, Vec3 look_to, Vec3 up) noexcept -> Matrix4x4;
        ARKXMM_API LookAt(Vec3 camera_position, Vec3 look_at, Vec3 up) noexcept -> Matrix4x4;

        // Projection
        ARKXMM_API Orthographic(float screen_width, float screen_height, float near_clip, float far_clip) noexcept -> Matrix4x4;
        ARKXMM_API PerspectiveFov(float fov_degree, float screen_width, float screen_height, float near_clip, float far_clip) noexcept -> Matrix4x4;
    }

#pragma region Matrix4x4

    ARKXMM_API transpose(Matrix4x4 a) noexcept -> Matrix4x4
    {
        auto m = a;
        arkxmm::transpose_32x4x4(m.m0, m.m1, m.m2, m.m3);
        return m;
    }

    ARKXMM_API inverse(Matrix4x4 a) noexcept -> Matrix4x4
    {
        return Matrix4x4(DirectX::XMMatrixInverse(nullptr, DirectX::XMMATRIX{a.m0.v, a.m1.v, a.m2.v, a.m3.v}));
    }

    ARKXMM_API operator *(Matrix4x4 a, Matrix4x4 b) noexcept -> Matrix4x4
    {
        auto t = transpose(b);
        return {
            dot<0b1111, 0b0001>(a.m0, t.m0) | dot<0b1111, 0b0010>(a.m0, t.m1) | dot<0b1111, 0b0100>(a.m0, t.m2) | dot<0b1111, 0b1000>(a.m0, t.m3),
            dot<0b1111, 0b0001>(a.m1, t.m0) | dot<0b1111, 0b0010>(a.m1, t.m1) | dot<0b1111, 0b0100>(a.m1, t.m2) | dot<0b1111, 0b1000>(a.m1, t.m3),
            dot<0b1111, 0b0001>(a.m2, t.m0) | dot<0b1111, 0b0010>(a.m2, t.m1) | dot<0b1111, 0b0100>(a.m2, t.m2) | dot<0b1111, 0b1000>(a.m2, t.m3),
            dot<0b1111, 0b0001>(a.m3, t.m0) | dot<0b1111, 0b0010>(a.m3, t.m1) | dot<0b1111, 0b0100>(a.m3, t.m2) | dot<0b1111, 0b1000>(a.m3, t.m3),
        };
    }

    namespace matrix4x4
    {
        ARKXMM_API Translate(Vec2 translate) noexcept -> Matrix4x4
        {
            auto zero = arkxmm::zero<arkxmm::vf32x4>();
            auto one = arkxmm::broadcast<arkxmm::vf32x4>(1.0f);
            return Matrix4x4(
                arkxmm::insert_element<0, 0>(zero, one),
                arkxmm::insert_element<1, 1>(zero, one),
                arkxmm::insert_element<2, 2>(zero, one),
                PositionVector(translate).v);
        }

        ARKXMM_API Translate(Vec3 translate) noexcept -> Matrix4x4
        {
            auto zero = arkxmm::zero<arkxmm::vf32x4>();
            auto one = arkxmm::broadcast<arkxmm::vf32x4>(1.0f);
            return Matrix4x4(
                arkxmm::insert_element<0, 0>(zero, one),
                arkxmm::insert_element<1, 1>(zero, one),
                arkxmm::insert_element<2, 2>(zero, one),
                PositionVector(translate).v);
        }

        ARKXMM_API ScaleTranslate(Vec2 scale, Vec2 translate) noexcept -> Matrix4x4
        {
            auto zero = arkxmm::zero<arkxmm::vf32x4>();
            auto one = arkxmm::broadcast<arkxmm::vf32x4>(1.0f);
            return Matrix4x4(
                arkxmm::insert_element<0, 0>(zero, scale.v),
                arkxmm::insert_element<1, 1>(zero, scale.v),
                arkxmm::insert_element<2, 2>(zero, one),
                PositionVector(translate).v);
        }

        ARKXMM_API ScaleTranslate(Vec3 scale, Vec3 translate) noexcept -> Matrix4x4
        {
            auto zero = arkxmm::zero<arkxmm::vf32x4>();
            auto one = arkxmm::broadcast<arkxmm::vf32x4>(1.0f);
            return Matrix4x4(
                arkxmm::insert_element<0, 0>(zero, scale.v),
                arkxmm::insert_element<1, 1>(zero, scale.v),
                arkxmm::insert_element<2, 2>(zero, scale.v),
                PositionVector(translate).v);
        }

        ARKXMM_API ScaleRollTranslate(Vec2 scale, float roll, Vec2 translate) noexcept -> Matrix4x4
        {
            auto zero = arkxmm::zero<arkxmm::vf32x4>();
            auto one = arkxmm::broadcast<arkxmm::vf32x4>(1.0f);
            float cos = std::cosf(roll);
            float sin = std::sinf(roll);

            return Matrix4x4(
                arkxmm::f32x4(cos, sin, 0.0f, 0.0f) * arkxmm::shuffle<0, 0, 0, 0>(scale.v),
                arkxmm::f32x4(-sin, cos, 0.0f, 0.0f) * arkxmm::shuffle<1, 1, 1, 1>(scale.v),
                arkxmm::insert_element<2, 2>(zero, one),
                PositionVector(translate).v);
        }

        ARKXMM_API ScaleRollTranslate(Vec3 scale, float roll, Vec3 translate) noexcept -> Matrix4x4
        {
            auto zero = arkxmm::zero<arkxmm::vf32x4>();
            auto one = arkxmm::broadcast<arkxmm::vf32x4>(1.0f);
            float cos = std::cosf(roll);
            float sin = std::sinf(roll);

            return Matrix4x4(
                arkxmm::f32x4(cos, sin, 0.0f, 0.0f) * arkxmm::shuffle<0, 0, 0, 0>(scale.v),
                arkxmm::f32x4(-sin, cos, 0.0f, 0.0f) * arkxmm::shuffle<1, 1, 1, 1>(scale.v),
                arkxmm::insert_element<2, 2>(zero, scale.v),
                PositionVector(translate).v);
        }

        ARKXMM_API ScaleYawPitchRollTranslate(Vec3 scale, Vec3 rotate, Vec3 translate) noexcept -> Matrix4x4
        {
            auto one = arkxmm::broadcast<arkxmm::vf32x4>(1.0f);
            auto cos = to_array(arkxmm::vf32x4{_mm_cos_ps(rotate.v.v)});
            auto sin = to_array(arkxmm::vf32x4{_mm_sin_ps(rotate.v.v)});

            return Matrix4x4(
                arkxmm::f32x4(
                    (cos[2] * cos[0] + sin[2] * sin[1] * sin[0]),
                    (sin[2] * cos[1]),
                    (cos[2] * -sin[0] + sin[2] * sin[1] * cos[0]),
                    0.0f) * arkxmm::shuffle<0, 0, 0, 0>(scale.v),
                arkxmm::f32x4(
                    (-sin[2] * cos[0] + cos[2] * sin[1] * sin[0]),
                    (cos[2] * cos[1]),
                    (-sin[2] * -sin[0] + cos[2] * sin[1] * cos[0]),
                    0.0f) * arkxmm::shuffle<1, 1, 1, 1>(scale.v),
                arkxmm::f32x4(
                    (cos[1] * sin[0]),
                    (-sin[1]),
                    (cos[1] * cos[0]),
                    0.0f) * arkxmm::shuffle<2, 2, 2, 2>(scale.v),
                PositionVector(translate).v);
        }

        ARKXMM_API LookTo(Vec3 camera_position, Vec3 look_to, Vec3 up) noexcept -> Matrix4x4
        {
            return Matrix4x4(DirectX::XMMatrixLookToLH(camera_position.v.v, look_to.v.v, up.v.v));
        }

        ARKXMM_API LookAt(Vec3 camera_position, Vec3 look_at, Vec3 up) noexcept -> Matrix4x4
        {
            return Matrix4x4(DirectX::XMMatrixLookAtLH(camera_position.v.v, look_at.v.v, up.v.v));
        }

        ARKXMM_API Orthographic(float screen_width, float screen_height, float near_clip, float far_clip) noexcept -> Matrix4x4
        {
            return Matrix4x4(DirectX::XMMatrixOrthographicLH(screen_width, screen_height, near_clip, far_clip));
        }

        ARKXMM_API Orthographic2D(float screen_width, float screen_height) noexcept -> Matrix4x4
        {
            return Matrix4x4(
                2.0f / screen_width, 0.0f, 0.0f, 0.0f,
                0.0f, -2.0f / screen_height, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, 1.0f, 0.0f, 1.0f);
        }

        ARKXMM_API PerspectiveFov(float fov_degree, float screen_width, float screen_height, float near_clip, float far_clip) noexcept -> Matrix4x4
        {
            return Matrix4x4(DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fov_degree), screen_width / screen_height, near_clip, far_clip));
        }
    }

#pragma endregion

#pragma region colors

    struct Color4 final
    {
        arkxmm::vf32x4 value{};

        Color4(arkxmm::vf32x4 v = arkxmm::zero<arkxmm::vf32x4>()) noexcept : value{v} { }
        Color4(float r, float g, float b, float a = 1.0f) noexcept : value{arkxmm::f32x4(r, g, b, a)} { }

        [[nodiscard]] float r() const noexcept { return arkxmm::extract_element<0>(value); }
        [[nodiscard]] float g() const noexcept { return arkxmm::extract_element<1>(value); }
        [[nodiscard]] float b() const noexcept { return arkxmm::extract_element<2>(value); }
        [[nodiscard]] float a() const noexcept { return arkxmm::extract_element<3>(value); }
        [[nodiscard]] const float* pointer() const noexcept { return reinterpret_cast<const float*>(value.v.m128_f32); }

        [[nodiscard]] Color4 with_red(float r) const noexcept { return Color4{arkxmm::insert_element<0>(value, r)}; }
        [[nodiscard]] Color4 with_green(float g) const noexcept { return Color4{arkxmm::insert_element<1>(value, g)}; }
        [[nodiscard]] Color4 with_blue(float b) const noexcept { return Color4{arkxmm::insert_element<2>(value, b)}; }
        [[nodiscard]] Color4 with_alpha(float a) const noexcept { return Color4{arkxmm::insert_element<3>(value, a)}; }

        [[nodiscard]] static Color4 from_rgb(uint32_t rgb) noexcept
        {
            return Color4{
                static_cast<float>(rgb >> 16 & 0xff) / 255.0f,
                static_cast<float>(rgb >> 8 & 0xff) / 255.0f,
                static_cast<float>(rgb >> 0 & 0xff) / 255.0f,
                1.0f
            };
        }

        [[nodiscard]] static Color4 from_argb(uint32_t rgb) noexcept
        {
            return Color4{
                static_cast<float>(rgb >> 16 & 0xff) / 255.0f,
                static_cast<float>(rgb >> 8 & 0xff) / 255.0f,
                static_cast<float>(rgb >> 0 & 0xff) / 255.0f,
                static_cast<float>(rgb >> 24 & 0xff) / 255.0f,
            };
        }
    };

    static inline Color4 operator +(const Color4& a, const Color4& b) noexcept { return Color4{arkxmm::operator+(a.value, b.value)}; }
    static inline Color4 operator -(const Color4& a, const Color4& b) noexcept { return Color4{arkxmm::operator-(a.value, b.value)}; }
    static inline Color4 operator *(const Color4& a, const Color4& b) noexcept { return Color4{arkxmm::operator*(a.value, b.value)}; }
    static inline Color4 operator *(const Color4& a, float b) noexcept { return Color4{ arkxmm::operator*(a.value, b) }; }
    static inline Color4 operator *(float a, const Color4& b) noexcept { return Color4{ arkxmm::operator*(b.value, a) }; }
    static inline Color4& operator +=(Color4& a, const Color4& b) noexcept { return a = a + b; }
    static inline Color4& operator -=(Color4& a, const Color4& b) noexcept { return a = a - b; }
    static inline Color4& operator *=(Color4& a, const Color4& b) noexcept { return a = a * b; }
    static inline Color4& operator *=(Color4& a, float b) noexcept { return a = a * b; }

    namespace colors
    {
        static inline Color4 MakeGray(float g, float a = 1.0f) { return Color4(g, g, g, a); }
        static const inline Color4 Transparent = MakeGray(0.0f, 0.0f);
        static const inline Color4 TransparentBlack = MakeGray(0.0f, 0.0f);
        static const inline Color4 TransparentWhite = MakeGray(1.0f, 0.0f);
        static inline Color4 FromRGB(uint32_t rgb) { return Color4::from_rgb(rgb); }
        static inline Color4 FromARGB(uint32_t rgb) { return Color4::from_argb(rgb); }

        static const inline Color4 AliceBlue = Color4::from_rgb(0xF0F8FF);
        static const inline Color4 AntiqueWhite = Color4::from_rgb(0xFAEBD7);
        static const inline Color4 Aqua = Color4::from_rgb(0x00FFFF);
        static const inline Color4 Aquamarine = Color4::from_rgb(0x7FFFD4);
        static const inline Color4 Azure = Color4::from_rgb(0xF0FFFF);
        static const inline Color4 Beige = Color4::from_rgb(0xF5F5DC);
        static const inline Color4 Bisque = Color4::from_rgb(0xFFE4C4);
        static const inline Color4 Black = Color4::from_rgb(0x000000);
        static const inline Color4 BlanchedAlmond = Color4::from_rgb(0xFFEBCD);
        static const inline Color4 Blue = Color4::from_rgb(0x0000FF);
        static const inline Color4 BlueViolet = Color4::from_rgb(0x8A2BE2);
        static const inline Color4 Brown = Color4::from_rgb(0xA52A2A);
        static const inline Color4 BurlyWood = Color4::from_rgb(0xDEB887);
        static const inline Color4 CadetBlue = Color4::from_rgb(0x5F9EA0);
        static const inline Color4 Chartreuse = Color4::from_rgb(0x7FFF00);
        static const inline Color4 Chocolate = Color4::from_rgb(0xD2691E);
        static const inline Color4 Coral = Color4::from_rgb(0xFF7F50);
        static const inline Color4 CornflowerBlue = Color4::from_rgb(0x6495ED);
        static const inline Color4 Cornsilk = Color4::from_rgb(0xFFF8DC);
        static const inline Color4 Crimson = Color4::from_rgb(0xDC143C);
        static const inline Color4 Cyan = Color4::from_rgb(0x00FFFF);
        static const inline Color4 DarkBlue = Color4::from_rgb(0x00008B);
        static const inline Color4 DarkCyan = Color4::from_rgb(0x008B8B);
        static const inline Color4 DarkGoldenRod = Color4::from_rgb(0xB8860B);
        static const inline Color4 DarkGray = Color4::from_rgb(0xA9A9A9);
        static const inline Color4 DarkGreen = Color4::from_rgb(0x006400);
        static const inline Color4 DarkKhaki = Color4::from_rgb(0xBDB76B);
        static const inline Color4 DarkMagenta = Color4::from_rgb(0x8B008B);
        static const inline Color4 DarkOliveGreen = Color4::from_rgb(0x556B2F);
        static const inline Color4 DarkOrange = Color4::from_rgb(0xFF8C00);
        static const inline Color4 DarkOrchid = Color4::from_rgb(0x9932CC);
        static const inline Color4 DarkRed = Color4::from_rgb(0x8B0000);
        static const inline Color4 DarkSalmon = Color4::from_rgb(0xE9967A);
        static const inline Color4 DarkSeaGreen = Color4::from_rgb(0x8FBC8F);
        static const inline Color4 DarkSlateBlue = Color4::from_rgb(0x483D8B);
        static const inline Color4 DarkSlateGray = Color4::from_rgb(0x2F4F4F);
        static const inline Color4 DarkTurquoise = Color4::from_rgb(0x00CED1);
        static const inline Color4 DarkViolet = Color4::from_rgb(0x9400D3);
        static const inline Color4 DeepPink = Color4::from_rgb(0xFF1493);
        static const inline Color4 DeepSkyBlue = Color4::from_rgb(0x00BFFF);
        static const inline Color4 DimGray = Color4::from_rgb(0x696969);
        static const inline Color4 DodgerBlue = Color4::from_rgb(0x1E90FF);
        static const inline Color4 FireBrick = Color4::from_rgb(0xB22222);
        static const inline Color4 FloralWhite = Color4::from_rgb(0xFFFAF0);
        static const inline Color4 ForestGreen = Color4::from_rgb(0x228B22);
        static const inline Color4 Fuchsia = Color4::from_rgb(0xFF00FF);
        static const inline Color4 Gainsboro = Color4::from_rgb(0xDCDCDC);
        static const inline Color4 GhostWhite = Color4::from_rgb(0xF8F8FF);
        static const inline Color4 Gold = Color4::from_rgb(0xFFD700);
        static const inline Color4 GoldenRod = Color4::from_rgb(0xDAA520);
        static const inline Color4 Gray = Color4::from_rgb(0x808080);
        static const inline Color4 Green = Color4::from_rgb(0x008000);
        static const inline Color4 GreenYellow = Color4::from_rgb(0xADFF2F);
        static const inline Color4 HoneyDew = Color4::from_rgb(0xF0FFF0);
        static const inline Color4 HotPink = Color4::from_rgb(0xFF69B4);
        static const inline Color4 IndianRed = Color4::from_rgb(0xCD5C5C);
        static const inline Color4 Indigo = Color4::from_rgb(0x4B0082);
        static const inline Color4 Ivory = Color4::from_rgb(0xFFFFF0);
        static const inline Color4 Khaki = Color4::from_rgb(0xF0E68C);
        static const inline Color4 Lavender = Color4::from_rgb(0xE6E6FA);
        static const inline Color4 LavenderBlush = Color4::from_rgb(0xFFF0F5);
        static const inline Color4 LawnGreen = Color4::from_rgb(0x7CFC00);
        static const inline Color4 LemonChiffon = Color4::from_rgb(0xFFFACD);
        static const inline Color4 LightBlue = Color4::from_rgb(0xADD8E6);
        static const inline Color4 LightCoral = Color4::from_rgb(0xF08080);
        static const inline Color4 LightCyan = Color4::from_rgb(0xE0FFFF);
        static const inline Color4 LightGoldenRodYellow = Color4::from_rgb(0xFAFAD2);
        static const inline Color4 LightGray = Color4::from_rgb(0xD3D3D3);
        static const inline Color4 LightGreen = Color4::from_rgb(0x90EE90);
        static const inline Color4 LightPink = Color4::from_rgb(0xFFB6C1);
        static const inline Color4 LightSalmon = Color4::from_rgb(0xFFA07A);
        static const inline Color4 LightSeaGreen = Color4::from_rgb(0x20B2AA);
        static const inline Color4 LightSkyBlue = Color4::from_rgb(0x87CEFA);
        static const inline Color4 LightSlateGray = Color4::from_rgb(0x778899);
        static const inline Color4 LightSteelBlue = Color4::from_rgb(0xB0C4DE);
        static const inline Color4 LightYellow = Color4::from_rgb(0xFFFFE0);
        static const inline Color4 Lime = Color4::from_rgb(0x00FF00);
        static const inline Color4 LimeGreen = Color4::from_rgb(0x32CD32);
        static const inline Color4 Linen = Color4::from_rgb(0xFAF0E6);
        static const inline Color4 Magenta = Color4::from_rgb(0xFF00FF);
        static const inline Color4 Maroon = Color4::from_rgb(0x800000);
        static const inline Color4 MediumAquaMarine = Color4::from_rgb(0x66CDAA);
        static const inline Color4 MediumBlue = Color4::from_rgb(0x0000CD);
        static const inline Color4 MediumOrchid = Color4::from_rgb(0xBA55D3);
        static const inline Color4 MediumPurple = Color4::from_rgb(0x9370DB);
        static const inline Color4 MediumSeaGreen = Color4::from_rgb(0x3CB371);
        static const inline Color4 MediumSlateBlue = Color4::from_rgb(0x7B68EE);
        static const inline Color4 MediumSpringGreen = Color4::from_rgb(0x00FA9A);
        static const inline Color4 MediumTurquoise = Color4::from_rgb(0x48D1CC);
        static const inline Color4 MediumVioletRed = Color4::from_rgb(0xC71585);
        static const inline Color4 MidnightBlue = Color4::from_rgb(0x191970);
        static const inline Color4 MintCream = Color4::from_rgb(0xF5FFFA);
        static const inline Color4 MistyRose = Color4::from_rgb(0xFFE4E1);
        static const inline Color4 Moccasin = Color4::from_rgb(0xFFE4B5);
        static const inline Color4 NavajoWhite = Color4::from_rgb(0xFFDEAD);
        static const inline Color4 Navy = Color4::from_rgb(0x000080);
        static const inline Color4 OldLace = Color4::from_rgb(0xFDF5E6);
        static const inline Color4 Olive = Color4::from_rgb(0x808000);
        static const inline Color4 OliveDrab = Color4::from_rgb(0x6B8E23);
        static const inline Color4 Orange = Color4::from_rgb(0xFFA500);
        static const inline Color4 OrangeRed = Color4::from_rgb(0xFF4500);
        static const inline Color4 Orchid = Color4::from_rgb(0xDA70D6);
        static const inline Color4 PaleGoldenRod = Color4::from_rgb(0xEEE8AA);
        static const inline Color4 PaleGreen = Color4::from_rgb(0x98FB98);
        static const inline Color4 PaleTurquoise = Color4::from_rgb(0xAFEEEE);
        static const inline Color4 PaleVioletRed = Color4::from_rgb(0xDB7093);
        static const inline Color4 PapayaWhip = Color4::from_rgb(0xFFEFD5);
        static const inline Color4 PeachPuff = Color4::from_rgb(0xFFDAB9);
        static const inline Color4 Peru = Color4::from_rgb(0xCD853F);
        static const inline Color4 Pink = Color4::from_rgb(0xFFC0CB);
        static const inline Color4 Plum = Color4::from_rgb(0xDDA0DD);
        static const inline Color4 PowderBlue = Color4::from_rgb(0xB0E0E6);
        static const inline Color4 Purple = Color4::from_rgb(0x800080);
        static const inline Color4 RebeccaPurple = Color4::from_rgb(0x663399);
        static const inline Color4 Red = Color4::from_rgb(0xFF0000);
        static const inline Color4 RosyBrown = Color4::from_rgb(0xBC8F8F);
        static const inline Color4 RoyalBlue = Color4::from_rgb(0x4169E1);
        static const inline Color4 SaddleBrown = Color4::from_rgb(0x8B4513);
        static const inline Color4 Salmon = Color4::from_rgb(0xFA8072);
        static const inline Color4 SandyBrown = Color4::from_rgb(0xF4A460);
        static const inline Color4 SeaGreen = Color4::from_rgb(0x2E8B57);
        static const inline Color4 SeaShell = Color4::from_rgb(0xFFF5EE);
        static const inline Color4 Sienna = Color4::from_rgb(0xA0522D);
        static const inline Color4 Silver = Color4::from_rgb(0xC0C0C0);
        static const inline Color4 SkyBlue = Color4::from_rgb(0x87CEEB);
        static const inline Color4 SlateBlue = Color4::from_rgb(0x6A5ACD);
        static const inline Color4 SlateGray = Color4::from_rgb(0x708090);
        static const inline Color4 Snow = Color4::from_rgb(0xFFFAFA);
        static const inline Color4 SpringGreen = Color4::from_rgb(0x00FF7F);
        static const inline Color4 SteelBlue = Color4::from_rgb(0x4682B4);
        static const inline Color4 Tan = Color4::from_rgb(0xD2B48C);
        static const inline Color4 Teal = Color4::from_rgb(0x008080);
        static const inline Color4 Thistle = Color4::from_rgb(0xD8BFD8);
        static const inline Color4 Tomato = Color4::from_rgb(0xFF6347);
        static const inline Color4 Turquoise = Color4::from_rgb(0x40E0D0);
        static const inline Color4 Violet = Color4::from_rgb(0xEE82EE);
        static const inline Color4 Wheat = Color4::from_rgb(0xF5DEB3);
        static const inline Color4 White = Color4::from_rgb(0xFFFFFF);
        static const inline Color4 WhiteSmoke = Color4::from_rgb(0xF5F5F5);
        static const inline Color4 Yellow = Color4::from_rgb(0xFFFF00);
        static const inline Color4 YellowGreen = Color4::from_rgb(0x9ACD32);
    }
#pragma endregion

#pragma region interpolation functions

    /// Linear interpolation
    template <class T>
    static inline constexpr T __vectorcall leap(T a, T b, float t) noexcept
    {
        return a + (b - a) * t;
    }

    /// AMD's SmoothStep interpolation
    template <class T>
    static inline constexpr T __vectorcall smooth(T a, T b, float t) noexcept
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return leap(a, b, t * t * (3.0f - 2.0f * t));
    }

    /// Barycentric interpolation
    template <class T>
    static inline constexpr T __vectorcall barycentric(T p0, T p1, T p2, float t1, float t2) noexcept
    {
        return p0 + t1 * (p1 - p0) + t2 * (p2 - p0);
    }

    /// Hermite interpolation
    template <class T>
    static inline constexpr T __vectorcall hermite(T pos0, T tan0, T pos1, T tan1, float t) noexcept
    {
        t = std::clamp(t, 0.0f, 1.0f);
        auto t2 = t * t;
        auto t3 = t2 * t;
        return (2.0f * t * 3.0f - 3.0f * t2 + 1.0f) * pos0 +
            (t3 - 2.0f * t2 + t) * tan0 +
            (-2.0f * t3 + 3.0f * t2) * pos1 +
            (t3 - t2) * tan1;
    }

    /// Catmull-Rom interpolation
    template <class T>
    static inline constexpr T __vectorcall catmull_rom(T pos0, T pos1, T pos2, T pos3, float t) noexcept
    {
        t = std::clamp(t, 0.0f, 1.0f);
        float t2 = t * t;
        float t3 = t * t2;
        return 0.5f * (
            (-t3 + 2.0f * t2 - t) * pos0 +
            (3.0f * t3 - 5.0f * t2 + 2.0f) * pos1 +
            (-3.0f * t3 + 4.0f * t2 + t) * pos2 +
            (t3 - t2) * pos3);
    }

#pragma endregion
}
