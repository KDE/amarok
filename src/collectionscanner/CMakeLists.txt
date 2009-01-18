
include_directories( ../
                     ../meta
                     ../charset-detector/include
                     ../charset-detector/src
                     ../charset-detector/src/tables
                    ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

add_definitions(${TAGLIB_CFLAGS})

#####################################################################
# Character Encoding Detector
#####################################################################
set( libchardet_SRCS
    ../charset-detector/src/impl.cpp
    ../charset-detector/src/CharDistribution.cpp
    ../charset-detector/src/JpCntx.cpp
    ../charset-detector/src/LangBulgarianModel.cpp
    ../charset-detector/src/LangCyrillicModel.cpp
    ../charset-detector/src/LangGreekModel.cpp
    ../charset-detector/src/LangHebrewModel.cpp
    ../charset-detector/src/LangHungarianModel.cpp
    ../charset-detector/src/LangThaiModel.cpp
    ../charset-detector/src/nsBig5Prober.cpp
    ../charset-detector/src/nsCharSetProber.cpp
    ../charset-detector/src/nsEscCharsetProber.cpp
    ../charset-detector/src/nsEscSM.cpp
    ../charset-detector/src/nsEUCJPProber.cpp
    ../charset-detector/src/nsEUCKRProber.cpp
    ../charset-detector/src/nsEUCTWProber.cpp
    ../charset-detector/src/nsGB2312Prober.cpp
    ../charset-detector/src/nsHebrewProber.cpp
    ../charset-detector/src/nsLatin1Prober.cpp
    ../charset-detector/src/nsMBCSGroupProber.cpp
    ../charset-detector/src/nsMBCSSM.cpp
    ../charset-detector/src/nsSBCharSetProber.cpp
    ../charset-detector/src/nsSBCSGroupProber.cpp
    ../charset-detector/src/nsSJISProber.cpp
    ../charset-detector/src/nsUniversalDetector.cpp
    ../charset-detector/src/nsUTF8Prober.cpp
)

########### next target ###############

set(amarokcollectionscanner_SRCS main.cpp CollectionScanner.cpp ../meta/MetaReplayGain.cpp)

kde4_add_executable(amarokcollectionscanner NOGUI ${amarokcollectionscanner_SRCS} ${libchardet_SRCS})

target_link_libraries(amarokcollectionscanner 
                      amarok_taglib
                      ${TAGLIB_LIBRARIES}
                      ${KDE4_KDEUI_LIBS}
                      ${QT_QTXML_LIBRARY})

if(APPLE)
	SET_TARGET_PROPERTIES(amarokcollectionscanner PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
	# install to app bundle on os x, otherwise amarok fails to load it
	install(TARGETS amarokcollectionscanner DESTINATION ${BUNDLE_INSTALL_DIR}/Amarok.app/Contents/MacOS )
else(APPLE)
	install(TARGETS amarokcollectionscanner ${INSTALL_TARGETS_DEFAULT_ARGS} )
endif(APPLE)
