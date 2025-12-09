#include "Database.h"
#include <iostream>
#include "../common/Crypto.h"

// Hàm callback hỗ trợ lấy dữ liệu từ SQLite
static int callback(void* data, int argc, char** argv, char** azColName) {
    // Logic parse dữ liệu dòng từ DB sẽ nằm ở đây
    return 0;
}

Database::Database() {
    if (sqlite3_open("secure_notes.db", &db)) {
        std::cerr << "Lỗi mở DB: " << sqlite3_errmsg(db) << std::endl;
    }
}

Database::~Database() {
    sqlite3_close(db);
}

bool Database::init() {
    // Requirement:
    // 1. Create 'Users' table if not exists (id, username, password_hash, salt, receive_public_key_hex).
    // 2. Create 'Notes' table if not exists (id, user_id, encrypted_content, wrapped_key, iv_hex, created_at).
    // 3. Create 'SharedLinks' table if not exists (id, token, note_id, owner_id, expiration_time).
    // 4. Create 'SharedLinkAccess' table if not exists (link_id, username, send_public_key_hex, wrapped_key).
    // 5. Create 'UserShares' table if not exists (id, note_id, sender_id, recipient_id, send_public_key_hex, new_wrapped_key, expiration_time).
    // 6. Return true if successful, false otherwise.
    return true; 
}

bool Database::createUser(std::string username, std::string pass_hash, std::string salt, std::string receive_pub_key) {
    // Requirement:
    // 1. Insert a new record into 'Users' table with the provided data.
    // 2. Handle unique constraint violations (username).
    // 3. Return true if inserted, false on error.
    return true; 
}

UserRecord Database::getUserByUsername(std::string username) {
    // Requirement:
    // 1. Select user details from 'Users' table where username matches.
    // 2. Return UserRecord struct populated with data.
    // 3. Return record with id = -1 if not found.
    return UserRecord{-1, "", "", "", ""};
}

int Database::saveNote(int user_id, std::string encrypted_content, std::string wrapped_key, std::string iv_hex) {
    // Requirement:
    // 1. Insert a new record into 'Notes' table.
    // 2. Store encrypted_content, wrapped_key, iv_hex, and current timestamp.
    // 3. Return the new note_id, or -1 on error.
    return 1;
}

// Helper for getNoteById
struct NoteCallbackData {
    NoteData note;
    bool found;
};

static int getNoteCallback(void* data, int argc, char** argv, char** azColName) {
    NoteCallbackData* cbData = (NoteCallbackData*)data;
    cbData->found = true;
    // Columns: encrypted_content, wrapped_key, iv_hex
    if (argv[0]) cbData->note.encrypted_content = argv[0];
    if (argv[1]) cbData->note.wrapped_key = argv[1];
    if (argv[2]) cbData->note.iv_hex = argv[2];
    return 0;
}

NoteData Database::getNoteById(int note_id) {
    // Requirement:
    // 1. Select note details from 'Notes' table by id.
    // 2. Return NoteData struct.
    // 3. Return empty/error indicator if not found.
    return NoteData();
}

std::string Database::createShareLink(int note_id, int user_id, 
                                     std::vector<UserAccessEntry> user_access_list,
                                     int duration_seconds) {
    // Requirement:
    // 1. Generate random token (32 bytes hex).
    // 2. Calculate expiration_time = current_time + duration_seconds.
    // 3. Insert into SharedLinks table (token, note_id, owner_id, expiration_time), get link_id.
    // 4. For each entry in user_access_list:
    //    Insert into SharedLinkAccess (link_id, username, send_public_key_hex, wrapped_key).
    // 5. Return token string.
    return "token_placeholder";
}

Database::ShareLinkData Database::getShareLinkData(std::string token, std::string username) {
    // Requirement:
    // 1. Query SharedLinks table by token.
    // 2. Check if expiration_time > current_time.
    // 3. If expired, delete the record and return invalid.
    // 4. Query SharedLinkAccess to check if username is in whitelist.
    // 5. If not authorized, return invalid.
    // 6. If valid, get send_public_key_hex and wrapped_key for this username.
    // 7. JOIN with Notes table to get encrypted_content and iv_hex.
    // 8. Return ShareLinkData with all necessary info.
    return ShareLinkData{-1, "", "", "", "", false};
}

bool Database::deleteShareLink(std::string token, int user_id) {
    // Requirement:
    // 1. Query SharedLinks to verify user_id owns the note.
    // 2. Delete the share link record.
    // 3. Return true if deleted, false otherwise.
    return false;
}

bool Database::createUserShare(int note_id, int sender_id, int recipient_id,
                               std::string send_public_key_hex, std::string new_wrapped_key,
                               int duration_seconds) {
    // Requirement:
    // 1. Calculate expiration_time = current_time + duration_seconds.
    // 2. Insert into UserShares table (note_id, sender_id, recipient_id, send_public_key_hex, new_wrapped_key, expiration_time).
    // 3. Return true if inserted successfully.
    return false;
}

std::vector<int> Database::getSharedNotesForUser(int user_id) {
    // Requirement:
    // 1. Query UserShares table where recipient_id = user_id AND expiration_time > current_time.
    // 2. Return vector of share IDs.
    return {};
}

Database::ShareInfo Database::getShareInfo(int share_id, int recipient_id) {
    // Requirement:
    // 1. Query UserShares JOIN Notes where share.id = share_id AND recipient_id matches.
    // 2. Return ShareInfo with note data and keys.
    return ShareInfo();
}

std::vector<NoteData> Database::getNotesForUser(int user_id) {
    // Requirement:
    // 1. Query Notes table where user_id matches.
    // 2. Return vector of NoteData structs.
    return {};
}

bool Database::deleteNote(int note_id, int user_id) {
    // Requirement:
    // 1. Verify note belongs to user_id.
    // 2. Delete note from Notes table.
    // 3. Delete associated shares from UserShares and SharedLinks.
    // 4. Return true if deleted.
    return false;
}
