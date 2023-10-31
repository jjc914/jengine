add_executable(${TEST_NAME} test.cpp)

target_link_libraries(${TEST_NAME} ${LIB_DEPENDENCIES})
target_link_libraries(${TEST_NAME} ${SRC_DEPENDENCIES})
