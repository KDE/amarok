set(amarokpkg_SRCS
    amarokpkg.cpp
)

add_executable(amarokpkg ${amarokpkg_SRCS})

target_link_libraries(amarokpkg  ${KDE4_PLASMA_LIBS})

install(TARGETS amarokpkg ${INSTALL_TARGETS_DEFAULT_ARGS})

