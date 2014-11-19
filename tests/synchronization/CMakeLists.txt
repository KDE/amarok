
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

set( testmasterslave_SRCS
        TestMasterSlaveSynchronizationJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/SynchronizationBaseJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/MasterSlaveSynchronizationJob.cpp
        ${GOOGLEMOCK_SRCS}
)

kde4_add_unit_test( testmasterslavesynchronizationjob  ${testmasterslave_SRCS} )

add_dependencies( testmasterslavesynchronizationjob amarokcore )
add_dependencies( testmasterslavesynchronizationjob amaroklib )

target_link_libraries(testmasterslavesynchronizationjob
                        amarokcore
                        amaroklib
                        ${KDE4_KDECORE_LIBS}
                        ${KDE4_KIO_LIBS}
                        ${KDE4_SOLID_LIBRARY}
                        ${QT_QTGUI_LIBRARY}
                        ${KDE4_THREADWEAVER_LIBRARIES}
                        ${QT_QTTEST_LIBRARY}
                        ${QT_QTCORE_LIBRARY}
                        ${GOOGLEMOCK_LIBRARIES})

#------------------------ TestUnionJob -----------------------------

set( testunion_SRCS
        TestUnionJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/SynchronizationBaseJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/UnionJob.cpp
        ${GOOGLEMOCK_SRCS}
)

kde4_add_unit_test( testunionjob  ${testunion_SRCS} )
add_dependencies( testunionjob amarokcore )
add_dependencies( testunionjob amaroklib )

target_link_libraries(testunionjob
                        amarokcore
                        amaroklib
                        ${KDE4_KDECORE_LIBS}
                        ${KDE4_KIO_LIBS}
                        ${KDE4_KDEUI_LIBS}
                        ${KDE4_SOLID_LIBRARY}
                        ${QT_QTGUI_LIBRARY}
                        ${KDE4_THREADWEAVER_LIBRARIES}
                        ${QT_QTTEST_LIBRARY}
                        ${QT_QTCORE_LIBRARY}
                        ${GOOGLEMOCK_LIBRARIES})

#------------------------ TestOneWaySynchronizationJob -----------------------------

set( testonewaysynchronization_SRCS
        TestOneWaySynchronizationJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/SynchronizationBaseJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/OneWaySynchronizationJob.cpp
        ${GOOGLEMOCK_SRCS}
)

kde4_add_unit_test( testonewaysynchronizationjob  ${testonewaysynchronization_SRCS} )
add_dependencies( testonewaysynchronizationjob amarokcore )
add_dependencies( testonewaysynchronizationjob amaroklib )

target_link_libraries(testonewaysynchronizationjob
                        amarokcore
                        amaroklib
                        ${KDE4_KDECORE_LIBS}
                        ${KDE4_KIO_LIBS}
                        ${KDE4_KDEUI_LIBS}
                        ${KDE4_SOLID_LIBRARY}
                        ${QT_QTGUI_LIBRARY}
                        ${KDE4_THREADWEAVER_LIBRARIES}
                        ${QT_QTTEST_LIBRARY}
                        ${QT_QTCORE_LIBRARY}
                        ${GOOGLEMOCK_LIBRARIES})
