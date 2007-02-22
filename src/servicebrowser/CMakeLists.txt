
find_package(KDE4 REQUIRED)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

link_directories(${KDE4_LIB_DIR})

include(ConvenienceLibs.cmake)

include_directories( ../../src/ ../../src/amarokcore ../../src/analyzers ../../src/plugin ../../src/statusbar ../ ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )

set(libservicebrowser_SRCS
                                servicebrowser.cpp
                                servicebase.cpp

)

kde4_automoc( ${libmagnatunebrowser_SRCS} )

ADD_LIBRARY( libservicebrowser.la ${libmagnatunebrowser_SRCS} )


########### next target ###############


########### install files ###############



