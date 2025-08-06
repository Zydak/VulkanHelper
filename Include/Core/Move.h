#pragma once

namespace VulkanHelper
{
    template<typename T>
    struct RemoveReference {
        using Type = T;
    };

    template<typename T>
    struct RemoveReference<T&> {
        using Type = T;
    };

    template<typename T>
    struct RemoveReference<T&&> {
        using Type = T;
    };

    template<typename T>
    using RemoveReferenceT = typename RemoveReference<T>::Type;

    template<typename T>
    constexpr RemoveReferenceT<T>&& Move(T&& t) noexcept
    {
        return (RemoveReferenceT<T>&&)t;
    }
}