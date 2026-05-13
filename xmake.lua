add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

add_requires("libsdl3", "glm", "enet", "zstd", "zlib", "freetype")
add_requires("openal-soft")
add_requires("rocksdb", {configs = {zstd = true, zlib = true}})
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
    remove_files("src/server_main.cpp")
    add_includedirs("include")
    add_packages("libsdl3", "glad", "glm", "enet", "stb", "zstd", "zlib", "rocksdb", "freetype", "openal-soft")

    if is_plat("windows") then
        add_syslinks("user32", "gdi32", "shell32")
    elseif is_plat("linux") then
        add_syslinks("pthread", "dl", "m")
    end

target("ourcraft-server")
    set_kind("binary")
    set_languages("c++20")
    if is_mode("release") then
        set_optimize("fastest")
        set_strip("all")
        set_policy("build.optimization.lto", true)
    end
    add_files("src/net/Server.cpp", "src/net/Packet.cpp", "src/net/IntegratedServer.cpp", "src/net/ServerTick.cpp")
    add_files("src/world/**.cpp")
    add_files("src/entities/**.cpp")
    remove_files("src/entities/PlayerTick.cpp")
    add_files("src/physics/**.cpp")
    add_files("src/util/**.cpp")
    add_files("src/inventory/**.cpp")
    add_files("src/items/**.cpp")
    add_files("src/server_main.cpp")
    
    -- remove_files("src/world/ChunkLoader.cpp") -- Restored as it doesn't depend on GL
    
    add_includedirs("include")
    add_packages("glm", "enet", "zstd", "zlib", "rocksdb")
    add_defines("SERVER_ONLY")

    if is_plat("windows") then
        add_syslinks("user32", "gdi32", "shell32")
    elseif is_plat("linux") then
        add_syslinks("pthread", "dl", "m")
    end
