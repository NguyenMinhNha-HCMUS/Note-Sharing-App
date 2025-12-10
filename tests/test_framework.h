#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

// Simple test framework
class TestFramework {
public:
    struct TestCase {
        std::string name;
        bool (*func)();
    };

    static TestFramework& instance() {
        static TestFramework inst;
        return inst;
    }

    void registerTest(const std::string& name, bool (*func)()) {
        tests.push_back({name, func});
    }

    int runAll() {
        int passed = 0;
        int failed = 0;

        std::cout << "\n==========================================" << std::endl;
        std::cout << "    RUNNING TESTS" << std::endl;
        std::cout << "==========================================\n" << std::endl;

        for (const auto& test : tests) {
            std::cout << "[TEST] " << test.name << " ... ";
            try {
                if (test.func()) {
                    std::cout << "PASSED" << std::endl;
                    passed++;
                } else {
                    std::cout << "FAILED" << std::endl;
                    failed++;
                }
            } catch (const std::exception& e) {
                std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
                failed++;
            } catch (...) {
                std::cout << "FAILED (Unknown exception)" << std::endl;
                failed++;
            }
        }

        std::cout << "\n==========================================" << std::endl;
        std::cout << "    TEST RESULTS" << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Total:  " << (passed + failed) << std::endl;
        std::cout << "==========================================\n" << std::endl;

        return failed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCase> tests;
};

// Macros for easier test writing
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "\n  ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "  Condition: " << #condition << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "\n  ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "  Expected: " << (b) << ", Got: " << (a) << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::cerr << "\n  ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "  Values should not be equal: " << (a) << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_STREQ(a, b) \
    do { \
        if (std::string(a) != std::string(b)) { \
            std::cerr << "\n  ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "  Expected: \"" << (b) << "\", Got: \"" << (a) << "\"" << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST(name) \
    bool test_##name(); \
    static bool _registered_##name = []() { \
        TestFramework::instance().registerTest(#name, test_##name); \
        return true; \
    }(); \
    bool test_##name()

