include_directories( tables )

set( chardet_SRCS
  impl.cpp
  CharDistribution.cpp
  JpCntx.cpp
  LangBulgarianModel.cpp
  LangCyrillicModel.cpp
  LangGreekModel.cpp
  LangHebrewModel.cpp
  LangHungarianModel.cpp
  LangThaiModel.cpp
  nsBig5Prober.cpp
  nsCharSetProber.cpp
  nsEscCharsetProber.cpp
  nsEscSM.cpp
  nsEUCJPProber.cpp
  nsEUCKRProber.cpp
  nsEUCTWProber.cpp
  nsGB2312Prober.cpp
  nsHebrewProber.cpp
  nsLatin1Prober.cpp
  nsMBCSGroupProber.cpp
  nsMBCSSM.cpp
  nsSBCharSetProber.cpp
  nsSBCSGroupProber.cpp
  nsSJISProber.cpp
  nsUniversalDetector.cpp
  nsUTF8Prober.cpp
)
add_library( chardet SHARED ${chardet_SRCS} )
#install(TARGETS chardet LIBRARY DESTINATION lib)