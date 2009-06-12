set(amarokpkg_SRCS
    amarokpkg.cpp
)

kde4_add_executable(amarokpkg ${amarokpkg_SRCS})

target_link_libraries(amarokpkg ${KDE4_KDEUI_LIBS} ${KDE4_PLASMA_LIBS})

install(TARGETS amarokpkg ${INSTALL_TARGETS_DEFAULT_ARGS})

