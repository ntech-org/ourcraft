add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

-- Tracy frame profiler (function/line sampling + manual zones)
-- Enable:  xmake f -m debug --tracy=y && xmake
-- GUI:     run `tracy`, launch ourcraft, connect (on-demand)
option("tracy")
    set_default(false)
    set_showmenu(true)
    set_description("Enable Tracy profiler (function/line-level CPU profiling)")
option_end()

add_requires("libsdl3", "glm", "enet", "zstd", "zlib", "freetype")
add_requires("openal-soft")
add_requires("rocksdb", {configs = {zstd = true, zlib = true}})
add_requires("stb", {system = false})
add_requires("glad", {system = false, configs = {extensions = "all", api = "gl=4.6"}})
add_requires("doctest", {system = false})

-- Standalone Tracy client object lib so LTO/visibility don't strip/export wrong
if has_config("tracy") then
    target("tracy-client")
        set_kind("object")
        set_languages("c++20")
        set_symbols("debug")
        add_defines("TRACY_ENABLE", "TRACY_ON_DEMAND")
        -- Prefer portable timer if invariant TSC check fails on some CPUs
        -- add_defines("TRACY_TIMER_FALLBACK")
        add_includedirs("third_party/tracy/public", {public = true})
        add_files("third_party/tracy/public/TracyClient.cpp")
        -- Don't hide Tracy API symbols
        add_cxflags("-fvisibility=default", {force = true})
        if is_plat("linux") then
            add_syslinks("pthread", "dl")
        end
    target_end()
end

local function apply_tracy()
    if has_config("tracy") then
        add_deps("tracy-client")
        add_defines("TRACY_ENABLE", "TRACY_ON_DEMAND")
        add_includedirs("third_party/tracy/public", {public = false})
        set_symbols("debug")
        if is_plat("linux") then
            add_syslinks("pthread", "dl")
        end
    end
end

target("ourcraft")
    set_kind("binary")
    set_languages("c++20")
    if is_mode("release") then
        set_optimize("fastest")
        if not has_config("tracy") then
            set_strip("all")
            set_policy("build.optimization.lto", true)
        end
    end
    add_files("src/**.cpp")
    remove_files("src/server_main.cpp")
    add_includedirs("include")
    add_packages("libsdl3", "glad", "glm", "enet", "stb", "zstd", "zlib", "rocksdb", "freetype", "openal-soft")
    apply_tracy()

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
        if not has_config("tracy") then
            set_strip("all")
            set_policy("build.optimization.lto", true)
        end
    end
    add_files("src/net/Server.cpp", "src/net/Packet.cpp", "src/net/IntegratedServer.cpp", "src/net/ServerTick.cpp", "src/net/ServerPacketHandler.cpp", "src/net/Permissions.cpp", "src/net/CommandHandler.cpp", "src/net/RegistrationManager.cpp", "src/net/ServerConfig.cpp")
    add_files("src/world/**.cpp")
    add_files("src/entities/**.cpp")
    remove_files("src/entities/PlayerTick.cpp")
    add_files("src/physics/**.cpp")
    add_files("src/util/**.cpp")
    add_files("src/inventory/**.cpp")
    add_files("src/items/**.cpp")
    add_files("src/simulation/SimulationTick.cpp")
    add_files("src/server_main.cpp")

    add_includedirs("include")
    add_packages("glm", "enet", "zstd", "zlib", "rocksdb")
    add_defines("SERVER_ONLY")
    apply_tracy()

    if is_plat("windows") then
        add_syslinks("user32", "gdi32", "shell32")
    elseif is_plat("linux") then
        add_syslinks("pthread", "dl", "m")
    end

target("ourcraft-tests")
    set_kind("binary")
    set_languages("c++20")
    if is_mode("release") then
        set_optimize("fastest")
        set_strip("all")
    end
    add_files("tests/**.cpp")
    add_files("src/world/**.cpp")
    add_files("src/physics/**.cpp")
    add_files("src/entities/**.cpp")
    remove_files("src/entities/PlayerTick.cpp")
    add_files("src/items/**.cpp")
    add_files("src/inventory/**.cpp")
    add_files("src/simulation/SimulationTick.cpp")
    add_files("src/util/Timer.cpp")
    add_includedirs("include")
    add_packages("glm", "zstd", "zlib", "rocksdb", "doctest")
    add_defines("SERVER_ONLY")

    if is_plat("linux") then
        add_syslinks("pthread", "dl", "m")
    end
