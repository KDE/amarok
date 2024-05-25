set(amarokpkg_SRCS
    amarokpkg.cpp
    ../AmarokContextPackageStructure.cpp
)

add_executable(amarokpkg ${amarokpkg_SRCS})

target_link_libraries(amarokpkg KF5::I18n
                                KF5::ConfigWidgets
                                KF5::Package
                                Qt::DBus
                                Qt::Core
                                Qt::Widgets
                     )

install(TARGETS amarokpkg ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

