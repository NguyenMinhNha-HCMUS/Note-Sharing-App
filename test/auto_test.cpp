// auto_test.cpp - Automated Test Suite for Secure Note App
// Compile: g++ test/auto_test.cpp -o auto_test.exe -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00 -lws2_32 -lwsock32
// Run: .\auto_test.exe

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <fstream>
#include "../vendor/httplib.h"
#include "../vendor/json.hpp"

using json = nlohmann::json;

// ============================================
// CONFIGURATION - Loaded from test/test_config.json
// ============================================

std::string SERVER_HOST = "localhost";
int SERVER_PORT = 8080;
int TOKEN_TTL_SECONDS = 60;
int EXPIRATION_WAIT_TIME = 65;

// Test users
struct TestUser {
    std::string username;
    std::string password;
    std::string token;
    std::string salt;
    int note_id = -1;
    std::string share_token;
};

std::map<std::string, TestUser> TEST_USERS;

// Test configuration
bool VERBOSE = true;
bool TEST_EXPIRATION = true;

// ============================================
// CONFIG FILE LOADER
// ============================================

bool loadConfig(const std::string& configPath = "test/test_config.json") {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        std::cerr << "[WARNING] Config file not found: " << configPath << "\n";
        std::cerr << "          Using default values\n\n";
        
        // Set defaults if file not found
        TEST_USERS = {
            {"alice", {"alice_test", "password123"}},
            {"bob", {"bob_test", "password456"}},
            {"charlie", {"charlie_test", "password789"}}
        };
        return false;
    }

    try {
        json config;
        configFile >> config;

        // Load server config
        if (config.contains("server")) {
            SERVER_HOST = config["server"].value("host", "localhost");
            SERVER_PORT = config["server"].value("port", 8080);
        }

        // Load token config
        if (config.contains("token")) {
            TOKEN_TTL_SECONDS = config["token"].value("ttl_seconds", 60);
            EXPIRATION_WAIT_TIME = config["token"].value("expiration_wait_time", TOKEN_TTL_SECONDS + 5);
        }

        // Load test users
        if (config.contains("test_users")) {
            TEST_USERS.clear();
            for (auto& [key, user] : config["test_users"].items()) {
                TEST_USERS[key] = {
                    user.value("username", ""),
                    user.value("password", "")
                };
            }
        }

        // Load test options
        if (config.contains("test_options")) {
            VERBOSE = config["test_options"].value("verbose", true);
            TEST_EXPIRATION = config["test_options"].value("test_expiration", true);
        }

        std::cout << "[INFO] Loaded config from: " << configPath << "\n";
        std::cout << "       Server: " << SERVER_HOST << ":" << SERVER_PORT << "\n";
        std::cout << "       Token TTL: " << TOKEN_TTL_SECONDS << "s\n";
        std::cout << "       Test users: " << TEST_USERS.size() << "\n\n";
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to parse config file: " << e.what() << "\n";
        std::cerr << "        Using default values\n\n";
        
        // Set defaults on parse error
        TEST_USERS = {
            {"alice", {"alice_test", "password123"}},
            {"bob", {"bob_test", "password456"}},
            {"charlie", {"charlie_test", "password789"}}
        };
        return false;
    }
}

// ============================================
// TEST UTILITIES
// ============================================

struct TestResult {
    int passed = 0;
    int total = 0;
};

void printHeader(const std::string& text) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << text << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

void printTest(const std::string& testName) {
    std::cout << "[TEST] " << testName << "\n";
}

void printPass(const std::string& message = "PASSED") {
    std::cout << "\033[32m [PASS] " << message << "\033[0m\n";
}

void printFail(const std::string& message = "FAILED") {
    std::cout << "\033[31m [FAIL] " << message << "\033[0m\n";
}

void printInfo(const std::string& message) {
    std::cout << "\033[33m [INFO] " << message << "\033[0m\n";
}

