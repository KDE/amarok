########### next target ###############

set(amarok_bin_SRCS loader.cpp splash.cpp )

kde4_automoc(${amarok_bin_SRCS})

kde4_add_executable(amarok_bin ${amarok_bin_SRCS})

set_target_properties(amarok_bin PROPERTIES OUTPUT_NAME amarok)

target_link_libraries(amarok_bin  ${KDE4_KDECORE_LIBS} )

install(TARGETS amarok_bin DESTINATION ${BIN_INSTALL_DIR} )
