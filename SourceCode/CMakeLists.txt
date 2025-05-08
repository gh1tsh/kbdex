project(
    kbdexModules
    DESCRIPTION "kbdexModules"
)


set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(LIBNOTIFY REQUIRED IMPORTED_TARGET libnotify)

# TODO: добавить библиотеки, необходимые моему проекту: libinput, libevdev, libudev


set(KBDEX_CORE_SRC
    hawck-macrod.cpp
    RemoteUDevice.cpp
    Daemon.cpp
    MacroDaemon.cpp
    LuaUtils.cpp
    Keyboard.cpp
    FSWatcher.cpp
    Permissions.cpp
    FIFOWatcher.cpp
    LuaConfig.cpp
    XDG.cpp
    KBDB.cpp
    Popen.cpp
)

add_executable(hawck-macrod ${KBDEX_CORE_SRC})
target_link_libraries(kbdexCore
    PkgConfig::LIBNOTIFY
    Threads::Threads
)
# target_include_directories(hawck-macrod PRIVATE ${LUA_INCLUDE_DIRS})

# install(TARGETS hawck-macrod)

set(KBDEX_KEYBOARD_AGENT_SRC
    hawck-inputd.cpp
    UDevice.cpp
    Version.cpp
    Daemon.cpp
    KBDDaemon.cpp
    Keyboard.cpp
    FSWatcher.cpp
    CSV.cpp
    Permissions.cpp
    LuaUtils.cpp
    KBDManager.cpp
)

add_executable(kbdexCore ${KBDEX_KEYBOARD_AGENT_SRC})
target_link_libraries(kbdexCore
    Threads::Threads
)
# target_include_directories(kbdexCore PRIVATE ${LUA_INCLUDE_DIRS})

# install(TARGETS kbdexCore)
