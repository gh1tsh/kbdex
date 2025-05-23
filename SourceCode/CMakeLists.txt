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
pkg_check_modules(GLIBMM REQUIRED glibmm-2.68)
pkg_check_modules(GIOMM REQUIRED giomm-2.68)

# TODO: добавить библиотеки, необходимые моему проекту: libinput, libevdev, libudev


set(KBDEX_CORE_SRC
    hawck-macrod.cpp
    RemoteUDevice.cpp
    Daemon.cpp
    MacroDaemon.cpp
    Keyboard.cpp
    FSWatcher.cpp
    Permissions.cpp
    FIFOWatcher.cpp
    XDG.cpp
    KBDB.cpp
    Popen.cpp
    Decoder.cpp
)

add_executable(kbdexCore ${KBDEX_CORE_SRC})

target_link_libraries(kbdexCore
    PkgConfig::LIBNOTIFY
    ${GLIBMM_LIBRARIES}
    ${GIOMM_LIBRARIES}
    Threads::Threads
)

target_include_directories(kbdexCore
    PRIVATE ${CMAKE_BINARY_DIR}
    ${GIOMM_INCLUDE_DIRS}
)

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
    KBDManager.cpp
)

add_executable(kbdexKeyboardAgent ${KBDEX_KEYBOARD_AGENT_SRC})

target_link_libraries(kbdexKeyboardAgent
    Threads::Threads
)

target_include_directories(kbdexKeyboardAgent
    PRIVATE ${CMAKE_BINARY_DIR}
)

# install(TARGETS kbdexKeyboardAgent)
