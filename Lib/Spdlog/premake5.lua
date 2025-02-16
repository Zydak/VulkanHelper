project "Spdlog"
    architecture "x64"
    kind "StaticLib"
    language "C++"
    location "build"
	staticruntime "on"
	cppdialect "C++11"

    objdir "build/obj/%{cfg.buildcfg}"
    targetdir "build/bin/%{cfg.buildcfg}"
    
    defines
    {
        "SPDLOG_COMPILED_LIB",
        "FMT_UNICODE=0"
    }

    includedirs
    {
        ".",
        "include/",
        "include/spdlog/"
    }

    files
    {
        "src/**.cpp"
    }
    buildoptions { "/MP" }

  filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "speed"

  filter "configurations:Dist"
		runtime "Release"
		optimize "speed"
