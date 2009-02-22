ENABLE_TESTING()
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set( SmartPointerListTest_SRCS TestSmartPointerList.cpp )

kde4_add_unit_test( SmartPointerListTest
  TESTNAME General-SmartPointerListTest
  ${SmartPointerListTest_SRCS}
)
 
target_link_libraries( SmartPointerListTest
  ${QT_QTCORE_LIBRARY}
  ${QT_QTTEST_LIBRARY}
)
