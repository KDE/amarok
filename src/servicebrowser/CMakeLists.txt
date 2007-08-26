set(CMAKE_VERBOSE_MAKEFILE ON)

include(ConvenienceLibs.cmake)

#add_subdirectory(magnatunestore)

set(libservicebrowser_SRCS
                                servicebrowser.cpp
                                servicebase.cpp

)


KDE4_ADD_LIBRARY( libservicebrowser.la ${libmagnatunebrowser_SRCS} )


########### next target ###############


########### install files ###############



