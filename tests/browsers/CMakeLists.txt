include_directories(
  ..
  ${AMAROK_SOURCE_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ TestMasterSlaveSynchronizationJob -----------------------------

set( testsinglecollectiontreeitemmodel_SRCS
        TestSingleCollectionTreeItemModel.cpp
        ${GOOGLEMOCK_SRCS}
    )

kde4_add_unit_test( testsinglecollectiontreeitemmodel  ${testsinglecollectiontreeitemmodel_SRCS} )

add_dependencies( testsinglecollectiontreeitemmodel amarokcore )
add_dependencies( testsinglecollectiontreeitemmodel amaroklib )

target_link_libraries(testsinglecollectiontreeitemmodel
                        amarokcore
                        amaroklib
                        ${KDE4_KDECORE_LIBS}
                        ${KDE4_KDEUI_LIBS}
                        ${QT_QTGUI_LIBRARY}
                        ${KDE4_THREADWEAVER_LIBRARIES}
                        ${QT_QTTEST_LIBRARY}
                        ${QT_QTCORE_LIBRARY}
                        ${GOOGLEMOCK_LIBRARIES})
