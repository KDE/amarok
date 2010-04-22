
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
        ${AMAROK_SOURCE_TREE}/core/support/Components.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/SynchronizationBaseJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/MasterSlaveSynchronizationJob.cpp
        ${AMAROK_SOURCE_TREE}/core/collections/Collection.cpp
        ${AMAROK_SOURCE_TREE}/core/collections/CollectionLocation.cpp
        ${AMAROK_SOURCE_TREE}/core/collections/QueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryQueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryQueryMakerInternal.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryFilter.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryMatcher.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryCustomValue.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryQueryMakerHelper.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/Meta.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/support/MetaUtility.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/EditCapability.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/Capability.cpp
    )

kde4_add_unit_test( testmasterslavesynchronizationjob  ${testmasterslave_SRCS} )

add_dependencies( testmasterslavesynchronizationjob amarokcore )
add_dependencies( testmasterslavesynchronizationjob amaroklib )

target_link_libraries(testmasterslavesynchronizationjob
                        amarokcore
                        amaroklib
                        ${KDE4_KDECORE_LIBS}
                        ${KDE4_KDEUI_LIBS}
                        ${QT_QTGUI_LIBRARY}
                        ${KDE4_THREADWEAVER_LIBRARIES}
                        ${QT_QTTEST_LIBRARY}
                        ${QT_QTCORE_LIBRARY}
                        ${GOOGLEMOCK_LIBRARIES})

#------------------------ TestUnionJob -----------------------------

set( testunion_SRCS
        TestUnionJob.cpp
        ${AMAROK_SOURCE_TREE}/core/support/Components.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/SynchronizationBaseJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/UnionJob.cpp
        ${AMAROK_SOURCE_TREE}/core/collections/Collection.cpp
        ${AMAROK_SOURCE_TREE}/core/collections/CollectionLocation.cpp
        ${AMAROK_SOURCE_TREE}/core/collections/QueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryQueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryQueryMakerInternal.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryFilter.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryMatcher.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryCustomValue.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryQueryMakerHelper.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/Meta.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/support/MetaUtility.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/EditCapability.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/Capability.cpp
    )

kde4_add_unit_test( testunionjob  ${testunion_SRCS} )
add_dependencies( testunionjob amarokcore )
add_dependencies( testunionjob amaroklib )

target_link_libraries(testunionjob
                        amarokcore
                        amaroklib
                        ${KDE4_KDECORE_LIBS}
                        ${KDE4_KDEUI_LIBS}
                        ${QT_QTGUI_LIBRARY}
                        ${KDE4_THREADWEAVER_LIBRARIES}
                        ${QT_QTTEST_LIBRARY}
                        ${QT_QTCORE_LIBRARY}
                        ${GOOGLEMOCK_LIBRARIES})

#------------------------ TestOneWaySynchronizationJob -----------------------------

set( testonewaysynchronization_SRCS
        TestOneWaySynchronizationJob.cpp
        ${AMAROK_SOURCE_TREE}/core/support/Components.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/SynchronizationBaseJob.cpp
        ${AMAROK_SOURCE_TREE}/synchronization/OneWaySynchronizationJob.cpp
        ${AMAROK_SOURCE_TREE}/core/collections/Collection.cpp
        ${AMAROK_SOURCE_TREE}/core/collections/CollectionLocation.cpp
        ${AMAROK_SOURCE_TREE}/core/collections/QueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryQueryMaker.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryQueryMakerInternal.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryFilter.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryMatcher.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryCustomValue.cpp
        ${AMAROK_SOURCE_TREE}/core-impl/collections/support/MemoryQueryMakerHelper.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/Meta.cpp
        ${AMAROK_SOURCE_TREE}/core/meta/support/MetaUtility.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/EditCapability.cpp
        ${AMAROK_SOURCE_TREE}/core/capabilities/Capability.cpp
    )

kde4_add_unit_test( testonewaysynchronizationjob  ${testonewaysynchronization_SRCS} )
add_dependencies( testonewaysynchronizationjob amarokcore )
add_dependencies( testonewaysynchronizationjob amaroklib )

target_link_libraries(testonewaysynchronizationjob
                        amarokcore
                        amaroklib
                        ${KDE4_KDECORE_LIBS}
                        ${KDE4_KDEUI_LIBS}
                        ${QT_QTGUI_LIBRARY}
                        ${KDE4_THREADWEAVER_LIBRARIES}
                        ${QT_QTTEST_LIBRARY}
                        ${QT_QTCORE_LIBRARY}
                        ${GOOGLEMOCK_LIBRARIES})
