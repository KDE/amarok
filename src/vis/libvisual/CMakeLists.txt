########### next target ###############

set(amarok_libvisual_SRCS libvisual.cpp )

kde4_automoc(${amarok_libvisual_SRCS})

kde4_add_executable(amarok_libvisual ${amarok_libvisual_SRCS})

target_link_libraries(amarok_libvisual  ${KDE4_KDECORE_LIBS} )

install(TARGETS amarok_libvisual DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############




#original Makefile.am contents follow:

#bin_PROGRAMS = \
#    amarok_libvisual
#
#INCLUDES = \
#    $(LIBVISUAL_CFLAGS) \
#    $(sdl_cflags)
#
#amarok_libvisual_SOURCES = \
#    libvisual.cpp
#
#amarok_libvisual_LDFLAGS = \
#    $(LIBVISUAL_LIBS) \
#    $(sdl_libs)
