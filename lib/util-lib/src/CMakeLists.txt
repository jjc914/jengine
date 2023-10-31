set(LIB_NAME utilib)

add_library(
    ${LIB_NAME}
    SHARED
    "${DSLIB_SOURCE_DIR}/include/utilib.hpp"
)

target_include_directories(${LIB_NAME} PUBLIC ${DSLIB_SOURCE_DIR}/include)
set_target_properties(${LIB_NAME} PROPERTIES LINKER_LANGUAGE CXX)
