#ifndef engine_core_debug_ASSERT_HPP
#define engine_core_debug_ASSERT_HPP

#include "logger.hpp"

#include <cstdlib>
#include <iostream>

#define _ENGINE_HAS_ARGS(...) _ENGINE_HAS_ARGS_IMPL(__VA_ARGS__, 1, 0)
#define _ENGINE_HAS_ARGS_IMPL(_1, _2, N, ...) N

#ifndef NDEBUG
    #define ENGINE_ASSERT(expr, ...) \
        do { \
            if (!(expr)) { \
                auto& logger = engine::core::debug::Logger::get_singleton(); \
                logger.error("[ENGINE ASSERT FAILED]"); \
                logger.error("Expression: {}", #expr); \
                logger.error("File: {}:{}", __FILE__, __LINE__); \
                \
                if (sizeof(#__VA_ARGS__) > 1) { \
                    char _assert_buffer[512]; \
                    std::snprintf(_assert_buffer, sizeof(_assert_buffer), __VA_ARGS__); \
                    logger.error("Message: {}", _assert_buffer); \
                } \
                logger.fatal("Aborting due to failed assertion"); \
                std::abort(); \
            } \
        } while (0)
#else
    #define ENGINE_ASSERT(expr, ...) ((void)0)
#endif

#endif // engine_core_debug_ASSERT_HPP