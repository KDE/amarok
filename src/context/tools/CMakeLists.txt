set(amarokpkg_SRCS
    amarokpkg.cpp
)

add_executable(amarokpkg ${amarokpkg_SRCS})

target_link_libraries(amarokpkg  KF5::Plasma)

install(TARGETS amarokpkg ${INSTALL_TARGETS_DEFAULT_ARGS})

