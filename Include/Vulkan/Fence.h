#pragma
#include "Core/UniquePtr.h"
#include "Core/Move.h"

namespace VulkanHelper
{
    class Fence
    {
    public:

    private:
        class Impl;
        UniquePtr<Impl> m_Impl;

        Fence(UniquePtr<Impl>&& impl)
            : m_Impl(VulkanHelper::Move(impl))
        {}
    };
}