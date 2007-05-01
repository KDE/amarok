
add_subdirectory( helix-sp )

include_directories(
${CMAKE_CURRENT_SOURCE_DIR}/../..
${CMAKE_CURRENT_SOURCE_DIR}/../../plugin
${CMAKE_CURRENT_SOURCE_DIR}/../../amarokcore
${CMAKE_CURRENT_BINARY_DIR}/../.. #for amarokconfig.h
${KDE4_INCLUDE_DIR} ${QT_INCLUDES}
${CMAKE_CURRENT_SOURCE_DIR}/../../engine/helix/helix-sp
${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helix-include/runtime
${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helix-include/audio/fixptutil
${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helix-include/common/include
${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helix-include/client/include
${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helix-include/common/container
${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helix-include/common/system
${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helix-include/common/dbgtool
${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helix-include/common/util
${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helix-include/common/log
)

FIND_PACKAGE(Alsa)
if (ALSA_FOUND)

add_definitions( -DUSE_HELIX_ALSA )

if (HAVE_SND_PCM_RESUME)
add_definitions( -DHAVE_SND_PCM_RESUME )
endif(HAVE_SND_PCM_RESUME)

set ( ALSA_LIBS ${ASOUND_LIBRARY} )

endif(ALSA_FOUND)

add_definitions( -DHELIX_LIBS='"'${HELIX_LIBRARY_PATH}'"' -DHELIX_CONFIG_DISABLE_ATOMIC_OPERATORS -include ${CMAKE_CURRENT_SOURCE_DIR}/helix-sp/helixdefines.h )

########### next target ###############

set(amarok_helixengine_plugin_PART_SRCS
   helix-sp/hspcontext.cpp
   helix-sp/hspadvisesink.cpp
   helix-sp/hsperror.cpp
   helix-sp/hspauthmgr.cpp
   helix-sp/hsphook.cpp
   helix-sp/iids.cpp
   helix-sp/utils.cpp
   helix-sp/gain.cpp
   helix-sp/hspalsadevice.cpp
   helix-sp/helix-sp.cpp
   helix-engine.cpp
   helix-errors.cpp
   helix-configdialog.cpp
   hxplayercontrol.cpp
   helixconfig.cpp )

kde4_add_kcfg_files( amarok_helixengine_PART_SRCS helixconfig.kcfgc )

kde4_automoc(${amarok_helixengine_plugin_PART_SRCS})

kde4_add_plugin(amarok_helixengine_plugin WITH_PREFIX ${amarok_helixengine_plugin_PART_SRCS})


target_link_libraries(amarok_helixengine_plugin ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KDEUI_LIBS} amarok ${ALSA_LIBS} dl )

install(TARGETS amarok_helixengine_plugin DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES helixconfig.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(FILES  amarok_helixengine_plugin.desktop DESTINATION
${SERVICES_INSTALL_DIR} )
