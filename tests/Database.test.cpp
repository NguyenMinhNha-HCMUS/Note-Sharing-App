#include "test_framework.h"
#include "../server/Database.h"
#include "../common/Crypto.h"
#include <cstdio>
#include <ctime>

// Use a test database file
const char* TEST_DB_FILE = "test_secure_notes.db";

// Helper to clean up test database
void cleanupTestDB() {
    std::remove(TEST_DB_FILE);
    // Also clean up the actual database file used by Database constructor
    std::remove("secure_notes.db");
}

TEST(Database_Init) {
    cleanupTestDB();
    Database db;
    bool result = db.init();
    ASSERT_TRUE(result);
    // Note: Database destructor will close the connection
    cleanupTestDB();
    return true;
}

TEST(Database_CreateUser) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    std::string username = "testuser";
    std::string password = "testpass123";
    auto saltBytes = Crypto::generateRandomBytes(16);
    std::string salt = Crypto::toHex(saltBytes);
    std::string passHash = Crypto::hashSHA256(password + salt);
    std::string pubKey = "test_public_key_hex";
    
    bool result = db.createUser(username, passHash, salt, pubKey);
    ASSERT_TRUE(result);
    
    cleanupTestDB();
    return true;
}

TEST(Database_CreateUser_Duplicate) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    std::string username = "duplicate_user";
    std::string password = "testpass";
    auto saltBytes = Crypto::generateRandomBytes(16);
    std::string salt = Crypto::toHex(saltBytes);
    std::string passHash = Crypto::hashSHA256(password + salt);
    std::string pubKey = "test_key";
    
    // Create user first time
    ASSERT_TRUE(db.createUser(username, passHash, salt, pubKey));
    
    // Try to create duplicate
    bool result = db.createUser(username, passHash, salt, pubKey);
    ASSERT_FALSE(result); // Should fail
    
    cleanupTestDB();
    return true;
}

TEST(Database_GetUserByUsername) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    std::string username = "getuser_test";
    std::string password = "testpass";
    auto saltBytes = Crypto::generateRandomBytes(16);
    std::string salt = Crypto::toHex(saltBytes);
    std::string passHash = Crypto::hashSHA256(password + salt);
    std::string pubKey = "test_pub_key";
    
    ASSERT_TRUE(db.createUser(username, passHash, salt, pubKey));
    
    UserRecord user = db.getUserByUsername(username);
    ASSERT_TRUE(user.id != -1);
    ASSERT_STREQ(user.username.c_str(), username.c_str());
    ASSERT_STREQ(user.password_hash.c_str(), passHash.c_str());
    ASSERT_STREQ(user.salt.c_str(), salt.c_str());
    ASSERT_STREQ(user.receive_public_key_hex.c_str(), pubKey.c_str());
    
    cleanupTestDB();
    return true;
}

TEST(Database_GetUserByUsername_NotFound) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    UserRecord user = db.getUserByUsername("nonexistent_user");
    ASSERT_EQ(user.id, -1);
    
    cleanupTestDB();
    return true;
}

TEST(Database_SaveNote) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    // Create a user first
    std::string username = "note_owner";
    auto saltBytes = Crypto::generateRandomBytes(16);
    std::string salt = Crypto::toHex(saltBytes);
    std::string passHash = Crypto::hashSHA256("password" + salt);
    std::string pubKey = "pub_key";
    ASSERT_TRUE(db.createUser(username, passHash, salt, pubKey));
    
    UserRecord user = db.getUserByUsername(username);
    ASSERT_TRUE(user.id != -1);
    
    // Save a note
    std::string encryptedContent = "encrypted_content_base64";
    std::string wrappedKey = "wrapped_key_base64";
    std::string ivHex = "1234567890abcdef";
    
    int noteId = db.saveNote(user.id, encryptedContent, wrappedKey, ivHex);
    ASSERT_TRUE(noteId > 0);
    
    cleanupTestDB();
    return true;
}

TEST(Database_GetNoteById) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    // Create user
    std::string username = "note_getter";
    auto saltBytes = Crypto::generateRandomBytes(16);
    std::string salt = Crypto::toHex(saltBytes);
    std::string passHash = Crypto::hashSHA256("pass" + salt);
    std::string pubKey = "key";
    ASSERT_TRUE(db.createUser(username, passHash, salt, pubKey));
    
    UserRecord user = db.getUserByUsername(username);
    
    // Save note
    std::string encryptedContent = "test_encrypted_content";
    std::string wrappedKey = "test_wrapped_key";
    std::string ivHex = "abcdef1234567890";
    
    int noteId = db.saveNote(user.id, encryptedContent, wrappedKey, ivHex);
    ASSERT_TRUE(noteId > 0);
    
    // Get note
    NoteData note = db.getNoteById(noteId);
    ASSERT_TRUE(note.note_id == noteId);
    ASSERT_STREQ(note.encrypted_content.c_str(), encryptedContent.c_str());
    ASSERT_STREQ(note.wrapped_key.c_str(), wrappedKey.c_str());
    ASSERT_STREQ(note.iv_hex.c_str(), ivHex.c_str());
    
    cleanupTestDB();
    return true;
}

