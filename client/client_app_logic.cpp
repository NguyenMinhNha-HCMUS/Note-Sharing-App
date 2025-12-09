// Client/AppLogic.cpp
#include "client_app_logic.h"
#include "../common/Crypto.h"
#include "../common/Protocol.h"
#include <fstream>
#include <iostream>
#include "../vendor/json.hpp"

AppLogic::AppLogic() {
    net = new Network("http://localhost:8080");
}

bool AppLogic::registerUser(std::string user, std::string pass) {
    // Requirement:
    // 1. Generate ECDH Key Pair (Receive Key).
    // 2. Create AuthRequest with username, password, and receive public key.
    // 3. Send POST /register to Server.
    // 4. Return true if Server responds 200 OK.
    return false;
}

bool AppLogic::login(std::string user, std::string pass) {
    // Requirement:
    // 1. Send POST /login with username and password.
    // 2. If success, parse LoginResponse to get Token and Salt.
    // 3. Store Token.
    // 4. Derive Master Key from Password and Salt using PBKDF2.
    // 5. Store Master Key in RAM.
    return false;
}

void AppLogic::uploadFile(std::string filepath) {
    // Requirement:
    // 1. Read file content from disk (filepath already includes "upload/" prefix).
    // 2. Generate random File Key and IV.
    // 3. Encrypt content with File Key (AES).
    // 4. Encrypt File Key with Master Key (Key Wrapping).
    // 5. Send NotePayload (encrypted_content, wrapped_key, iv) to Server via POST /upload.
    // 6. On success, display note_id to user.
}

void AppLogic::downloadFile(int note_id) {
    // Requirement:
    // 1. Send GET /note/{id} to Server.
    // 2. Receive NotePayload.
    // 3. Decrypt Wrapped Key using Master Key to get File Key.
    // 4. Decrypt Content using File Key.
    // 5. Save decrypted content to "download/note_{note_id}.txt" or original filename if stored.
}

void AppLogic::listNotes() {
    // Requirement:
    // 1. Send GET /notes with auth token.
    // 2. Server returns list of notes (id, created_at, metadata) for current user.
    // 3. Display list to user.
}

void AppLogic::deleteNote(int note_id) {
    // Requirement:
    // 1. Send DELETE /note/{id} with auth token.
    // 2. Server verifies ownership and deletes from database.
    // 3. Return success/failure message.
}

void AppLogic::shareNoteWithUser(int note_id, std::string recipient_username, int duration_seconds) {
    // Requirement:
    // 1. Send GET /user/{recipient_username}/pubkey to get receiver's receive_public_key.
    // 2. Generate ephemeral send_key_pair (send_private_key, send_public_key).
    // 3. Compute session_key = ECDH(send_private_key, receiver_receive_public_key).
    // 4. Download note to get original wrapped_key.
    // 5. Decrypt wrapped_key with master_key to get file_key.
    // 6. Encrypt file_key with session_key to get new_wrapped_key.
    // 7. Send POST /share with NoteShareRequest (note_id, recipient_username, send_public_key, new_wrapped_key, duration_seconds).
    // 8. IMMEDIATELY destroy send_private_key and session_key from memory.
}

std::string AppLogic::createShareLink(int note_id, std::vector<std::string> allowed_usernames, int duration_seconds) {
    // Requirement:
    // 1. Download note to get original wrapped_key and encrypted_content.
    // 2. Decrypt wrapped_key with master_key to get file_key.
    // 3. For each username in allowed_usernames:
    //    a. Send GET /user/{username}/pubkey to get their receive_public_key.
    //    b. Generate ephemeral send_key_pair.
    //    c. Compute session_key = ECDH(send_private_key, user_receive_public_key).
    //    d. Encrypt file_key with session_key -> wrapped_key_for_user.
    //    e. Add {username, send_public_key, wrapped_key_for_user} to list.
    // 4. Send POST /share/link with {note_id, user_access_list, duration_seconds}.
    // 5. Server generates token, stores (token, note_id, user_access_list, expiration_time).
    // 6. Server returns {share_link: "http://localhost:8080/share/{token}"}.
    // 7. Return share_link to display.
    return "";
}

void AppLogic::accessSharedNote(std::string share_token) {
    // Requirement:
    // 1. Send GET /share/{token} with auth token.
    // 2. Server checks: token valid? not expired? current_username in whitelist?
    // 3. If expired, server returns 410 Gone or 404 - display error to user.
    // 4. If valid, server returns {encrypted_content, send_public_key, wrapped_key, iv}.
    // 5. Compute session_key = ECDH(receive_private_key, sender_send_public_key).
    // 6. Decrypt wrapped_key with session_key to get file_key.
    // 7. Decrypt encrypted_content with file_key.
    // 8. Save to "download/shared_{token_prefix}.txt".
}

void AppLogic::revokeShare(std::string share_token) {
    // Requirement:
    // 1. Send DELETE /share/{token} with auth token.
    // 2. Server verifies user owns the note associated with the token.
    // 3. Server deletes the share token from SharedLinks table.
    // 4. Return success message.
}

void AppLogic::receiveSharedNote(int share_id) {
    // Requirement:
    // 1. Send GET /shared/{share_id} with auth token.
    // 2. Server checks if current user is the recipient.
    // 3. Server checks if not expired, if expired returns 410 Gone.
    // 4. If valid, server returns {note_id, send_public_key, new_wrapped_key, encrypted_content, iv}.
    // 5. Compute session_key = ECDH(receive_private_key, sender_send_public_key).
    // 6. Decrypt new_wrapped_key with session_key to get file_key.
    // 7. Decrypt encrypted_content with file_key.
    // 8. Save to "download/shared_note_{share_id}.txt".
}