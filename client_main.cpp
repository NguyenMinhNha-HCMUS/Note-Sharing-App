#include <iostream>
#include <filesystem>
#include "client/client_app_logic.h"

namespace fs = std::filesystem;

int main() {
    AppLogic app;
    int choice;

    // Tạo thư mục upload và download nếu chưa có
    if (!fs::exists("upload")) {
        fs::create_directory("upload");
        std::cout << "[INFO] Tao thu muc 'upload/' de chua cac file can upload.\n";
    }
    if (!fs::exists("download")) {
        fs::create_directory("download");
        std::cout << "[INFO] Tao thu muc 'download/' de chua cac file tai ve.\n";
    }

    while (true) {
        std::cout << "\n=== SECURE NOTE APP ===\n";
        std::cout << "1. Dang ky\n";
        std::cout << "2. Dang nhap\n";
        std::cout << "3. Upload file\n";
        std::cout << "4. Tai file ve\n";
        std::cout << "5. Liet ke ghi chu\n";
        std::cout << "6. Xoa ghi chu\n";
        std::cout << "7. Tao link chia se tam thoi\n";
        std::cout << "8. Xem ghi chu ban da chia se\n";        
        std::cout << "9. Truy cap link chia se\n";
        std::cout << "10. Huy chia se\n";
        //std::cout << "10. Xem ghi chu duoc chia se\n";
        std::cout << "0. Thoat\n";
        std::cout << "Chon: ";
        std::cin >> choice;

        if (choice == 1) {
            std::string u, p;
            std::cout << "User: "; std::cin >> u;
            std::cout << "Pass: "; std::cin >> p;
            if (app.registerUser(u, p)) std::cout << "Register OK!\n";
            else std::cout << "Register Failed (Not Implemented)!\n";
        }
        else if (choice == 2) {
            std::string u, p;
            std::cout << "User: "; std::cin >> u;
            std::cout << "Pass: "; std::cin >> p;
            if (app.login(u, p)) std::cout << "Login OK!\n";
            else std::cout << "Login Failed!\n";
        }
        else if (choice == 3) {
            std::string filename;
            std::cout << "Nhap ten file (trong thu muc upload/): "; std::cin >> filename;
            std::string fullpath = "upload/" + filename;
            
            if (!fs::exists(fullpath)) {
                std::cout << "[ERROR] File khong ton tai: " << fullpath << "\n";
            } else {
                app.uploadFile(fullpath);
            }
        }
        else if (choice == 4) {
            int id;
            std::cout << "Nhap ID file: "; std::cin >> id;
            app.downloadFile(id);
        }
        else if (choice == 5) {
            app.listNotes();
        }
        else if (choice == 6) {
            int id;
            std::cout << "Nhap ID ghi chu can xoa: "; std::cin >> id;
            app.deleteNote(id);
        }
        else if (choice == 7) {
            int id, duration, num_users;
            std::cout << "Nhap ID ghi chu: "; std::cin >> id;
            std::cout << "So luong nguoi duoc phep truy cap: "; std::cin >> num_users;
            std::vector<std::string> usernames;
            for (int i = 0; i < num_users; i++) {
                std::string user;
                std::cout << "Nhap username thu " << (i+1) << ": "; std::cin >> user;
                usernames.push_back(user);
            }
            std::cout << "Thoi gian het han (giay): "; std::cin >> duration;
            std::string link = app.createShareLink(id, usernames, duration);
            std::cout << "Link chia se: " << link << "\n";
            std::cout << "Chi cac user da chi dinh moi truy cap duoc!\n";
        }
        else if (choice == 8) {
            app.listMyShares();
        }
        else if (choice == 9) {
            std::string token;
            std::cout << "Nhap link chia se: "; std::cin >> token;
            app.accessSharedNote(token);
        }
        else if (choice == 10) {
            std::string token;
            std::cout << "Nhap link chia se can huy: "; std::cin >> token;
            app.revokeShare(token);
        }
        else if (choice == 0) break;
    }
    return 0;
}