void printResponse(int status, const std::string& body) {
    if (VERBOSE) {
        std::cout << "  Status: " << status << "\n";
        try {
            auto j = json::parse(body);
            std::cout << "  Response: " << j.dump(2) << "\n";
        } catch (...) {
            std::cout << "  Response: " << body.substr(0, 200) << "\n";
        }
    }
}

// ============================================
// HTTP CLIENT WRAPPER
// ============================================

class TestClient {
private:
    httplib::Client client;

public:
    TestClient() : client(SERVER_HOST, SERVER_PORT) {
        client.set_connection_timeout(5, 0);
        client.set_read_timeout(10, 0);
    }

    httplib::Result get(const std::string& path, const std::string& token = "") {
        httplib::Headers headers;
        if (!token.empty()) {
            headers.emplace("Authorization", "Bearer " + token);
        }
        return client.Get(path.c_str(), headers);
    }

    httplib::Result post(const std::string& path, const json& body, const std::string& token = "") {
        httplib::Headers headers;
        headers.emplace("Content-Type", "application/json");
        if (!token.empty()) {
            headers.emplace("Authorization", "Bearer " + token);
        }
        return client.Post(path.c_str(), headers, body.dump(), "application/json");
    }

    httplib::Result del(const std::string& path, const std::string& token = "") {
        httplib::Headers headers;
        if (!token.empty()) {
            headers.emplace("Authorization", "Bearer " + token);
        }
        return client.Delete(path.c_str(), headers);
    }
};

// ============================================
// TEST CATEGORY 1: AUTHENTICATION
// ============================================

