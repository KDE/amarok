find_package(Qt4)

set ( QT_USE_QTXML TRUE )
set ( QT_USE_QTSCRIPT TRUE ) #generator doesn't need this, just the qtscript libs
set ( QT_DONT_USE_QTGUI TRUE )
include(${QT_USE_FILE})

add_definitions( -DRXX_ALLOCATOR_INIT_0 )

include_directories(
    ${QT_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/parser/rpp
)

set(Rpp_SRCS
    parser/rpp/preprocessor.cpp
)

set(Parser_SRCS
    parser/ast.cpp 
    parser/lexer.cpp
    parser/list.cpp
    parser/parser.cpp
    parser/smallobject.cpp
    parser/control.cpp
    parser/visitor.cpp
    parser/default_visitor.cpp
    parser/dumptree.cpp
    parser/tokens.cpp
    parser/binder.cpp
    parser/codemodel.cpp
    parser/type_compiler.cpp
    parser/name_compiler.cpp
    parser/declarator_compiler.cpp
    parser/class_compiler.cpp
    parser/codemodel_finder.cpp
    parser/compiler_utils.cpp
)

set(Generator_SRCS
    ${Parser_SRCS}
    ${Rpp_SRCS}
    generator.cpp
    main.cpp
    reporthandler.cpp
    typeparser.cpp
    typesystem.cpp
    asttoxml.cpp
    fileout.cpp
    generatorset.cpp
    metajava.cpp
    customtypes.cpp
    abstractmetabuilder.cpp
    abstractmetalang.cpp
    prigenerator.cpp

    generatorsetqtscript.cpp
    metaqtscriptbuilder.cpp
    metaqtscript.cpp
    classgenerator.cpp
    shellgenerator.cpp
    shellimplgenerator.cpp
    shellheadergenerator.cpp
    setupgenerator.cpp
    docgenerator.cpp
)

set(Generator_MOCS
    generator.h
    fileout.h
    generatorset.h
    prigenerator.h
    generatorsetqtscript.h
    classgenerator.h
    shellgenerator.h
    shellimplgenerator.h
    shellheadergenerator.h
    setupgenerator.h
)

QT4_WRAP_CPP(Generator_MOC_SRCS ${Generator_MOCS})

set(Generator_RCCS generator.qrc)

qt4_add_resources(Generator_RCC_SRCS ${Generator_RCCS})

include_directories( ${CMAKE_BINARY_DIR} )

add_executable(generator ${Generator_SRCS} ${Generator_MOC_SRCS} ${Generator_RCC_SRCS})

target_link_libraries(generator ${QT_LIBRARIES})

get_target_property( GENERATOR_EXE generator LOCATION )
add_custom_command( TARGET generator
                    POST_BUILD
                    COMMAND ${GENERATOR_EXE} --output-directory=${CMAKE_CURRENT_BINARY_DIR}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../qtbindings/qtscript_core/ )

include( "qtscript_core.cmake" )
include( "qtscript_gui.cmake" )
include( "qtscript_network.cmake" )
include( "qtscript_sql.cmake" )
include( "qtscript_webkit.cmake" )
include( "qtscript_xml.cmake" )
#XML Patterns requires Qt to be compiled with exception support
#so Qt is often missing this feature
#include( "qtscript_xmlpatterns.cmake" )