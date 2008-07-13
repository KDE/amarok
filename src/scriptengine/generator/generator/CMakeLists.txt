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

set( Generated_QtCore_SRCS
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QBasicTimer.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QBitArray.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QBuffer.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QByteArray.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QByteArrayMatcher.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QChildEvent.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QCoreApplication.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QCryptographicHash.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QDataStream.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QDir.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QDirIterator.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QDynamicPropertyChangeEvent.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QEvent.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QEventLoop.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFile.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFileInfo.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFileSystemWatcher.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFuture.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFutureIterator.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFutureSynchronizer.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFutureSynchronizerVoid.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFutureVoid.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFutureWatcher.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QFutureWatcherVoid.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QIODevice.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QLibraryInfo.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QLocale.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QMutex.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QObject.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QPoint.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QPointF.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QProcess.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QRect.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QRectF.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QRunnable.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QSemaphore.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QSettings.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QSignalMapper.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QSize.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QSizeF.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QSocketNotifier.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QStringMatcher.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QSystemSemaphore.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTemporaryFile.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTextBoundaryFinder.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTextCodec.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTextDecoder.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTextEncoder.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTextStream.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QThreadPool.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTime.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTimeLine.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTimer.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTimerEvent.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QTranslator.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QUrl.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QUuid.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QWaitCondition.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QXmlStreamAttribute.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QXmlStreamAttributes.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QXmlStreamEntityDeclaration.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QXmlStreamEntityResolver.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QXmlStreamNamespaceDeclaration.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QXmlStreamNotationDeclaration.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QXmlStreamReader.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QXmlStreamWriter.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_Qt.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscript_QtConcurrent.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QBuffer.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QChildEvent.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QCoreApplication.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QDataStream.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QDirIterator.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QEvent.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QEventLoop.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QFile.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QFileSystemWatcher.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QFutureSynchronizer.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QFutureSynchronizerVoid.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QIODevice.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QObject.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QProcess.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QRunnable.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QSettings.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QSignalMapper.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QSocketNotifier.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QTemporaryFile.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QTextCodec.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QTextStream.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QThreadPool.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QTimeLine.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QTimer.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QTimerEvent.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QTranslator.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QUuid.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QXmlStreamAttributes.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/qtscriptshell_QXmlStreamEntityResolver.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/generated_cpp/com_trolltech_qt_core/main.cpp
)
get_target_property( GENERATOR_EXE generator LOCATION )
add_custom_command( OUTPUT ${Generated_QtCore_SRCS}
                    COMMAND ${GENERATOR_EXE} --output-directory=${CMAKE_CURRENT_BINARY_DIR}
                    DEPENDS generator
#                    MAIN_DEPENDENCY build_all.txt
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../qtbindings/qtscript_core/ )
add_library( qtscript_core MODULE ${Generated_QtCore_SRCS} )
#qtscript bindings don't use moc
target_link_libraries(qtscript_core ${QT_LIBRARIES}) 
