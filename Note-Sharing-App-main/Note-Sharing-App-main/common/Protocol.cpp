#include "Protocol.h"

// Dùng để trao đổi dữ liệu JSON
using json = nlohmann::json;

// ------------------------------------------------------------------
// --- AuthRequest (to_json)
// ------------------------------------------------------------------
void to_json(json& j, const AuthRequest& p) {
    j["username"] = p.username;
    j["password"] = p.password;
    j["receivePublicKey"] = p.receivePublicKey;
}

// ------------------------------------------------------------------
// --- LoginResponse (from_json)
// ------------------------------------------------------------------
void from_json(const json& j, LoginResponse& p) {
    j.at("success").get_to(p.success);
    j.at("token").get_to(p.token);
    j.at("salt").get_to(p.salt);

    // Dùng .contains() hoặc .find() để xử lý các trường tùy chọn (như khi login thất bại)
    if (j.contains("receivePrivateKey")) {
        j.at("receivePrivateKey").get_to(p.receivePrivateKey);
    }
    if (j.contains("receivePublicKey")) {
        j.at("receivePublicKey").get_to(p.receivePublicKey);
    }
    if (j.contains("message")) {
        j.at("message").get_to(p.message);
    } else {
        p.message = "";
    }
}

// ------------------------------------------------------------------
// --- NotePayload (from_json và to_json)
// ------------------------------------------------------------------
void from_json(const json& j, NotePayload& p) {
    j.at("encryptedContent").get_to(p.encryptedContent);
    j.at("wrappedKey").get_to(p.wrappedKey);
    j.at("iv").get_to(p.iv);
    j.at("metadata").get_to(p.metadata);
}

void to_json(json& j, const NotePayload& p) {
    j["encryptedContent"] = p.encryptedContent;
    j["wrappedKey"] = p.wrappedKey;
    j["iv"] = p.iv;
    j["metadata"] = p.metadata;
}

// ------------------------------------------------------------------
// --- NoteShareRequest (to_json)
// ------------------------------------------------------------------
void to_json(json& j, const NoteShareRequest& p) {
    j["noteId"] = p.noteId;
    j["recipientUsername"] = p.recipientUsername;
    j["sendPublicKey"] = p.sendPublicKey;
    j["newWrappedKey"] = p.newWrappedKey;
    j["durationSeconds"] = p.durationSeconds;
}

// ------------------------------------------------------------------
// --- UserAccessItem (to_json)
// ------------------------------------------------------------------
void to_json(json& j, const UserAccessItem& p) {
    j["username"] = p.username;
    j["sendPublicKey"] = p.sendPublicKey;
    j["wrappedKey"] = p.wrappedKey;
}

// ------------------------------------------------------------------
// --- ShareLinkRequest (to_json)
// ------------------------------------------------------------------
void to_json(json& j, const ShareLinkRequest& p) {
    j["noteId"] = p.noteId;
    j["userAccessList"] = p.userAccessList; // json.hpp tự xử lý vector của UserAccessItem
    j["durationSeconds"] = p.durationSeconds;
}

// ------------------------------------------------------------------
// --- SharedNoteAccessResponse (from_json)
// ------------------------------------------------------------------
void from_json(const json& j, SharedNoteAccessResponse& p) {
    j.at("encryptedContent").get_to(p.encryptedContent);
    j.at("sendPublicKey").get_to(p.sendPublicKey);
    j.at("wrappedKey").get_to(p.wrappedKey);
    j.at("iv").get_to(p.iv);
    if (j.contains("metadata")) {
        j.at("metadata").get_to(p.metadata);
    }
}

// ------------------------------------------------------------------
// --- SharedNoteReceiveResponse (from_json)
// ------------------------------------------------------------------
void from_json(const json& j, SharedNoteReceiveResponse& p) {
    j.at("noteId").get_to(p.noteId);
    j.at("sendPublicKey").get_to(p.sendPublicKey);
    j.at("newWrappedKey").get_to(p.newWrappedKey);
    j.at("encryptedContent").get_to(p.encryptedContent);
    j.at("iv").get_to(p.iv);
    if (j.contains("metadata")) {
        j.at("metadata").get_to(p.metadata);
    }
}

// ------------------------------------------------------------------
// --- NoteMetadata (from_json)
// ------------------------------------------------------------------
void from_json(const json& j, NoteMetadata& p) {
    j.at("noteId").get_to(p.noteId);
    j.at("createdAt").get_to(p.createdAt);
    j.at("metadata").get_to(p.metadata);
}