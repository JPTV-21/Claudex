#pragma once

// Minimal dependency-free test harness. Tests register themselves at static
// initialization time and are executed by tests/test_main.cpp.

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace testfw {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> body) {
        registry().push_back({name, std::move(body)});
    }
};

// Thrown by REQUIRE to stop the current test after a fatal failure.
struct AbortTest {};

inline int& failureCount() {
    static int count = 0;
    return count;
}

inline void reportFailure(const char* file, int line,
                          const std::string& message) {
    std::cerr << "  FAIL " << file << ':' << line << ": " << message << '\n';
    ++failureCount();
}

inline std::string show(const std::string& value);
inline std::string show(const char* value);
inline std::string show(bool value);
inline std::string show(const std::filesystem::path& value);

template <class T>
std::string show(const T& value) {
    if constexpr (requires(std::ostream& out, const T& item) { out << item; }) {
        std::ostringstream out;
        out << value;
        return out.str();
    } else if constexpr (requires { value.begin(); value.end(); }) {
        std::string text = "[";
        bool first = true;
        for (const auto& item : value) {
            if (!first) {
                text += ", ";
            }
            first = false;
            text += show(item);
        }
        return text + "]";
    } else {
        return "<unprintable>";
    }
}
inline std::string show(const std::string& value) {
    return "\"" + value + "\"";
}
inline std::string show(const char* value) {
    return std::string("\"") + value + "\"";
}
inline std::string show(bool value) { return value ? "true" : "false"; }
inline std::string show(const std::filesystem::path& value) {
    return "path(" + value.generic_string() + ")";
}

template <class A, class B>
bool equal(const A& a, const B& b) {
    return a == b;
}

// A uniquely named scratch directory that is removed on destruction.
class TempDir {
public:
    TempDir() {
        static std::atomic<unsigned> counter{0};
        const auto stamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        std::error_code ec;
        const std::filesystem::path base =
            std::filesystem::temp_directory_path(ec);
        path_ = base / ("claudex-test-" + std::to_string(stamp) + "-" +
                        std::to_string(counter++));
        std::filesystem::create_directories(path_, ec);
        // Canonicalize so comparisons survive symlinked temp roots.
        std::filesystem::path canonical =
            std::filesystem::weakly_canonical(path_, ec);
        if (!ec) {
            path_ = std::move(canonical);
        }
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

inline void writeFile(const std::filesystem::path& file,
                      const std::string& content) {
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    std::ofstream out(file, std::ios::binary);
    out << content;
}

inline std::string readFile(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

}  // namespace testfw

#define TF_CONCAT_INNER(a, b) a##b
#define TF_CONCAT(a, b) TF_CONCAT_INNER(a, b)

#define TEST_CASE(name)                                                    \
    static void TF_CONCAT(tf_test_, __LINE__)();                           \
    static testfw::Registrar TF_CONCAT(tf_registrar_, __LINE__)(           \
        name, &TF_CONCAT(tf_test_, __LINE__));                             \
    static void TF_CONCAT(tf_test_, __LINE__)()

#define CHECK(expr)                                                        \
    do {                                                                   \
        if (!(expr)) {                                                     \
            testfw::reportFailure(__FILE__, __LINE__, "CHECK(" #expr ")"); \
        }                                                                  \
    } while (false)

#define CHECK_FALSE(expr) CHECK(!(expr))

#define CHECK_EQ(actual, expected)                                         \
    do {                                                                   \
        const auto& tf_a = (actual);                                       \
        const auto& tf_e = (expected);                                     \
        if (!testfw::equal(tf_a, tf_e)) {                                  \
            testfw::reportFailure(                                         \
                __FILE__, __LINE__,                                        \
                std::string(#actual " == " #expected "\n    actual:   ") + \
                    testfw::show(tf_a) + "\n    expected: " +             \
                    testfw::show(tf_e));                                   \
        }                                                                  \
    } while (false)

#define REQUIRE(expr)                                                      \
    do {                                                                   \
        if (!(expr)) {                                                     \
            testfw::reportFailure(__FILE__, __LINE__,                      \
                                  "REQUIRE(" #expr ")");                   \
            throw testfw::AbortTest{};                                     \
        }                                                                  \
    } while (false)

#define REQUIRE_MESSAGE(expr, message)                                     \
    do {                                                                   \
        if (!(expr)) {                                                     \
            testfw::reportFailure(__FILE__, __LINE__,                      \
                                  std::string("REQUIRE(" #expr "): ") +    \
                                      (message));                          \
            throw testfw::AbortTest{};                                     \
        }                                                                  \
    } while (false)

#define REQUIRE_NOTHROW(statement)                                         \
    do {                                                                   \
        try {                                                              \
            statement;                                                     \
        } catch (const std::exception& tf_exception) {                     \
            testfw::reportFailure(                                         \
                __FILE__, __LINE__,                                        \
                std::string("threw: ") + tf_exception.what());             \
            throw testfw::AbortTest{};                                     \
        }                                                                  \
    } while (false)
