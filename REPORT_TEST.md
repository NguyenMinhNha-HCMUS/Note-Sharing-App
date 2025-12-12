## 3. KIỂM THỬ VÀ ĐÁNH GIÁ
### 3.1. Phương pháp kiểm thử
Chương trình cung cấp file `auto_test.cpp ` trong thư mục test nhằm kiểm thử tự động chương trình
-  **Mục tiêu**: Kiểm tra sự tương tác giữa Client và Server qua API.
-  **Phạm vi**: Bao phủ các luồng nghiệp vụ chính: Đăng ký, Đăng nhập, Upload, Download, Chia sẻ, và Kiểm soát quyền truy cập.
-  **Cách thức**: Gửi các HTTP Request (POST/GET/DELETE) với payload JSON chuẩn và kiểm tra HTTP Status Code cũng như nội dung JSON trả về.
### 3.2. Cách sử dụng
**Quy trình thực hiện kiểm thử tự động:**
1.  **Chuẩn bị**: Đảm bảo `server_app.exe` đã được build và remote database (nếu đã chạy chương trình trước khi test) bằng lệnh:
```
Remove-Item secure_notes.db
```
3.  **Khởi động Server**: Mở một terminal và chạy server:
```powershell
.\server_app.exe
```
4.  **Build và chạy Test**: Mở terminal thứ hai và build rồi chạy file test:
```powershell
g++ test/auto_test.cpp -o auto_test.exe  -std=c++17  -I vendor -D_WIN32_WINNT=0x0A00  -lws2_32 -lwsock32
.\auto_test.exe
```
5.  **Đánh giá**: Chương trình sẽ tự động chạy lần lượt 13 test cases và in kết quả (PASS/FAIL) ra màn hình với màu sắc trực quan.
### 3.3. Các test case  và kết quả kiểm thử 
**Nhóm 1: Authentication (Xác thực)**
***Test 1.1 - Đăng ký thành công***:
*  **Logic**: Gửi request `POST /register` với thông tin của user **Alice** (username/password được lấy từ từ test_config.json).
*  **Kết quả**: Server trả về HTTP 200 và `success: true`.
* **Ý nghĩa**: Chức năng đăng ký hoạt động, mật khẩu được hash và lưu trữ an toàn, cặp khóa ECDH được sinh thành công.

***Test 1.2 - Đăng ký trùng username***:
*  **Logic**: Gửi lại request `POST /register` với cùng username của **Alice**.
*  **Kết quả**: Server trả về HTTP 400 (Bad Request) để ngăn chặn trùng lặp.
* **Ý nghĩa**: Hệ thống ngăn chặn việc tạo tài khoản trùng lặp, đảm bảo tính toàn vẹn dữ liệu người dùng.

***Test 1.3 - Đăng nhập thành công***:
*  **Logic**: Gửi `POST /login` với đúng username/password của **Alice**.
*  **Kết quả**: Server trả về HTTP 200 kèm theo JWT Token và Salt.
* **Ý nghĩa**: Xác thực người dùng thành công, JWT token được cấp cho user.

***Test 1.4 - Đăng nhập sai password***:
*  **Logic**: Gửi `POST /login` với username của **Alice** nhưng sai password.
*  **Kết quả**: Server trả về HTTP 401 (Unauthorized).
* **Ý nghĩa**: Hệ thống từ chối truy cập khi thông tin xác thực không chính xác, bảo vệ tài khoản người dùng.

***Test 1.5 - Đăng nhập user không tồn tại***:
*  **Logic**: Gửi `POST /login` với một username ngẫu nhiên không tồn tại trong database.
*  **Kết quả**: Server trả về HTTP 401.
* **Ý nghĩa**: Hệ thống xử lý an toàn với các tài khoản không tồn tại.

***Test 1.7 - Request không có token***:
*  **Logic**: Gửi `GET /notes` mà không đính kèm header `Authorization`.
*  **Kết quả**: Server từ chối với HTTP 401.
* **Ý nghĩa**: Đảm bảo yêu cầu xác thực trước khi truy cập.
**Nhóm 2: Chức năng cơ bản**

