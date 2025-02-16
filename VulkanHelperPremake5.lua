globalIncludes = 
{
    "%{wks.location}/Src/",

    "%{wks.location}/Lib/Glfw/include/",
    "%{wks.location}/Lib/VulkanHpp/vulkan/",
    "%{wks.location}/Lib/VulkanHpp/VulkanHeaders/include/",
}

project "VulkanHelper"
	architecture "x64"
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
        "Glfw/glfw3_mt",
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