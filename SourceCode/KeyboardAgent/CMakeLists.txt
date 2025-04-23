project(
	kbdexKeyboardAgent
	DESCRIPTION "kbdex keyboard agent"
	LANGUAGES C
)

pkg_check_modules(LIBEVDEV REQUIRED libevdev)
pkg_check_modules(LIBINPUT REQUIRED libinput)
pkg_check_modules(LIBUDEV REQUIRED libudev)

add_executable(KeyboardAgent
	main.c
)

# Флаги компиляции взяты из вот этой рекомендации:
# https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html

# Для компиляции в релизы также стоит использовать:
#		-fno-delete-null-pointer-checks -fno-strict-overflow -fno-strict-aliasing -ftrivial-auto-var-init=zero

target_compile_options(susuwatarid PRIVATE -O2 -Wall -Wformat -Wformat=2
										   -Wconversion -Wimplicit-fallthrough
										   -Werror=format-security
										   -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3
										   -D_GLIBCXX_ASSERTIONS
										   -fPIE -fcf-protection=full
										   -fstrict-flex-arrays=3
										   -fstack-clash-protection
										   -fstack-protector-strong
)

target_link_options(susuwatarid PRIVATE -Wl,-z,nodlopen -Wl,-z,noexecstack
										-Wl,-z,relro -Wl,-z,now -Wl,--as-needed 
										-Wl,--no-copy-dt-needed-entries)

target_link_libraries(susuwatarid PRIVATE
	${LIBEVDEV_LIBRARIES}
	${LIBINPUT_LIBRARIES}
	${LIBUDEV_LIBRARIES}
)

target_include_directories(susuwatarid PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${LIBEVDEV_INCLUDE_DIRS}
	${LIBINPUT_INCLUDE_DIRS}
	${LIBUDEV_INCLUDE_DIRS}
)