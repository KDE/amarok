set(amarokpkg_SRCS
    amarokpkg.cpp
)

add_executable(amarokpkg ${amarokpkg_SRCS})

target_link_libraries(amarokpkg KF5::I18n
                                KF5::Plasma
                                KF5::KConfigWidgets
                                KF5::Service
                                Qt5::DBus
                                Qt5::Core
                                Qt5::Widgets
                     )

install(TARGETS amarokpkg ${INSTALL_TARGETS_DEFAULT_ARGS})

