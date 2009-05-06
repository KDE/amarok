SET(
	CLASSES
	ParseException
	JsonRpc
	JsonRpcAdaptor
	JsonRpcAdaptorPrivate
	JsonToProperties
	JsonToVariant
	VariantToJson
)
SET(HEADERS JsonQtExport.h)

SET(VERSION 0.1.0)

##### Probably don't want to edit below this line ##### 

# Find Qt4
FIND_PACKAGE( Qt4 REQUIRED )

SET( QT_DONT_USE_QTGUI TRUE )

# Include the cmake file needed to use qt4
INCLUDE( ${QT_USE_FILE} )

# Include the binary directory for moc files.
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

FOREACH(class ${CLASSES})
	QT4_WRAP_CPP(MOC_SOURCE ${class}.h)
	SET(SOURCES ${SOURCES} ${class}.cpp ${MOC_SOURCE})
	SET(HEADERS ${HEADERS} ${class}.h)
ENDFOREACH()
SET(SOURCES ${SOURCES} ${MOC_SOURCE})

IF("${STATIC_JSONQT}" STREQUAL "")
	OPTION(STATIC_JSONQT "Build static libraries of JsonQt instead of shared" OFF)
ENDIF()
IF(STATIC_JSONQT)
	SET(BUILD STATIC)
ELSE()
	SET(BUILD SHARED)
ENDIF()

MESSAGE(STATUS "Building ${BUILD} JsonQt")

ADD_LIBRARY(
	JsonQt
	${BUILD}
	${SOURCES}
)

SET_TARGET_PROPERTIES(
	JsonQt
	PROPERTIES
	VERSION "${VERSION}"
	SOVERSION "0"
)

TARGET_LINK_LIBRARIES(
	JsonQt
	${QT_LIBRARIES}
)

INSTALL(
	TARGETS
	JsonQt
	DESTINATION
	lib${LIB_SUFFIX}
)

INSTALL(
	FILES
	${HEADERS}
	DESTINATION
	include/JsonQt
)
