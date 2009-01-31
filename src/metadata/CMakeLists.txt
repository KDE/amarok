if(MP4V2_FOUND)
include_directories( ${MP4V2_INCLUDE_DIR} )
set(libtagmp4_SRCS
        mp4/mp4file.cpp
        mp4/mp4tag.cpp
        mp4/mp4properties.cpp
        mp4/taglib_mp4filetyperesolver.cpp
)
else(MP4V2_FOUND)
set(libtagmp4_SRCS
	m4a/taglib_mp4filetyperesolver.cpp
	m4a/mp4file.cpp
	m4a/mp4itunestag.cpp
	m4a/mp4isobox.cpp
	m4a/mp4isofullbox.cpp
	m4a/mp4skipbox.cpp
	m4a/mp4moovbox.cpp
	m4a/mp4mvhdbox.cpp
	m4a/mp4ilstbox.cpp
	m4a/boxfactory.cpp
	m4a/mp4fourcc.cpp
	m4a/mp4udtabox.cpp
	m4a/mp4metabox.cpp
	m4a/mp4tagsproxy.cpp
	m4a/mp4mdiabox.cpp
	m4a/mp4minfbox.cpp
	m4a/mp4audioproperties.cpp
	m4a/mp4hdlrbox.cpp
	m4a/mp4stblbox.cpp
	m4a/mp4audiosampleentry.cpp
	m4a/mp4stsdbox.cpp
	m4a/mp4sampleentry.cpp
	m4a/mp4trakbox.cpp
	m4a/mp4propsproxy.cpp
	m4a/itunesnambox.cpp
	m4a/itunesartbox.cpp
	m4a/itunesalbbox.cpp
	m4a/itunescvrbox.cpp
	m4a/itunesgenbox.cpp
	m4a/itunestrknbox.cpp
	m4a/itunesdaybox.cpp
	m4a/itunescmtbox.cpp
	m4a/itunesgrpbox.cpp
	m4a/ituneswrtbox.cpp
	m4a/itunesdiskbox.cpp
	m4a/itunestmpobox.cpp
	m4a/itunesdatabox.cpp
)
endif(MP4V2_FOUND)

set(libtagwav_SRCS
        wav/wavfile.cpp
        wav/wavproperties.cpp
        wav/wavfiletyperesolver.cpp
)

set(libtagspeex_SRCS
        speex/speexfile.cpp
        speex/taglib_speexfiletyperesolver.cpp
        speex/speexproperties.cpp
)

set(libtagasf_SRCS
	asf/asfattribute.cpp
	asf/asfproperties.cpp
	asf/asftag.cpp
	asf/asffile.cpp
	asf/taglib_asffiletyperesolver.cpp
)

set(libtagaac_SRCS
	aac/aacfiletyperesolver.cpp
)

set(libtagrealmedia_SRCS
	rmff/rmff.cpp
	rmff/taglib_realmediafile.cpp
	rmff/taglib_realmediafiletyperesolver.cpp
)


set(libtagaudible_SRCS
	audible/audibleproperties.cpp
	audible/audibletag.cpp
	audible/taglib_audiblefile.cpp
	audible/taglib_audiblefiletyperesolver.cpp
	)

ADD_DEFINITIONS(${TAGLIB_CFLAGS})

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../
    ${CMAKE_CURRENT_BINARY_DIR}
    ${QT_INCLUDE_DIR}
    ${TAGLIB_INCLUDE_DIR}
)

########### next target ###############

SET(amarok_taglib_SRCS
        tplugins.cpp
		tfile_helper.cpp
        ${libtagmp4_SRCS}
        ${libtagaudible_SRCS}
        ${libtagrealmedia_SRCS}
        ${libtagaac_SRCS}
        ${libtagasf_SRCS}
        ${libtagwav_SRCS}
        ${libtagspeex_SRCS}
)

kde4_add_library(amarok_taglib SHARED ${amarok_taglib_SRCS})

set_target_properties(amarok_taglib PROPERTIES VERSION 1.0.0 SOVERSION 1 )
target_link_libraries(amarok_taglib ${TAGLIB_LIBRARIES})
if(WIN32)
target_link_libraries(amarok_taglib ws2_32)
endif(WIN32)
if(MP4V2_FOUND)
target_link_libraries(amarok_taglib ${MP4V2_LIBRARY})
endif(MP4V2_FOUND)

########### install files ###############

install(TARGETS amarok_taglib ${INSTALL_TARGETS_DEFAULT_ARGS} )
