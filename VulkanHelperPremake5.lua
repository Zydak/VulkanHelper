globalIncludes = 
{
    "Src/",

    "Lib/Glfw/include/",

    "Lib/VulkanHpp/vulkan/",
    "Lib/VulkanHpp/VulkanHeaders/include/",

    "Lib/Spdlog/include/",

    "Lib/Vma/",

    "Lib/Dxc/inc/",

    "Lib/Entt/",

    "Lib/Glm/",

    "Lib/StbImage/",

    "Lib/Assimp/include",

}

globalDefines = 
{
    "FMT_UNICODE=0"
}

include "lib/Spdlog"

project "VulkanHelper"
	architecture "x86_64"
    kind "StaticLib"
    language "C++"
	cppdialect "C++20"
	staticruntime "on"

    targetdir ("%{wks.location}/Bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinInt/" .. outputdir .. "/%{prj.name}")

    pchheader "Pch.h"
    pchsource "Src/Pch.cpp"

    libdirs
    {
        "Lib/",
    }

    defines
    {
        globalDefines,
    }

    files 
    {
        "Src/**.cpp",
        "Src/**.h",
    }

    includedirs 
    {
        globalIncludes,
    }

    links
    {
        "Lib/VulkanHpp/vulkan-1",
        "Lib/Dxc/lib/x64/dxcompiler",

        "Glfw/glfw3_mt",
        "Spdlog",
        "Lib/Assimp/lib/x64/assimp-vc143-mt",
    }
    
    buildoptions { "/MP" }

    filter "platforms:Windows"
        system "Windows"
        defines { "WIN" }

    filter "platforms:Linux"
        system "Linux"
        defines "LIN"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "RELEASE" }
		runtime "Release"
        optimize "Full"

    filter "configurations:Distribution"
		defines "DISTRIBUTION"
		runtime "Release"
		optimize "Full"