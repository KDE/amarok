SET(
	CLASSES
	ParseException
	JsonToProperties
	JsonToVariant
	VariantToJson
)

SET(VERSION 0.0.2)

##### Probably don't want to edit below this line ##### 

FOREACH(class ${CLASSES})
	SET(SOURCES ${SOURCES} ${class}.cpp)
	SET(HEADERS ${HEADERS} ${class}.h)
ENDFOREACH()

# Find Qt4
FIND_PACKAGE( Qt4 REQUIRED )

SET( QT_DONT_USE_QTGUI TRUE )

# Include the cmake file needed to use qt4
INCLUDE( ${QT_USE_FILE} )

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
