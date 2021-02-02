project "Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	akkadLib = "%{wks.location}/bin/" .. outputdir .. "/Akkad/Akkad.lib"

	files {
		"src/**.h",
		"src/**.cpp"
	}
	includedirs {
		"src/",
		"%{wks.location}/Akkad/src",
		"%{IncludeDir.imgui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.json}",
		"%{IncludeDir.iconfontcpp}",
	}
		
    postbuildcommands {
	  "{ECHO} ------------ Build done, copying resources into output directory -------------------",

      "{COPY} %{wks.location}/%{prj.name}/res %{wks.location}/bin/" .. outputdir .. "/%{prj.name}/res",

	  "{COPY} %{wks.location}/GameAssembly/src %{wks.location}/bin/" .. outputdir .. "/%{prj.name}/GameAssembly/src",
	  "{COPY} %{wks.location}/GameAssembly/Build/premake5.lua %{wks.location}/bin/" .. outputdir .. "/%{prj.name}/GameAssembly",

	  "{COPY} %{wks.location}/3rdparty/glm/include %{wks.location}/bin/" .. outputdir .. "/%{prj.name}/GameAssembly/include/glm/include",
	  "{COPY} %{wks.location}/3rdparty/spdlog/include %{wks.location}/bin/" .. outputdir .. "/%{prj.name}/GameAssembly/include/spdlog/include",
	  "{COPY} %{wks.location}/3rdparty/entt/single_include %{wks.location}/bin/" .. outputdir .. "/%{prj.name}/GameAssembly/include/entt/single_include",
	  "{COPY} %{wks.location}/Akkad/src/ %{wks.location}/bin/" .. outputdir .. "/%{prj.name}/GameAssembly/include/Akkad/",

	  -- copying Akkad.lib to GameAssembly
	  "{COPY} %{wks.location}/bin/" .. outputdir .. "/Akkad/Akkad.lib %{wks.location}/bin/" .. outputdir .. "/%{prj.name}/GameAssembly"

    }

	links {
	"Akkad"
	}

		filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "AK_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "AK_RELEASE"
		runtime "Release"
		optimize "on"