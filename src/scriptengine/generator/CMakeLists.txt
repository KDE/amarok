find_package(Qt4)
if(${QT_VERSION_PATCH} EQUAL "1")
    add_subdirectory( generator )
endif(${QT_VERSION_PATCH} EQUAL "1")

