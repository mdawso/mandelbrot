workspace "Mandelbrot"
configurations { "Debug", "Release" }

project "Mandelbrot"
kind "ConsoleApp"
language "C++"
targetdir "bin/%{cfg.buildcfg}"

files { "src/**.hpp", "src/**.cpp" }
links { "SDL3" }

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "On"
