#!/bin/bash

# Test Build Script for Secure Note App

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VENDOR_PATH="$PROJECT_ROOT/vendor"
COMMON_PATH="$PROJECT_ROOT/common"
SERVER_PATH="$PROJECT_ROOT/server"
TESTS_PATH="$PROJECT_ROOT/tests"

echo "=========================================="
echo "    BUILDING TESTS"
echo "=========================================="

# Detect OpenSSL location
OPENSSL_PREFIX=""
if [ -d "/opt/homebrew/opt/openssl" ]; then
    OPENSSL_PREFIX="/opt/homebrew/opt/openssl"
elif [ -d "/usr/local/opt/openssl" ]; then
    OPENSSL_PREFIX="/usr/local/opt/openssl"
elif [ -d "/opt/homebrew/opt/openssl@3" ]; then
    OPENSSL_PREFIX="/opt/homebrew/opt/openssl@3"
elif [ -d "/usr/local/opt/openssl@3" ]; then
    OPENSSL_PREFIX="/usr/local/opt/openssl@3"
fi

if [ -z "$OPENSSL_PREFIX" ]; then
    echo "ERROR: OpenSSL not found. Install with: brew install openssl"
    exit 1
fi

OPENSSL_INCLUDE="-I$OPENSSL_PREFIX/include"
OPENSSL_LIB="-L$OPENSSL_PREFIX/lib"

# Build SQLite3 object if needed
SQLITE_OBJ="$VENDOR_PATH/sqlite3.o"
if [ ! -f "$SQLITE_OBJ" ] || [ "$VENDOR_PATH/sqlite3.c" -nt "$SQLITE_OBJ" ]; then
    echo "Compiling SQLite3..."
    gcc -c -O2 -DSQLITE_THREADSAFE=1 "$VENDOR_PATH/sqlite3.c" -o "$SQLITE_OBJ"
    if [ $? -ne 0 ]; then
        echo "ERROR: SQLite3 compilation failed"
        exit 1
    fi
fi

# Compile test sources
echo "Compiling tests..."

CXX_FLAGS="-std=c++17 -O2 -pthread"
INCLUDE_FLAGS="-I$VENDOR_PATH -I$COMMON_PATH -I$SERVER_PATH -I$TESTS_PATH $OPENSSL_INCLUDE"
LINK_FLAGS="$OPENSSL_LIB -lssl -lcrypto"

TEST_SRC="$TESTS_PATH/main.cpp $SERVER_PATH/Database.cpp $SERVER_PATH/Auth.cpp $COMMON_PATH/Crypto.cpp"
TEST_OUT="$TESTS_PATH/test_runner"

echo "Compiling test runner..."
g++ $CXX_FLAGS $INCLUDE_FLAGS $TEST_SRC "$SQLITE_OBJ" -o "$TEST_OUT" $LINK_FLAGS

if [ $? -eq 0 ]; then
    echo "Tests built successfully: $TEST_OUT"
    echo ""
    echo "To run tests:"
    echo "  cd tests && ./test_runner"
    echo ""
else
    echo "ERROR: Test build failed"
    exit 1
fi

