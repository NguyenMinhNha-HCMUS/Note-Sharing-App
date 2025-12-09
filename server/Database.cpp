#include "Database.h"
#include <iostream>
#include <ctime>
#include "../common/Crypto.h"

Database::Database() {
    if (sqlite3_open("secure_notes.db", &db)) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
    }
}

Database::~Database() {
    sqlite3_close(db);
}

bool Database::init() {
    const char* sqlUsers = R"(
        CREATE TABLE IF NOT EXISTS Users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            receive_public_key_hex TEXT NOT NULL
        );
    )";

    const char* sqlNotes = R"(
        CREATE TABLE IF NOT EXISTS Notes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            encrypted_content TEXT NOT NULL,
            wrapped_key TEXT NOT NULL,
            iv_hex TEXT NOT NULL,
            created_at INTEGER NOT NULL,
            FOREIGN KEY (user_id) REFERENCES Users(id)
        );
    )";

    const char* sqlSharedLinks = R"(
        CREATE TABLE IF NOT EXISTS SharedLinks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            token TEXT UNIQUE NOT NULL,
            note_id INTEGER NOT NULL,
            owner_id INTEGER NOT NULL,
            expiration_time INTEGER NOT NULL,
            FOREIGN KEY (note_id) REFERENCES Notes(id),
            FOREIGN KEY (owner_id) REFERENCES Users(id)
        );
    )";

    const char* sqlSharedLinkAccess = R"(
        CREATE TABLE IF NOT EXISTS SharedLinkAccess (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            link_id INTEGER NOT NULL,
            username TEXT NOT NULL,
            send_public_key_hex TEXT NOT NULL,
            wrapped_key TEXT NOT NULL,
            FOREIGN KEY (link_id) REFERENCES SharedLinks(id)
        );
    )";

    const char* sqlUserShares = R"(
        CREATE TABLE IF NOT EXISTS UserShares (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            note_id INTEGER NOT NULL,
            sender_id INTEGER NOT NULL,
            recipient_id INTEGER NOT NULL,
            send_public_key_hex TEXT NOT NULL,
            new_wrapped_key TEXT NOT NULL,
            expiration_time INTEGER NOT NULL,
            FOREIGN KEY (note_id) REFERENCES Notes(id),
            FOREIGN KEY (sender_id) REFERENCES Users(id),
            FOREIGN KEY (recipient_id) REFERENCES Users(id)
        );
    )";

    char* errMsg = nullptr;
    
    if (sqlite3_exec(db, sqlUsers, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create Users table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    if (sqlite3_exec(db, sqlNotes, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create Notes table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    if (sqlite3_exec(db, sqlSharedLinks, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create SharedLinks table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    if (sqlite3_exec(db, sqlSharedLinkAccess, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create SharedLinkAccess table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    if (sqlite3_exec(db, sqlUserShares, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create UserShares table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    std::cout << "Database initialized successfully" << std::endl;
    return true;
}

bool Database::createUser(std::string username, std::string pass_hash, 
                          std::string salt, std::string receive_pub_key) {
    const char* sql = "INSERT INTO Users (username, password_hash, salt, receive_public_key_hex) VALUES (?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare insert user statement" << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pass_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, receive_pub_key.c_str(), -1, SQLITE_TRANSIENT);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_CONSTRAINT) {
        std::cerr << "Username already exists" << std::endl;
        return false;
    }
    
    return rc == SQLITE_DONE;
}

UserRecord Database::getUserByUsername(std::string username) {
    UserRecord record{-1, "", "", "", ""};
    
    const char* sql = "SELECT id, username, password_hash, salt, receive_public_key_hex FROM Users WHERE username = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return record;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int(stmt, 0);
        record.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.receive_public_key_hex = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    }
    
    sqlite3_finalize(stmt);
    return record;
}

bool Database::updateUserPublicKey(int user_id, std::string receive_pub_key) {
    const char* sql = "UPDATE Users SET receive_public_key_hex = ? WHERE id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, receive_pub_key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

int Database::saveNote(int user_id, std::string encrypted_content, 
                       std::string wrapped_key, std::string iv_hex) {
    const char* sql = "INSERT INTO Notes (user_id, encrypted_content, wrapped_key, iv_hex, created_at) VALUES (?, ?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare save note statement" << std::endl;
        return -1;
    }
    
    long now = static_cast<long>(std::time(nullptr));
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, encrypted_content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, wrapped_key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, iv_hex.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, now);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        return -1;
    }
    
    return static_cast<int>(sqlite3_last_insert_rowid(db));
}

NoteData Database::getNoteById(int note_id) {
    NoteData note;
    note.note_id = -1;
    
    const char* sql = "SELECT id, user_id, encrypted_content, wrapped_key, iv_hex, created_at FROM Notes WHERE id = ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return note;
    }
    
    sqlite3_bind_int(stmt, 1, note_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        note.note_id = sqlite3_column_int(stmt, 0);
        note.encrypted_content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        note.wrapped_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        note.iv_hex = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        note.created_at = sqlite3_column_int64(stmt, 5);
    }
    
    sqlite3_finalize(stmt);
    return note;
}

std::vector<NoteData> Database::getNotesForUser(int user_id) {
    std::vector<NoteData> notes;
    
    const char* sql = "SELECT id, encrypted_content, wrapped_key, iv_hex, created_at FROM Notes WHERE user_id = ? ORDER BY created_at DESC";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return notes;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        NoteData note;
        note.note_id = sqlite3_column_int(stmt, 0);
        note.encrypted_content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        note.wrapped_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        note.iv_hex = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        note.created_at = sqlite3_column_int64(stmt, 4);
        notes.push_back(note);
    }
    
    sqlite3_finalize(stmt);
    return notes;
}

bool Database::deleteNote(int note_id, int user_id) {
    // First verify ownership
    const char* checkSql = "SELECT id FROM Notes WHERE id = ? AND user_id = ?";
    sqlite3_stmt* checkStmt;
    if (sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(checkStmt, 1, note_id);
    sqlite3_bind_int(checkStmt, 2, user_id);
    
    bool owned = (sqlite3_step(checkStmt) == SQLITE_ROW);
    sqlite3_finalize(checkStmt);
    
    if (!owned) {
        return false;
    }
    
    // Delete associated user shares
    const char* deleteSharesSql = "DELETE FROM UserShares WHERE note_id = ?";
    sqlite3_stmt* delSharesStmt;
    if (sqlite3_prepare_v2(db, deleteSharesSql, -1, &delSharesStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(delSharesStmt, 1, note_id);
        sqlite3_step(delSharesStmt);
        sqlite3_finalize(delSharesStmt);
    }
    
    // Delete associated shared link access entries and links
    const char* getLinksSql = "SELECT id FROM SharedLinks WHERE note_id = ?";
    sqlite3_stmt* getLinksStmt;
    if (sqlite3_prepare_v2(db, getLinksSql, -1, &getLinksStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(getLinksStmt, 1, note_id);
        while (sqlite3_step(getLinksStmt) == SQLITE_ROW) {
            int linkId = sqlite3_column_int(getLinksStmt, 0);
            
            const char* delAccessSql = "DELETE FROM SharedLinkAccess WHERE link_id = ?";
            sqlite3_stmt* delAccessStmt;
            if (sqlite3_prepare_v2(db, delAccessSql, -1, &delAccessStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(delAccessStmt, 1, linkId);
                sqlite3_step(delAccessStmt);
                sqlite3_finalize(delAccessStmt);
            }
        }
        sqlite3_finalize(getLinksStmt);
    }
    
    const char* deleteLinksSql = "DELETE FROM SharedLinks WHERE note_id = ?";
    sqlite3_stmt* delLinksStmt;
    if (sqlite3_prepare_v2(db, deleteLinksSql, -1, &delLinksStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(delLinksStmt, 1, note_id);
        sqlite3_step(delLinksStmt);
        sqlite3_finalize(delLinksStmt);
    }
    
    // Finally delete the note itself
    const char* deleteNoteSql = "DELETE FROM Notes WHERE id = ?";
    sqlite3_stmt* delNoteStmt;
    if (sqlite3_prepare_v2(db, deleteNoteSql, -1, &delNoteStmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(delNoteStmt, 1, note_id);
    int rc = sqlite3_step(delNoteStmt);
    sqlite3_finalize(delNoteStmt);
    
    return rc == SQLITE_DONE;
}

std::string Database::createShareLink(int note_id, int user_id,
                                      std::vector<UserAccessEntry> user_access_list,
                                      int duration_seconds) {
    // Generate random token
    auto tokenBytes = Crypto::generateRandomBytes(32);
    std::string token = Crypto::toHex(tokenBytes);
    
    long expirationTime = static_cast<long>(std::time(nullptr)) + duration_seconds;
    
    // Insert into SharedLinks
    const char* insertLinkSql = "INSERT INTO SharedLinks (token, note_id, owner_id, expiration_time) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* linkStmt;
    if (sqlite3_prepare_v2(db, insertLinkSql, -1, &linkStmt, nullptr) != SQLITE_OK) {
        return "";
    }
    
    sqlite3_bind_text(linkStmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(linkStmt, 2, note_id);
    sqlite3_bind_int(linkStmt, 3, user_id);
    sqlite3_bind_int64(linkStmt, 4, expirationTime);
    
    if (sqlite3_step(linkStmt) != SQLITE_DONE) {
        sqlite3_finalize(linkStmt);
        return "";
    }
    sqlite3_finalize(linkStmt);
    
    int linkId = static_cast<int>(sqlite3_last_insert_rowid(db));
    
    // Insert access entries for each user
    const char* insertAccessSql = "INSERT INTO SharedLinkAccess (link_id, username, send_public_key_hex, wrapped_key) VALUES (?, ?, ?, ?)";
    for (const auto& entry : user_access_list) {
        sqlite3_stmt* accessStmt;
        if (sqlite3_prepare_v2(db, insertAccessSql, -1, &accessStmt, nullptr) != SQLITE_OK) {
            continue;
        }
        
        sqlite3_bind_int(accessStmt, 1, linkId);
        sqlite3_bind_text(accessStmt, 2, entry.username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(accessStmt, 3, entry.send_public_key_hex.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(accessStmt, 4, entry.wrapped_key.c_str(), -1, SQLITE_TRANSIENT);
        
        sqlite3_step(accessStmt);
        sqlite3_finalize(accessStmt);
    }
    
    return token;
}

Database::ShareLinkData Database::getShareLinkData(std::string token, std::string username) {
    ShareLinkData result{-1, "", "", "", "", false};
    
    long now = static_cast<long>(std::time(nullptr));
    
    // Get link info and check expiration
    const char* getLinkSql = "SELECT id, note_id, expiration_time FROM SharedLinks WHERE token = ?";
    sqlite3_stmt* linkStmt;
    if (sqlite3_prepare_v2(db, getLinkSql, -1, &linkStmt, nullptr) != SQLITE_OK) {
        return result;
    }
    
    sqlite3_bind_text(linkStmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(linkStmt) != SQLITE_ROW) {
        sqlite3_finalize(linkStmt);
        return result;
    }
    
    int linkId = sqlite3_column_int(linkStmt, 0);
    int noteId = sqlite3_column_int(linkStmt, 1);
    long expirationTime = sqlite3_column_int64(linkStmt, 2);
    sqlite3_finalize(linkStmt);
    
    // Check if expired
    if (expirationTime < now) {
        // Delete expired link
        const char* delSql = "DELETE FROM SharedLinks WHERE id = ?";
        sqlite3_stmt* delStmt;
        if (sqlite3_prepare_v2(db, delSql, -1, &delStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(delStmt, 1, linkId);
            sqlite3_step(delStmt);
            sqlite3_finalize(delStmt);
        }
        return result;
    }
    
    // Check if user has access
    const char* getAccessSql = "SELECT send_public_key_hex, wrapped_key FROM SharedLinkAccess WHERE link_id = ? AND username = ?";
    sqlite3_stmt* accessStmt;
    if (sqlite3_prepare_v2(db, getAccessSql, -1, &accessStmt, nullptr) != SQLITE_OK) {
        return result;
    }
    
    sqlite3_bind_int(accessStmt, 1, linkId);
    sqlite3_bind_text(accessStmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(accessStmt) != SQLITE_ROW) {
        sqlite3_finalize(accessStmt);
        return result;
    }
    
    result.send_public_key_hex = reinterpret_cast<const char*>(sqlite3_column_text(accessStmt, 0));
    result.wrapped_key = reinterpret_cast<const char*>(sqlite3_column_text(accessStmt, 1));
    sqlite3_finalize(accessStmt);
    
    // Get note data
    NoteData note = getNoteById(noteId);
    if (note.note_id == -1) {
        return result;
    }
    
    result.note_id = noteId;
    result.encrypted_content = note.encrypted_content;
    result.iv_hex = note.iv_hex;
    result.valid = true;
    
    return result;
}

bool Database::deleteShareLink(std::string token, int user_id) {
    // Verify ownership
    const char* checkSql = "SELECT id FROM SharedLinks WHERE token = ? AND owner_id = ?";
    sqlite3_stmt* checkStmt;
    if (sqlite3_prepare_v2(db, checkSql, -1, &checkStmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_text(checkStmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(checkStmt, 2, user_id);
    
    if (sqlite3_step(checkStmt) != SQLITE_ROW) {
        sqlite3_finalize(checkStmt);
        return false;
    }
    
    int linkId = sqlite3_column_int(checkStmt, 0);
    sqlite3_finalize(checkStmt);
    
    // Delete access entries first
    const char* delAccessSql = "DELETE FROM SharedLinkAccess WHERE link_id = ?";
    sqlite3_stmt* delAccessStmt;
    if (sqlite3_prepare_v2(db, delAccessSql, -1, &delAccessStmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(delAccessStmt, 1, linkId);
        sqlite3_step(delAccessStmt);
        sqlite3_finalize(delAccessStmt);
    }
    
    // Delete the link
    const char* delLinkSql = "DELETE FROM SharedLinks WHERE id = ?";
    sqlite3_stmt* delLinkStmt;
    if (sqlite3_prepare_v2(db, delLinkSql, -1, &delLinkStmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(delLinkStmt, 1, linkId);
    int rc = sqlite3_step(delLinkStmt);
    sqlite3_finalize(delLinkStmt);
    
    return rc == SQLITE_DONE;
}

bool Database::createUserShare(int note_id, int sender_id, int recipient_id,
                               std::string send_public_key_hex, std::string new_wrapped_key,
                               int duration_seconds) {
    long expirationTime = static_cast<long>(std::time(nullptr)) + duration_seconds;
    
    const char* sql = "INSERT INTO UserShares (note_id, sender_id, recipient_id, send_public_key_hex, new_wrapped_key, expiration_time) VALUES (?, ?, ?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, note_id);
    sqlite3_bind_int(stmt, 2, sender_id);
    sqlite3_bind_int(stmt, 3, recipient_id);
    sqlite3_bind_text(stmt, 4, send_public_key_hex.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, new_wrapped_key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, expirationTime);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<int> Database::getSharedNotesForUser(int user_id) {
    std::vector<int> shareIds;
    
    long now = static_cast<long>(std::time(nullptr));
    
    const char* sql = "SELECT id FROM UserShares WHERE recipient_id = ? AND expiration_time > ?";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return shareIds;
    }
    
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int64(stmt, 2, now);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        shareIds.push_back(sqlite3_column_int(stmt, 0));
    }
    
    sqlite3_finalize(stmt);
    return shareIds;
}

Database::ShareInfo Database::getShareInfo(int share_id, int recipient_id) {
    ShareInfo info;
    info.note_id = -1;
    
    long now = static_cast<long>(std::time(nullptr));
    
    const char* sql = R"(
        SELECT us.note_id, us.send_public_key_hex, us.new_wrapped_key, 
               n.encrypted_content, n.iv_hex
        FROM UserShares us
        JOIN Notes n ON us.note_id = n.id
        WHERE us.id = ? AND us.recipient_id = ? AND us.expiration_time > ?
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return info;
    }
    
    sqlite3_bind_int(stmt, 1, share_id);
    sqlite3_bind_int(stmt, 2, recipient_id);
    sqlite3_bind_int64(stmt, 3, now);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        info.note_id = sqlite3_column_int(stmt, 0);
        info.send_public_key_hex = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.new_wrapped_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.encrypted_content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        info.iv_hex = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    }
    
    sqlite3_finalize(stmt);
    return info;
}
