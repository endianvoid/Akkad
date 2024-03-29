project "Runtime"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	debugdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

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
		"%{IncludeDir.box2d}",
		"%{IncludeDir.stb}",
		"%{IncludeDir.concurrentqueue}",
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
	
	if _OPTIONS['target-emscripten'] then
		webmodules = path.getabsolute("WebModules/include_assets.js")
		GameAssembly = path.getabsolute("gameassembly/libGameAssembly.a")
		Common = path.getabsolute("gameassembly/libCommon.a")
		LZ4 = path.getabsolute("gameassembly/liblz4.a")
		executable_suffix (".html")
		if _OPTIONS['em-debug'] then
			buildoptions   {"-O0", "-fPIC", "-pthread","-s NO_DISABLE_EXCEPTION_CATCHING", "--profiling", "-s ASSERTIONS=1"}
			linkoptions    {"-O0", "-s EXCEPTION_DEBUG=1", "-s PROXY_TO_PTHREAD", "-s ASSERTIONS=1","-s NO_DISABLE_EXCEPTION_CATCHING", "--profiling", "-s FETCH=1", "-fPIC", "-s TOTAL_MEMORY=1024MB", "-pthread", "-s PTHREAD_POOL_SIZE=3", "-s USE_PTHREADS=1", "-s ALLOW_TABLE_GROWTH", "-s EXPORTED_RUNTIME_METHODS=['FS']", "-s EXPORTED_FUNCTIONS=['_InitGameAssembly','_main','__embind_register_function']", "-lembind", "-s DYNCALLS=1", "-s USE_WEBGL2=1", "-s OFFSCREEN_FRAMEBUFFER=1","-s FORCE_FILESYSTEM=1", "--pre-js=" .. webmodules .. " " .. GameAssembly .. " " .. Common .. " " .. LZ4}
		else
			buildoptions   {"-O3", "-fPIC", "-pthread"}
			linkoptions    {"-O3",  "-s PROXY_TO_PTHREAD", "-s FETCH=1", "-fPIC", "-s TOTAL_MEMORY=1024MB", "-pthread", "-s PTHREAD_POOL_SIZE=3", "-s USE_PTHREADS=1", "-s ALLOW_TABLE_GROWTH", "-s EXPORTED_RUNTIME_METHODS=['FS','ccall','cwrap']", "-s EXPORTED_FUNCTIONS=['_InitGameAssembly','_main']", "-lembind", "-s DYNCALLS=1", "-s USE_WEBGL2=1", "-s OFFSCREEN_FRAMEBUFFER=1","-s FORCE_FILESYSTEM=1", "--pre-js=" .. webmodules .. " " .. GameAssembly .. " " .. Common .. " " .. LZ4}
		end

	end