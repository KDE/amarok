if(MP4V2_FOUND)
ADD_SUBDIRECTORY( mp4 ) 
include_directories( ${MP4V2_INCLUDE_DIR} )
set(libtagmp4_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/mp4/mp4file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mp4/mp4tag.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mp4/mp4properties.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mp4/taglib_mp4filetyperesolver.cpp
)
else(MP4V2_FOUND)
ADD_SUBDIRECTORY( m4a ) 
set(libtagmp4_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/taglib_mp4filetyperesolver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4file.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4itunestag.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4isobox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4isofullbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4skipbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4moovbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4mvhdbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4ilstbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/boxfactory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4fourcc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4udtabox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4metabox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4tagsproxy.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4mdiabox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4minfbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4audioproperties.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4hdlrbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4stblbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4audiosampleentry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4stsdbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4sampleentry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4trakbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/mp4propsproxy.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunesnambox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunesartbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunesalbbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunescvrbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunesgenbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunestrknbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunesdaybox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunescmtbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunesgrpbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/ituneswrtbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunesdiskbox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunestmpobox.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/m4a/itunesdatabox.cpp
)
endif(MP4V2_FOUND)

set(libtagwav_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/wav/wavfile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wav/wavproperties.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wav/wavfiletyperesolver.cpp
)

set(libtagtrueaudio_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/trueaudio/taglib_trueaudiofiletyperesolver.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/trueaudio/ttafile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/trueaudio/ttaproperties.cpp
)

set(libtagwavpack_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/wavpack/taglib_wavpackfiletyperesolver.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wavpack/wvfile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wavpack/wvproperties.cpp
)

set(libtagspeex_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/speex/speexfile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/speex/taglib_speexfiletyperesolver.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/speex/speexproperties.cpp
)

set(libtagwma_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/wma/wmaattribute.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/wma/wmaproperties.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/wma/wmatag.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/wma/wmafile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/wma/taglib_wmafiletyperesolver.cpp
)

set(libtagaac_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/aac/aacfiletyperesolver.cpp
)

set(libtagrealmedia_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/rmff/rmff.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/rmff/taglib_realmediafile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/rmff/taglib_realmediafiletyperesolver.cpp
)


set(libtagaudible_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/audible/audibleproperties.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/audible/audibletag.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/audible/taglib_audiblefile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/audible/taglib_audiblefiletyperesolver.cpp
	)


ADD_DEFINITIONS(${TAGLIB_CFLAGS})

INCLUDE_DIRECTORIES(
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}
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

kde4_add_library(amarok_taglib SHARED ${amarok_taglib_SRCS})

set_target_properties(amarok_taglib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
target_link_libraries(amarok_taglib ${TAGLIB_LIBRARIES})
if(MP4V2_FOUND)
target_link_libraries(amarok_taglib ${MP4V2_LIBRARY})
endif(MP4V2_FOUND)

########### install files ###############

install(TARGETS amarok_taglib DESTINATION ${LIB_INSTALL_DIR} )
