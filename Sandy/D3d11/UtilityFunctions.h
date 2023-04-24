/// @file
///	@brief   sandy::d3d11::util
///	@author  (C) 2023 ttsuki

#pragma once

#include <tuple>
#include <type_traits>

namespace sandy::d3d11::util
{
    namespace detail
    {
        template <typename T>
        struct FunctionParameterTypes;

        template <typename Interface, typename R, typename... A>
        struct FunctionParameterTypes<R(__stdcall Interface::*)(A...)>
        {
            using types = std::tuple<A...>;
            template <size_t i> using type = std::tuple_element_t<i, std::tuple<A...>>;
            using first = type<0>;
        };

        template <auto Method>
        using FirstParameterType = typename FunctionParameterTypes<decltype(Method)>::first;

        template <class SmartPointer>
        using TargetTypeOf = std::remove_pointer_t<decltype(std::declval<SmartPointer>().operator->())>;
    }

    template <
        class Interface,
        auto Method = &Interface::GetDesc,
        class DescType = std::remove_pointer_t<detail::FirstParameterType<Method>>>
    static inline DescType GetDesc(Interface* ptr)
    {
        DescType d{};
        if (ptr) { (ptr->*Method)(&d); }
        return d;
    }

    template <
        class Pointer,
        class Interface = detail::TargetTypeOf<Pointer>,
        auto Method = &Interface::GetDesc,
        class DescType = std::remove_pointer_t<detail::FirstParameterType<Method>>>
    static inline DescType GetDesc(Pointer&& ptr)
    {
        return GetDesc<Interface, Method>(ptr.operator->());
    }
}
