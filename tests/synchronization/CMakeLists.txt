
include_directories(
  ..
  ${AMAROK_SOURCE_TREE}
  ${CMAKE_BINARY_DIR}/src
  ${AMAROK_COLLECTION_SUPPORT_DIR}
  
  
  ${GOOGLEMOCK_INCLUDE_DIR}
  )

#------------------------ TestMasterSlaveSynchronizationJob -----------------------------

set( testmasterslave_SRCS
        TestMasterSlaveSynchronizationJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/SynchronizationBaseJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/MasterSlaveSynchronizationJob.cpp
        ${GOOGLEMOCK_SRCS}
)

add_executable( testmasterslavesynchronizationjob  ${testmasterslave_SRCS} )
add_test(testmasterslavesynchronizationjob testmasterslavesynchronizationjob)
ecm_mark_as_test(testmasterslavesynchronizationjob)

add_dependencies( testmasterslavesynchronizationjob amarokcore )
add_dependencies( testmasterslavesynchronizationjob amaroklib )

target_link_libraries(testmasterslavesynchronizationjob
                        amarokcore
                        amaroklib
                        KF5::KDELibs4Support
                        KF5::KIOCore
                        ${KDE4_SOLID_LIBRARY}
                        Qt5::Gui
                        KF5::ThreadWeaver
                        Qt5::Test
                        Qt5::Core
                        ${GOOGLEMOCK_LIBRARIES})

#------------------------ TestUnionJob -----------------------------

set( testunion_SRCS
        TestUnionJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/SynchronizationBaseJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/UnionJob.cpp
        ${GOOGLEMOCK_SRCS}
)

add_executable( testunionjob  ${testunion_SRCS} )
add_test(testunionjob testunionjob)
ecm_mark_as_test(testunionjob)
add_dependencies( testunionjob amarokcore )
add_dependencies( testunionjob amaroklib )

target_link_libraries(testunionjob
                        amarokcore
                        amaroklib
                        KF5::KDELibs4Support
                        KF5::KIOCore
                        
                        ${KDE4_SOLID_LIBRARY}
                        Qt5::Gui
                        KF5::ThreadWeaver
                        Qt5::Test
                        Qt5::Core
                        ${GOOGLEMOCK_LIBRARIES})

#------------------------ TestOneWaySynchronizationJob -----------------------------

set( testonewaysynchronization_SRCS
        TestOneWaySynchronizationJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/SynchronizationBaseJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/OneWaySynchronizationJob.cpp
        ${GOOGLEMOCK_SRCS}
)

add_executable( testonewaysynchronizationjob  ${testonewaysynchronization_SRCS} )
add_test(testonewaysynchronizationjob testonewaysynchronizationjob)
ecm_mark_as_test(testonewaysynchronizationjob)
add_dependencies( testonewaysynchronizationjob amarokcore )
add_dependencies( testonewaysynchronizationjob amaroklib )

target_link_libraries(testonewaysynchronizationjob
                        amarokcore
                        amaroklib
                        KF5::KDELibs4Support
                        KF5::KIOCore
                        
                        ${KDE4_SOLID_LIBRARY}
                        Qt5::Gui
                        KF5::ThreadWeaver
                        Qt5::Test
                        Qt5::Core
                        ${GOOGLEMOCK_LIBRARIES})
