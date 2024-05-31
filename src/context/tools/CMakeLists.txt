set(amarokpkg_SRCS
    amarokpkg.cpp
    ../AmarokContextPackageStructure.cpp
)

add_executable(amarokpkg ${amarokpkg_SRCS})

target_link_libraries(amarokpkg KF${KF_MAJOR_VERSION}::I18n
                                KF${KF_MAJOR_VERSION}::ConfigWidgets
                                KF${KF_MAJOR_VERSION}::Package
                                Qt::DBus
                                Qt::Core
                                Qt::Widgets
                     )

install(TARGETS amarokpkg ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

