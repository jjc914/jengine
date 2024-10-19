file(GLOB DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)

foreach(DIR ${DIRS})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/CMakeLists.txt)
        add_subdirectory(${DIR})
    endif()
endforeach()