TEST(Database_GetNotesForUser) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    // Create user
    std::string username = "multi_note_user";
    auto saltBytes = Crypto::generateRandomBytes(16);
    std::string salt = Crypto::toHex(saltBytes);
    std::string passHash = Crypto::hashSHA256("pass" + salt);
    std::string pubKey = "key";
    ASSERT_TRUE(db.createUser(username, passHash, salt, pubKey));
    
    UserRecord user = db.getUserByUsername(username);
    
    // Save multiple notes
    db.saveNote(user.id, "content1", "key1", "iv1");
    db.saveNote(user.id, "content2", "key2", "iv2");
    db.saveNote(user.id, "content3", "key3", "iv3");
    
    // Get all notes
    auto notes = db.getNotesForUser(user.id);
    ASSERT_EQ(notes.size(), 3);
    
    cleanupTestDB();
    return true;
}

TEST(Database_DeleteNote) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    // Create user
    std::string username = "delete_user";
    auto saltBytes = Crypto::generateRandomBytes(16);
    std::string salt = Crypto::toHex(saltBytes);
    std::string passHash = Crypto::hashSHA256("pass" + salt);
    std::string pubKey = "key";
    ASSERT_TRUE(db.createUser(username, passHash, salt, pubKey));
    
    UserRecord user = db.getUserByUsername(username);
    
    // Save note
    int noteId = db.saveNote(user.id, "content", "key", "iv");
    
    // Verify it exists
    auto notes = db.getNotesForUser(user.id);
    ASSERT_EQ(notes.size(), 1);
    
    // Delete note
    bool deleted = db.deleteNote(noteId, user.id);
    ASSERT_TRUE(deleted);
    
    // Verify it's gone
    notes = db.getNotesForUser(user.id);
    ASSERT_EQ(notes.size(), 0);
    
    cleanupTestDB();
    return true;
}

TEST(Database_CreateUserShare) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    // Create sender
    std::string senderName = "sender";
    auto salt1 = Crypto::toHex(Crypto::generateRandomBytes(16));
    std::string hash1 = Crypto::hashSHA256("pass1" + salt1);
    ASSERT_TRUE(db.createUser(senderName, hash1, salt1, "sender_key"));
    UserRecord sender = db.getUserByUsername(senderName);
    
    // Create recipient
    std::string recipientName = "recipient";
    auto salt2 = Crypto::toHex(Crypto::generateRandomBytes(16));
    std::string hash2 = Crypto::hashSHA256("pass2" + salt2);
    ASSERT_TRUE(db.createUser(recipientName, hash2, salt2, "recipient_key"));
    UserRecord recipient = db.getUserByUsername(recipientName);
    
    // Create note
    int noteId = db.saveNote(sender.id, "encrypted", "wrapped", "iv");
    
    // Share note
    bool shared = db.createUserShare(noteId, sender.id, recipient.id, 
                                     "send_pub_key", "new_wrapped_key", 3600);
    ASSERT_TRUE(shared);
    
    // Verify recipient can see shared notes
    auto shareIds = db.getSharedNotesForUser(recipient.id);
    ASSERT_EQ(shareIds.size(), 1);
    
    cleanupTestDB();
    return true;
}

TEST(Database_CreateShareLink) {
    cleanupTestDB();
    Database db;
    ASSERT_TRUE(db.init());
    
    // Create user
    std::string username = "link_owner";
    auto saltBytes = Crypto::generateRandomBytes(16);
    std::string salt = Crypto::toHex(saltBytes);
    std::string passHash = Crypto::hashSHA256("pass" + salt);
    std::string pubKey = "key";
    ASSERT_TRUE(db.createUser(username, passHash, salt, pubKey));
    
    UserRecord user = db.getUserByUsername(username);
    
    // Create note
    int noteId = db.saveNote(user.id, "encrypted", "wrapped", "iv");
    
    // Create share link
    std::vector<Database::UserAccessEntry> accessList;
    Database::UserAccessEntry entry;
    entry.username = "allowed_user";
    entry.send_public_key_hex = "send_key";
    entry.wrapped_key = "wrapped_key";
    accessList.push_back(entry);
    
    std::string token = db.createShareLink(noteId, user.id, accessList, 3600);
    ASSERT_FALSE(token.empty());
    
    cleanupTestDB();
    return true;
}

