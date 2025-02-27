globalIncludes = 
{
    "%{wks.location}/Src/",

    "%{wks.location}/Lib/Glfw/include/",

    "%{wks.location}/Lib/VulkanHpp/vulkan/",
    "%{wks.location}/Lib/VulkanHpp/VulkanHeaders/include/",

    "%{wks.location}/Lib/Spdlog/include/",

    "%{wks.location}/Lib/Vma/",

    "%{wks.location}/Lib/Slang/include/",
    "%{wks.location}/Lib/Dxc/inc/",
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
        "Lib/Slang/lib/slang",
        "Lib/Dxc/lib/x64/dxcompiler",

        "Glfw/glfw3_mt",
        "Spdlog"
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