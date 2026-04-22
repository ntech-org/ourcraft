add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_requires("glfw", "glm", "enet")
add_requires("stb", {system = false})
add_requires("glad", {system = false, configs = {extensions = "all", api = "gl=3.3"}})

target("ourcraft")
    set_kind("binary")
    set_languages("c++20")
    if is_mode("release") then
        set_optimize("fastest")
        set_strip("all")
        set_policy("build.optimization.lto", true)
    end
    add_files("src/**.cpp")
    add_includedirs("include")
    add_packages("glfw", "glad", "glm", "enet", "stb")

    if is_plat("windows") then
        add_syslinks("user32", "gdi32", "shell32")
    elseif is_plat("linux") then
        add_syslinks("pthread", "dl", "m")
    end
