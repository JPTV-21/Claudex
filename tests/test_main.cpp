#include "test_framework.hpp"

#include <exception>

// Usage: claudex_tests [substring-filter] [--list]
int main(int argc, char** argv) {
    std::string filter;
    bool listOnly = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--list") {
            listOnly = true;
        } else {
            filter = argument;
        }
    }

    int ran = 0;
    int failed = 0;
    for (const testfw::TestCase& test : testfw::registry()) {
        if (!filter.empty() && test.name.find(filter) == std::string::npos) {
            continue;
        }
        if (listOnly) {
            std::cout << test.name << '\n';
            continue;
        }

        const int before = testfw::failureCount();
        try {
            test.body();
        } catch (const testfw::AbortTest&) {
        } catch (const std::exception& exception) {
            testfw::reportFailure(test.name.c_str(), 0,
                                  std::string("unexpected exception: ") +
                                      exception.what());
        } catch (...) {
            testfw::reportFailure(test.name.c_str(), 0,
                                  "unexpected non-standard exception");
        }

        ++ran;
        if (testfw::failureCount() != before) {
            ++failed;
            std::cerr << "[FAIL] " << test.name << '\n';
        } else {
            std::cout << "[ ok ] " << test.name << '\n';
        }
    }

    if (listOnly) {
        return 0;
    }
    std::cout << '\n' << (ran - failed) << '/' << ran << " tests passed\n";
    return failed == 0 ? 0 : 1;
}