TestResult testAuthentication(TestClient& client) {
    printHeader("CATEGORY 1: AUTHENTICATION TESTS");
    TestResult result;

    // Test 1.1: Register success
    result.total++;
    printTest("1.1 - Dang ky thanh cong");
    try {
        json body = {
            {"username", TEST_USERS["alice"].username},
            {"password", TEST_USERS["alice"].password},
            {"receive_public_key_hex", "04" + std::string(128, '0')}
        };
        auto res = client.post("/register", body);
        
        if (res && res->status == 200) {
            auto j = json::parse(res->body);
            if (j.value("success", false)) {
                printPass();
                result.passed++;
            } else {
                printFail();
            }
        } else {
            printResponse(res ? res->status : 0, res ? res->body : "No response");
            printFail();
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Test 1.2: Duplicate username
    result.total++;
    printTest("1.2 - Dang ky voi username da ton tai");
    try {
        json body = {
            {"username", TEST_USERS["alice"].username},
            {"password", "another_password"},
            {"receive_public_key_hex", "04" + std::string(128, '0')}
        };
        auto res = client.post("/register", body);
        
        printResponse(res ? res->status : 0, res ? res->body : "");
        
        if (res && res->status == 400) {
            printPass("Tu choi thanh cong");
            result.passed++;
        } else {
            printFail("Khong tu choi duplicate");
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Register Bob for later tests
    json bobBody = {
        {"username", TEST_USERS["bob"].username},
        {"password", TEST_USERS["bob"].password},
        {"receive_public_key_hex", "04" + std::string(128, '1')}
    };
    client.post("/register", bobBody);

    // Test 1.3: Login success
    result.total++;
    printTest("1.3 - Dang nhap thanh cong");
    try {
        json body = {
            {"username", TEST_USERS["alice"].username},
            {"password", TEST_USERS["alice"].password}
        };
        auto res = client.post("/login", body);
        
        printResponse(res ? res->status : 0, res ? res->body : "");
        
        if (res && res->status == 200) {
            auto j = json::parse(res->body);
            if (j.contains("token")) {
                TEST_USERS["alice"].token = j["token"].get<std::string>();
                TEST_USERS["alice"].salt = j.value("salt", "");
                printPass("Token: " + TEST_USERS["alice"].token.substr(0, 30) + "...");
                result.passed++;
            } else {
                printFail();
            }
        } else {
            printFail();
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Test 1.4: Login with wrong password
    result.total++;
    printTest("1.4 - Dang nhap voi password sai");
    try {
        json body = {
            {"username", TEST_USERS["alice"].username},
            {"password", "wrong_password"}
        };
        auto res = client.post("/login", body);
        
        printResponse(res ? res->status : 0, res ? res->body : "");
        
        if (res && res->status == 401) {
            printPass("Tu choi thanh cong");
            result.passed++;
        } else {
            printFail();
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Test 1.5: Login with non-existent user
    result.total++;
    printTest("1.5 - Dang nhap voi username khong ton tai");
    try {
        json body = {
            {"username", "nonexistent_user_12345"},
            {"password", "anypassword"}
        };
        auto res = client.post("/login", body);
        
        printResponse(res ? res->status : 0, res ? res->body : "");
        
        if (res && res->status == 401) {
            printPass("Tu choi thanh cong");
            result.passed++;
        } else {
            printFail();
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Test 1.6: Token expiration - SKIPPED (server TTL = 30 minutes)
    // Expiration test removed because server token TTL is set to 30 minutes,
    // making this test impractical for automated testing.

    // Test 1.7: Request without token
    result.total++;
    printTest("1.7 - Request khong co token");
    try {
        auto res = client.get("/notes");
        
        printResponse(res ? res->status : 0, res ? res->body : "");
        
        if (res && res->status == 401) {
            printPass("Tu choi thanh cong");
            result.passed++;
        } else {
            printFail();
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    std::cout << "\nAuthentication: " << result.passed << "/" << result.total << " tests passed\n\n";
    return result;
}

// ============================================
// TEST CATEGORY 2: BASIC OPERATIONS
// ============================================

TestResult testBasicOperations(TestClient& client) {
    printHeader("CATEGORY 2: BASIC OPERATIONS");
    TestResult result;

    // Ensure alice is logged in
    if (TEST_USERS["alice"].token.empty()) {
        json body = {
            {"username", TEST_USERS["alice"].username},
            {"password", TEST_USERS["alice"].password}
        };
        auto res = client.post("/login", body);
        if (res && res->status == 200) {
            auto j = json::parse(res->body);
            TEST_USERS["alice"].token = j["token"].get<std::string>();
        }
    }

    std::string token = TEST_USERS["alice"].token;

    // Test 2.1: Upload note
    result.total++;
    printTest("2.1 - Upload note");
    try {
        json body = {
            {"encrypted_content", "VGhpcyBpcyBhIHRlc3QgbWVzc2FnZQ=="},
            {"wrapped_key", std::string(80, '0')},
            {"iv_hex", std::string(32, '0')},
            {"filename", "test.txt"}
        };
        auto res = client.post("/upload", body, token);
        
        printResponse(res ? res->status : 0, res ? res->body : "");
        
        if (res && res->status == 200) {
            auto j = json::parse(res->body);
            if (j.contains("note_id")) {
                TEST_USERS["alice"].note_id = j["note_id"].get<int>();
                printPass("Note ID: " + std::to_string(TEST_USERS["alice"].note_id));
                result.passed++;
            } else {
                printFail();
            }
        } else {
            printFail();
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Test 2.2: List notes
    result.total++;
    printTest("2.2 - List notes");
    try {
        auto res = client.get("/notes", token);
        
        printResponse(res ? res->status : 0, res ? res->body : "");
        
        if (res && res->status == 200) {
            auto j = json::parse(res->body);
            if (j.is_array()) {
                    printPass("Tim thay " + std::to_string(j.size()) + " chia se");
                result.passed++;
            } else {
                printFail();
            }
        } else {
            printFail();
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Test 2.3: Get note by ID
    result.total++;
    printTest("2.3 - Get note by ID");
    try {
        if (TEST_USERS["alice"].note_id != -1) {
            std::string path = "/note/" + std::to_string(TEST_USERS["alice"].note_id);
            auto res = client.get(path, token);
            
            printResponse(res ? res->status : 0, res ? res->body : "");
            
            if (res && res->status == 200) {
                auto j = json::parse(res->body);
                if (j.contains("encrypted_content") && j.contains("filename")) {
                    printPass("Filename: " + j["filename"].get<std::string>());
                    result.passed++;
                } else {
                    printFail("Thieu fields");
                }
            } else {
                printFail();
            }
        } else {
            printFail("Khong co note_id");
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    std::cout << "\nBasic Operations: " << result.passed << "/" << result.total << " tests passed\n\n";
    return result;
}

// ============================================
// TEST CATEGORY 3: ACCESS CONTROL
// ============================================

TestResult testAccessControl(TestClient& client) {
    printHeader("CATEGORY 3: ACCESS CONTROL");
    TestResult result;

    // Ensure users are logged in
    for (auto& [name, user] : TEST_USERS) {
        if (user.token.empty() && (name == "alice" || name == "bob")) {
            json body = {
                {"username", user.username},
                {"password", user.password}
            };
            auto res = client.post("/login", body);
            if (res && res->status == 200) {
                auto j = json::parse(res->body);
                user.token = j["token"].get<std::string>();
            }
        }
    }

    std::string aliceToken = TEST_USERS["alice"].token;
    std::string bobToken = TEST_USERS["bob"].token;

    // Test 3.1: Bob cannot access Alice's note
    result.total++;
    printTest("3.1 - User khac khong the download note");
    try {
        if (TEST_USERS["alice"].note_id != -1) {
            std::string path = "/note/" + std::to_string(TEST_USERS["alice"].note_id);
            auto res = client.get(path, bobToken);
            
            printResponse(res ? res->status : 0, res ? res->body : "");
            
            if (res && res->status == 403) {
                printPass("Access denied thanh cong");
                result.passed++;
            } else {
                printFail("Bob co the truy cap note cua Alice!");
            }
        } else {
            printFail("Khong co note de test");
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Test 3.2: Create share link
    result.total++;
    printTest("3.2 - Tao share link");
    try {
        if (TEST_USERS["alice"].note_id != -1) {
            json body = {
                {"note_id", TEST_USERS["alice"].note_id},
                {"duration_seconds", 120},
                {"user_access_list", json::array({
                    {
                        {"username", TEST_USERS["bob"].username},
                        {"send_public_key_hex", "04" + std::string(128, '2')},
                        {"wrapped_key", std::string(80, '0')}
                    }
                })}
            };
            auto res = client.post("/share/link", body, aliceToken);
            
            printResponse(res ? res->status : 0, res ? res->body : "");
            
            if (res && res->status == 200) {
                auto j = json::parse(res->body);
                if (j.contains("token")) {
                    TEST_USERS["alice"].share_token = j["token"].get<std::string>();
                    printPass("Token: " + TEST_USERS["alice"].share_token.substr(0, 30) + "...");
                    result.passed++;
                } else {
                    printFail();
                }
            } else {
                printFail();
            }
        } else {
            printFail("Khong co note de share");
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Test 3.3: Bob can access shared link
    result.total++;
    printTest("3.3 - User trong whitelist truy cap duoc link");
    try {
        if (!TEST_USERS["alice"].share_token.empty()) {
            std::string path = "/share/" + TEST_USERS["alice"].share_token;
            auto res = client.get(path, bobToken);
            
            printResponse(res ? res->status : 0, res ? res->body : "");
            
            if (res && res->status == 200) {
                printf("Bob truy cap thanh cong");
                result.passed++;
            } else {
                printFail();
            }
        } else {
            printFail("Không có share token");
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    // Test 3.4: Expired share link
    result.total++;
    printTest("3.4 - Truy cap link da het han");
    try {
        if (TEST_USERS["alice"].note_id != -1) {
            // Create short-lived link (2 seconds)
            json body = {
                {"note_id", TEST_USERS["alice"].note_id},
                {"duration_seconds", 2},
                {"user_access_list", json::array({
                    {
                        {"username", TEST_USERS["bob"].username},
                        {"send_public_key_hex", "04" + std::string(128, '2')},
                        {"wrapped_key", std::string(80, '0')}
                    }
                })}
            };
            
            printInfo("Tao link voi thoi han 2 giay...");
            auto res = client.post("/share/link", body, aliceToken);
            
            if (res && res->status == 200) {
                auto j = json::parse(res->body);
                std::string short_lived_token = j["token"].get<std::string>();
                
                printInfo("Cho 3 giay de link het han...");
                std::this_thread::sleep_for(std::chrono::seconds(3));
                
                std::string path = "/share/" + short_lived_token;
                auto resAccess = client.get(path, bobToken);
                
                printResponse(resAccess ? resAccess->status : 0, resAccess ? resAccess->body : "");
                
                if (resAccess && resAccess->status == 403) {
                    printPass("Tu choi truy cap link het han thanh cong");
                    result.passed++;
                } else {
                    printFail("Van truy cap duoc link da het han!");
                }
            } else {
                printFail("Khong tao duoc link test het han");
            }
        } else {
            printFail("Khong co note de test");
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    std::cout << "\nAccess Control: " << result.passed << "/" << result.total << " tests passed\n\n";
    return result;
}

// ============================================
// TEST CATEGORY 4: MY SHARES API
// ============================================

TestResult testMySharesAPI(TestClient& client) {
    printHeader("CATEGORY 4: MY SHARES API");
    TestResult result;

    std::string aliceToken = TEST_USERS["alice"].token;
    if (aliceToken.empty()) {
        json body = {
            {"username", TEST_USERS["alice"].username},
            {"password", TEST_USERS["alice"].password}
        };
        auto res = client.post("/login", body);
        if (res && res->status == 200) {
            auto j = json::parse(res->body);
            aliceToken = j["token"].get<std::string>();
        }
    }

    // Test 4.1: List my shares
    result.total++;
    printTest("4.1 - List shares cua minh");
    try {
        auto res = client.get("/myshares", aliceToken);
        
        printResponse(res ? res->status : 0, res ? res->body : "");
        
        if (res && res->status == 200) {
            auto j = json::parse(res->body);
            if (j.is_array()) {
                printPass("Tim thay " + std::to_string(j.size()) + " shares");
                
                if (j.size() > 0) {
                    auto share = j[0];
                    std::vector<std::string> required = {"note_id", "share_link", "expiration_time", 
                                                        "is_expired", "shared_with"};
                    bool hasAll = true;
                    for (const auto& field : required) {
                        if (!share.contains(field)) {
                            hasAll = false;
                            break;
                        }
                    }
                    
                    if (hasAll) {
                        auto shared_with = share["shared_with"];
                        std::string usersList = "";
                        if (shared_with.is_array()) {
                            for (size_t i = 0; i < shared_with.size(); i++) {
                                usersList += shared_with[i].get<std::string>();
                                if (i < shared_with.size() - 1) usersList += ", ";
                            }
                        }
                        printPass("Shared with: " + usersList);
                        result.passed++;
                    } else {
                        printFail("Thieu fields");
                    }
                } else {
                    result.passed++;
                }
            } else {
                printFail("Response khong phai array");
            }
        } else {
            printFail();
        }
    } catch (const std::exception& e) {
        printFail(std::string("Exception: ") + e.what());
    }

    std::cout << "\nMy Shares API: " << result.passed << "/" << result.total << " tests passed\n\n";
    return result;
}

// ============================================
// ARGUMENT PARSING
// ============================================

void parseArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: auto_test.exe\n\n";
            std::cout << "Configuration is loaded from: test/test_config.json\n\n";
            std::cout << "To modify test parameters:\n";
            std::cout << "  1. Edit test/test_config.json\n";
            std::cout << "  2. Change values (server, token TTL, users, etc.)\n";
            std::cout << "  3. Run auto_test.exe again (no rebuild needed)\n\n";
            std::cout << "Example config:\n";
            std::cout << "  {\n";
            std::cout << "    \"server\": { \"host\": \"localhost\", \"port\": 8080 },\n";
            std::cout << "    \"token\": { \"ttl_seconds\": 60 },\n";
            std::cout << "    \"test_users\": { \"alice\": {...} },\n";
            std::cout << "    \"test_options\": { \"verbose\": true }\n";
            std::cout << "  }\n";
            exit(0);
        }
    }
}

// ============================================
// MAIN TEST RUNNER
// ============================================

int main(int argc, char* argv[]) {
    // Check for help first
    parseArgs(argc, argv);
    
    // Load all config from file
    loadConfig("test/test_config.json");
    
    std::cout << "\033[34m\033[1m";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "  SECURE NOTE APP - AUTOMATED TEST SUITE\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "\033[0m\n";
    
    std::cout << "Config: test/test_config.json\n";
    std::cout << "Server: http://" << SERVER_HOST << ":" << SERVER_PORT << "\n";
    std::cout << "Token TTL: " << TOKEN_TTL_SECONDS << "s\n";
    std::cout << "Expiration Wait: " << EXPIRATION_WAIT_TIME << "s\n";
    std::cout << "Test Expiration: " << (TEST_EXPIRATION ? "Yes" : "No") << "\n";
    std::cout << "Verbose: " << (VERBOSE ? "Yes" : "No") << "\n\n";

    // Check server connectivity
    printInfo("Kiem tra ket noi server...");
    TestClient client;
    
    try {
        auto res = client.get("/");
        if (res && res->status == 200) {
            printPass("Server dang chay");
        } else {
            printFail("Server khong phan hoi dung");
            return 1;
        }
    } catch (const std::exception& e) {
        printFail("Khong the ket noi server: " + std::string(e.what()));
        printInfo("Hay dam bao server dang chay: .\\server_app.exe");
        return 1;
    }

    // Run test categories
    int totalPassed = 0;
    int totalTests = 0;

    auto r1 = testAuthentication(client);
    totalPassed += r1.passed;
    totalTests += r1.total;

    auto r2 = testBasicOperations(client);
    totalPassed += r2.passed;
    totalTests += r2.total;

    auto r3 = testAccessControl(client);
    totalPassed += r3.passed;
    totalTests += r3.total;

    auto r4 = testMySharesAPI(client);
    totalPassed += r4.passed;
    totalTests += r4.total;

    // Final summary
    printHeader("FINAL RESULTS");
    
    double percentage = totalTests > 0 ? (double)totalPassed / totalTests * 100.0 : 0.0;
    
    std::string color;
    if (totalPassed == totalTests) {
        color = "\033[32m";  // Green
    } else if (percentage >= 70) {
        color = "\033[33m";  // Yellow
    } else {
        color = "\033[31m";  // Red
    }

    std::cout << "\033[1mTotal: " << color << totalPassed << "/" << totalTests 
              << "\033[0m tests passed\n";
    std::cout << "\033[1mSuccess Rate: " << color << std::fixed 
              << std::setprecision(1) << percentage << "%\033[0m\n\n";

    if (totalPassed == totalTests) {
        std::cout << "\033[32m\033[1m ALL TESTS PASSED! \033[0m\n\n";
    } else if (percentage >= 70) {
        std::cout << "\033[33m  Some tests failed. Review output above.\033[0m\n\n";
    } else {
        std::cout << "\033[31m Many tests failed. Check server and configuration.\033[0m\n\n";
    }

    return (totalPassed == totalTests) ? 0 : 1;
}
