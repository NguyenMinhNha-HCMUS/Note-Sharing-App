#!/bin/bash

# Secure Note App - macOS Build Script

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
VENDOR_PATH="$PROJECT_ROOT/vendor"
COMMON_PATH="$PROJECT_ROOT/common"
SERVER_PATH="$PROJECT_ROOT/server"
CLIENT_PATH="$PROJECT_ROOT/client"

echo "=========================================="
echo "    SECURE NOTE APP - macOS BUILD"
echo "=========================================="

# Create vendor directory
mkdir -p "$VENDOR_PATH"

# Download dependencies if not present
echo ""
echo "[1/6] Checking dependencies..."

# json.hpp
if [ ! -f "$VENDOR_PATH/json.hpp" ]; then
    echo "Downloading json.hpp..."
    curl -sL "https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp" -o "$VENDOR_PATH/json.hpp"
fi

# httplib.h
if [ ! -f "$VENDOR_PATH/httplib.h" ]; then
    echo "Downloading httplib.h..."
    curl -sL "https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h" -o "$VENDOR_PATH/httplib.h"
fi

# crow_all.h - Use latest version for Boost compatibility
if [ ! -f "$VENDOR_PATH/crow_all.h" ]; then
    echo "Downloading crow_all.h (v1.3.0 for Boost compatibility)..."
    curl -sL "https://github.com/CrowCpp/Crow/releases/download/v1.3.0/crow_all.h" -o "$VENDOR_PATH/crow_all.h"
    if [ $? -ne 0 ]; then
        echo "Failed to download v1.3.0, trying v1.0+5..."
        curl -sL "https://github.com/CrowCpp/Crow/releases/download/v1.0%2B5/crow_all.h" -o "$VENDOR_PATH/crow_all.h"
    fi
fi

# SQLite3
if [ ! -f "$VENDOR_PATH/sqlite3.c" ]; then
    echo "Downloading SQLite3..."
    curl -sL "https://www.sqlite.org/2023/sqlite-amalgamation-3420000.zip" -o "$VENDOR_PATH/sqlite.zip"
    unzip -q "$VENDOR_PATH/sqlite.zip" -d "$VENDOR_PATH"
    mv "$VENDOR_PATH"/sqlite-amalgamation-*/sqlite3.c "$VENDOR_PATH/"
    mv "$VENDOR_PATH"/sqlite-amalgamation-*/sqlite3.h "$VENDOR_PATH/"
    rm -rf "$VENDOR_PATH/sqlite.zip" "$VENDOR_PATH"/sqlite-amalgamation-*
fi

echo "Dependencies ready."

# Detect OpenSSL location
echo ""
echo "[2/5] Detecting OpenSSL..."

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

echo "Found OpenSSL at: $OPENSSL_PREFIX"

OPENSSL_INCLUDE="-I$OPENSSL_PREFIX/include"
OPENSSL_LIB="-L$OPENSSL_PREFIX/lib"

# Detect Boost location
echo ""
echo "[3/5] Detecting Boost..."

BOOST_PREFIX=""
if [ -d "/opt/homebrew/opt/boost" ]; then
    BOOST_PREFIX="/opt/homebrew/opt/boost"
elif [ -d "/usr/local/opt/boost" ]; then
    BOOST_PREFIX="/usr/local/opt/boost"
elif [ -d "/opt/homebrew/Cellar/boost" ]; then
    # Find the latest boost version
    BOOST_PREFIX=$(ls -d /opt/homebrew/Cellar/boost/* 2>/dev/null | head -1)
elif [ -d "/usr/local/Cellar/boost" ]; then
    BOOST_PREFIX=$(ls -d /usr/local/Cellar/boost/* 2>/dev/null | head -1)
fi

if [ -z "$BOOST_PREFIX" ]; then
    echo "ERROR: Boost not found. Install with: brew install boost"
    exit 1
fi

echo "Found Boost at: $BOOST_PREFIX"

BOOST_INCLUDE="-I$BOOST_PREFIX/include"
BOOST_LIB="-L$BOOST_PREFIX/lib"

# Build SQLite3 as C object first
echo ""
echo "[4/5] Compiling SQLite3..."

SQLITE_OBJ="$VENDOR_PATH/sqlite3.o"
if [ ! -f "$SQLITE_OBJ" ] || [ "$VENDOR_PATH/sqlite3.c" -nt "$SQLITE_OBJ" ]; then
    gcc -c -O2 -DSQLITE_THREADSAFE=1 "$VENDOR_PATH/sqlite3.c" -o "$SQLITE_OBJ"
    if [ $? -ne 0 ]; then
        echo "ERROR: SQLite3 compilation failed"
        exit 1
    fi
    echo "SQLite3 compiled successfully"
else
    echo "SQLite3 object file is up to date"
fi

# Build Server
echo ""
echo "[5/5] Building Server..."

SERVER_SRC="$SERVER_PATH/server_main.cpp $SERVER_PATH/Database.cpp $SERVER_PATH/Auth.cpp $COMMON_PATH/Crypto.cpp"
SERVER_OUT="$PROJECT_ROOT/server_app"

# Detect Asio location (for Crow v1.3.0+)
ASIO_INCLUDE=""
if [ -d "/opt/homebrew/include" ] && [ -f "/opt/homebrew/include/asio.hpp" ]; then
    ASIO_INCLUDE="-I/opt/homebrew/include"
elif [ -d "/usr/local/include" ] && [ -f "/usr/local/include/asio.hpp" ]; then
    ASIO_INCLUDE="-I/usr/local/include"
fi

# Compiler flags for macOS
CXX_FLAGS="-std=c++17 -O2 -pthread"
INCLUDE_FLAGS="-I$VENDOR_PATH -I$COMMON_PATH -I$SERVER_PATH $OPENSSL_INCLUDE $BOOST_INCLUDE $ASIO_INCLUDE"
LINK_FLAGS="$OPENSSL_LIB $BOOST_LIB -lssl -lcrypto"

echo "Compiling server..."
g++ $CXX_FLAGS $INCLUDE_FLAGS $SERVER_SRC "$SQLITE_OBJ" -o "$SERVER_OUT" $LINK_FLAGS

if [ $? -eq 0 ]; then
    echo "Server built successfully: $SERVER_OUT"
else
    echo "ERROR: Server build failed"
    exit 1
fi

# Build Client (optional, skeleton only for now)
echo ""
echo "[6/6] Building Client..."

CLIENT_SRC="$PROJECT_ROOT/client_main.cpp $CLIENT_PATH/client_app_logic.cpp $CLIENT_PATH/network.cpp $COMMON_PATH/Crypto.cpp"
CLIENT_OUT="$PROJECT_ROOT/client_app"

echo "Compiling client..."
g++ $CXX_FLAGS $INCLUDE_FLAGS -I$CLIENT_PATH $CLIENT_SRC -o "$CLIENT_OUT" $LINK_FLAGS 2>/dev/null

if [ $? -eq 0 ]; then
    echo "Client built successfully: $CLIENT_OUT"
else
    echo "WARNING: Client build skipped (not fully implemented yet)"
fi

echo ""
echo "=========================================="
echo "    BUILD COMPLETE"
echo "=========================================="
echo ""
echo "To run the server:"
echo "  ./server_app"
echo ""
echo "The server will start on http://localhost:8080"
echo ""

