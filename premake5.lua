workspace "Mandelbrot"
configurations { "Debug", "Release" }

project "Mandelbrot"
kind "ConsoleApp"
language "C++"
cppdialect "C++23"
targetdir "bin/%{cfg.buildcfg}"

files { "src/**.hpp", "src/**.cpp" }
links { "SDL3" }

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "On"
