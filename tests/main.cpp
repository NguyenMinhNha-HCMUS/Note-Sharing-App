#include "test_framework.h"
#include "Database.test.cpp"
#include "Auth.test.cpp"
#include "Crypto.test.cpp"

int main() {
    return TestFramework::instance().runAll();
}

