#ifndef engine_core_debug_ASSERT_HPP
#define engine_core_debug_ASSERT_HPP

#include <cstdlib>
#include <iostream>

namespace engine::core::debug {

inline void HandleAssertFailure(
    const char* expr,
    const char* file,
    int line,
    const char* message = nullptr)
{
#ifndef NDEBUG
    std::cerr << "\n[ENGINE ASSERT FAILED]\n"
              << "Expression: " << expr << "\n"
              << "File: " << file << ":" << line << "\n";
    if (message) std::cerr << "Message: " << message << "\n";
    std::abort();
#endif
}

} // namespace engine::core::debug

#ifndef NDEBUG
#define ENGINE_ASSERT(expr, ...) \
    do { \
        if (!(expr)) { \
            engine::core::debug::HandleAssertFailure(#expr, __FILE__, __LINE__, ##__VA_ARGS__); \
        } \
    } while (0)
#else
#define ENGINE_ASSERT(expr, ...) ((void)0)
#endif

#endif // engine_core_debug_ASSERT_HPP