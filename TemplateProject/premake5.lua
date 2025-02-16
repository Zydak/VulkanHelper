project "TemplateProject"
	architecture "x64"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("%{wks.location}/Bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/BinInt/" .. outputdir .. "/%{prj.name}")

	files
	{
		"Src/**.h",
		"Src/**.cpp"
	}

    includedirs
	{
		globalIncludes,
    }

	links
	{
		"VulkanHelper",
	}

	buildoptions { "/MP" }

	filter "system:windows"
		defines "WIN"
		systemversion "latest"

	filter "configurations:Debug"
		defines "DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "RELEASE"
		runtime "Release"
		optimize "Full"

	filter "configurations:Distribution"
		defines "DISTRIBUTION"
		runtime "Release"
		optimize "Full"