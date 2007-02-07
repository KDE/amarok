if(MP4V2_FOUND)
ADD_SUBDIRECTORY( mp4 ) 
include_directories( ${MP4V2_INCLUDE_DIR} )
set(libtagmp4_SRCS
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/mp4/mp4file.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/mp4/mp4tag.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/mp4/mp4properties.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/mp4/taglib_mp4filetyperesolver.cpp
)
target_link_libraries(amarok_taglib ${MP4V2_LIBRARY})
else(MP4V2_FOUND)
ADD_SUBDIRECTORY( m4a ) 
set(libtagmp4_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/taglib_mp4filetyperesolver.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4file.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4itunestag.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4isobox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4isofullbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4skipbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4moovbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4mvhdbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4ilstbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/boxfactory.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4fourcc.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4udtabox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4metabox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4tagsproxy.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4mdiabox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4minfbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4audioproperties.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4hdlrbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4stblbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4audiosampleentry.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4stsdbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4sampleentry.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4trakbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/mp4propsproxy.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunesnambox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunesartbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunesalbbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunescvrbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunesgenbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunestrknbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunesdaybox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunescmtbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunesgrpbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/ituneswrtbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunesdiskbox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunestmpobox.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/m4a/itunesdatabox.cpp
)
endif(MP4V2_FOUND)

set(libtagwav_SRCS
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/wav/wavfile.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/wav/wavproperties.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/wav/wavfiletyperesolver.cpp
)

set(libtagtrueaudio_SRCS
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/trueaudio/taglib_trueaudiofiletyperesolver.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/trueaudio/ttafile.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/trueaudio/ttaproperties.cpp
)

set(libtagwavpack_SRCS
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/wavpack/taglib_wavpackfiletyperesolver.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/wavpack/wvfile.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/wavpack/wvproperties.cpp
)

set(libtagspeex_SRCS
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/speex/speexfile.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/speex/taglib_speexfiletyperesolver.cpp
        ${CMAKE_SOURCE_DIR}/amarok/src/metadata/speex/speexproperties.cpp
)

set(libtagwma_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/wma/wmaattribute.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/wma/wmaproperties.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/wma/wmatag.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/wma/wmafile.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/wma/taglib_wmafiletyperesolver.cpp
)

set(libtagaac_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/aac/aacfiletyperesolver.cpp
)

set(libtagrealmedia_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/rmff/rmff.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/rmff/taglib_realmediafile.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/rmff/taglib_realmediafiletyperesolver.cpp
)


set(libtagaudible_SRCS
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/audible/audibleproperties.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/audible/audibletag.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/audible/taglib_audiblefile.cpp
	${CMAKE_SOURCE_DIR}/amarok/src/metadata/audible/taglib_audiblefiletyperesolver.cpp
	)


ADD_DEFINITIONS(${TAGLIB_CFLAGS})

INCLUDE_DIRECTORIES(
        ${CMAKE_SOURCE_DIR}/amarok/src ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        ${KDE3_INCLUDE_DIR} ${QT_INCLUDE_DIR} ${TAGLIB_INCLUDE_DIR}
)

########### next target ###############

SET(amarok_taglib_SRCS
        tplugins.cpp
        ${libtagmp4_SRCS}
        ${libtagaudible_SRCS}
        ${libtagrealmedia_SRCS}
        ${libtagaac_SRCS}
        ${libtagwma_SRCS}
        ${libtagwav_SRCS}
        ${libtagwavpack_SRCS}
        ${libtagspeex_SRCS}
        ${libtagtrueaudio_SRCS}
)

kde4_automoc(${amarok_taglib_SRCS})

kde4_add_library(amarok_taglib STATIC ${amarok_taglib_SRCS})

# Laurent : strange a static lib with so name ?
set_target_properties(amarok_taglib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
target_link_libraries(amarok_taglib ${TAGLIB_LIBRARIES})


########### install files ###############

install(TARGETS amarok_taglib DESTINATION ${LIB_INSTALL_DIR} )