***Test 2.1 - Upload note***:
*  **Logic**: Dùng token của **Alice** gửi `POST /upload` với nội dung file giả dạng base64 và tên file "test.txt".
*  **Kết quả**: Server lưu thành công, trả về HTTP 200 và `note_id`.
* **Ý nghĩa**: Chức năng upload hoạt động, server chấp nhận và lưu trữ dữ liệu đã mã hóa cùng metadata.

***Test 2.2 - List notes***:
*  **Logic**: Gửi `GET /notes` với token của **Alice**.
*  **Kết quả**: Server trả về danh sách JSON chứa note vừa upload.
* **Ý nghĩa**: Người dùng có thể truy xuất danh sách dữ liệu của chính mình đã upload.

***Test 2.3 - Get note by ID***:
*  **Logic**: Gửi `GET /note/{id}` với ID nhận được từ Test 2.1.
*  **Kết quả**: Server trả về đúng nội dung file, tên file và các metadata khác.
* **Ý nghĩa**: Dữ liệu tải về toàn vẹn, bao gồm nội dung mã hóa và tên file gốc.
**Nhóm 3: Kiểm soát quyền truy cập**

***Test 3.1 - Truy cập trái phép***:
*  **Logic**: Dùng token của **Bob** (đã đăng ký và đăng nhập) để gọi `GET /note/{id}` của **Alice**.
*  **Kết quả**: Server chặn truy cập với HTTP 403 (Forbidden).
* **Ý nghĩa**: Access Control hoạt động hiệu quả. Người dùng không thể truy cập dữ liệu của người khác nếu không được chia sẻ.

***Test 3.2 - Tạo share link (Whitelist)***:
*  **Logic**: **Alice** tạo link chia sẻ note cho **Bob** qua `POST /share/link` (kèm theo Wrapped Key cho Bob).
*  **Kết quả**: Server trả về HTTP 200 và một link share duy nhất.
* **Ý nghĩa**: Chức năng chia sẻ hoạt động, tạo ra token truy cập tạm thời cho người được ủy quyền.

***Test 3.3 - Truy cập link chia sẻ***:
*  **Logic**: **Bob** dùng token của mình để truy cập `GET /share/{share_token}`.
*  **Kết quả**: Server cho phép (HTTP 200) vì **Bob** có trong whitelist.
* **Ý nghĩa**: Người được chia sẻ có thể truy cập nội dung thành công.

***Test 3.4 - Truy cập link đã hết hạn***:
*  **Logic**: **Alice** tạo link chia sẻ cho **Bob** với thời hạn cực ngắn (2 giây). Hệ thống chờ 3 giây rồi dùng token của **Bob** để truy cập.
*  **Kết quả**: Server từ chối truy cập với HTTP 403 (Forbidden).
*  **Ý nghĩa**: Đảm bảo tính năng giới hạn thời gian hoạt động đúng.=
**Nhóm 4: My Shares API**

***Test 4.1 - Kiểm tra danh sách chia sẻ***:
*  **Logic**: **Alice** gọi `GET /myshares`.
*  **Kết quả**: Server trả về danh sách chứa thông tin link chia sẻ vừa tạo, bao gồm danh sách người được chia sẻ (`shared_with`).
* **Ý nghĩa**: Người dùng có thể theo dõi và quản lý các tài nguyên mình đã chia sẻ.

### 3.4 Đánh giá kiểm thử
1. **Xác thực:** Các test case nhóm 1 chứng tỏ rằng đăng ký và đăng nhập hoạt động đúng yêu cầu, đảm bảo độ an toàn.
2. **Mã hóa/Giải mã:** Ở Test 2.1 (Upload), client gửi khóa của file (được mã hóa) lên server cùng vời nội dung file. Ở Test 2.3 (Download), client tải khóa này về, dùng mật khẩu của mình để giải mã ra khóa của file, sau đó dùng khóa của file để giải mã file.
4. **Giới hạn truy cập:** Test 3.4 đảm bảo liên kết hết hạn không thể truy cập.
5. **Mã hóa đầu-cuối:** Test 3.2 (Tạo share link) và 3.3 (Truy cập link chia sẻ) chứng minh luồng trao đổi khóa hoạt động đúng, server đóng vai trò trung gian chuyển phát khóa đã mã hóa mà không biết nội dung khóa.