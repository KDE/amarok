
include_directories( ../
                     ../../src/
                     charset-detector/include
                     charset-detector/src
                     charset-detector/src/tables
                     ${QT_INCLUDES}
                     ${TAGLIB_INCLUDES}
                     ${TAGLIB-EXTRAS_INCLUDES}
                   )

add_definitions(${TAGLIB_CFLAGS} ${TAGLIB-EXTRAS_CFLAGS})

#####################################################################
# Character Encoding Detector
#####################################################################
set( libchardet_SRCS
    charset-detector/src/impl.cpp
    charset-detector/src/CharDistribution.cpp
    charset-detector/src/JpCntx.cpp
    charset-detector/src/LangBulgarianModel.cpp
    charset-detector/src/LangCyrillicModel.cpp
    charset-detector/src/LangGreekModel.cpp
    charset-detector/src/LangHebrewModel.cpp
    charset-detector/src/LangHungarianModel.cpp
    charset-detector/src/LangThaiModel.cpp
    charset-detector/src/nsBig5Prober.cpp
    charset-detector/src/nsCharSetProber.cpp
    charset-detector/src/nsEscCharsetProber.cpp
    charset-detector/src/nsEscSM.cpp
    charset-detector/src/nsEUCJPProber.cpp
    charset-detector/src/nsEUCKRProber.cpp
    charset-detector/src/nsEUCTWProber.cpp
    charset-detector/src/nsGB2312Prober.cpp
    charset-detector/src/nsHebrewProber.cpp
    charset-detector/src/nsLatin1Prober.cpp
    charset-detector/src/nsMBCSGroupProber.cpp
    charset-detector/src/nsMBCSSM.cpp
    charset-detector/src/nsSBCharSetProber.cpp
    charset-detector/src/nsSBCSGroupProber.cpp
    charset-detector/src/nsSJISProber.cpp
    charset-detector/src/nsUniversalDetector.cpp
    charset-detector/src/nsUTF8Prober.cpp
)

#####################################################################
# TagLib File Type Resolvers
#####################################################################
set( taglib_filetype_resolver_SRCS
    ../../shared/taglib_filetype_resolvers/wavfiletyperesolver.cpp
    ../../shared/taglib_filetype_resolvers/mp4filetyperesolver.cpp
    ../../shared/taglib_filetype_resolvers/asffiletyperesolver.cpp
)

########### next target ###############

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

qt4_automoc( CollectionScanner.cpp )

add_definitions( "-DUTILITIES_BUILD" )

set(amarokcollectionscanner_SRCS
    CollectionScanner.cpp
    AFTUtility.cpp
    ../../shared/MetaReplayGain.cpp
)

add_executable(amarokcollectionscanner ${amarokcollectionscanner_SRCS} ${libchardet_SRCS} ${taglib_filetype_resolver_SRCS})

target_link_libraries(amarokcollectionscanner 
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTXML_LIBRARY}
                      ${QT_QTDBUS_LIBRARY}
                      ${TAGLIB_LIBRARIES}
                      ${TAGLIB-EXTRAS_LIBRARIES}
                      )

if(APPLE)
	SET_TARGET_PROPERTIES(amarokcollectionscanner PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
	# install to app bundle on os x, otherwise amarok fails to load it
	install(TARGETS amarokcollectionscanner DESTINATION ${BUNDLE_INSTALL_DIR}/Amarok.app/Contents/MacOS )
else(APPLE)
	install(TARGETS amarokcollectionscanner RUNTIME DESTINATION ${BIN_INSTALL_DIR} )
endif(APPLE)
