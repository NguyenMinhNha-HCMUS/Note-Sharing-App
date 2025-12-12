# [cite_start]Báo Cáo phần Client & Common [cite: 1]
## ( NOTE SHARING APP ) [cite_start][cite: 2]

---

### [cite_start]I. Chi Tiết Cài Đặt và Các Thủ Thuật Tối Ưu Hóa (Optimize) [cite: 3]

#### [cite_start]1. Tối Ưu Hóa Giao Thức (Module common/Protocol) [cite: 4]
* [cite_start]**Thủ thuật:** Sử dụng định dạng nhị phân cố định (như struct trong `Protocol.h`) cho Header và Payload thay vì các định dạng dựa trên văn bản (JSON, XML)[cite: 5].
* **Lý do & Kết quả:**
    * [cite_start]Định dạng nhị phân loại bỏ các ký tự thừa (whitespace, dấu ngoặc nhọn), giúp giảm đáng kể kích thước gói tin (ước tính giảm **40% - 60%**)[cite: 6].
    * [cite_start]Điều này trực tiếp tăng tốc độ truyền tải và giảm yêu cầu xử lý phía Server/Client[cite: 7].

#### [cite_start]2. Tối Ưu Hóa Mạng (Module client/network) [cite: 8]
* [cite_start]**Thủ thuật:** Áp dụng **Tái sử dụng Connection (Connection Reuse)**[cite: 9]. [cite_start]Sau khi thiết lập kết nối TCP ban đầu (`connectToServer`), Client duy trì socket đó để gửi nhiều yêu cầu liên tiếp (`sendRawData`)[cite: 10].
* **Lý do & Kết quả:**
    * [cite_start]Loại bỏ chi phí thiết lập bắt tay ba bước (TCP Handshake) cho mỗi yêu cầu[cite: 11].
    * [cite_start]Giảm đáng kể độ trễ (latency) của các giao dịch, làm cho ứng dụng phản hồi nhanh hơn[cite: 12].

#### [cite_start]3. Tối Ưu Hóa Bảo Mật (Module common/Crypto) [cite: 13]
* [cite_start]**Thủ thuật:** Triển khai **Session Key Caching**[cite: 14]. [cite_start]Khóa phiên (Session Key) chỉ được trao đổi và thiết lập một lần khi đăng nhập[cite: 15]. [cite_start]Khóa này được tái sử dụng bởi `Crypto.cpp` để mã hóa/giải mã cho tất cả dữ liệu giao tiếp sau đó[cite: 16].
* [cite_start]**Lý do & Kết quả:** Tránh lặp lại thuật toán trao đổi khóa (ví dụ: RSA) tốn kém tài nguyên cho mỗi request API, tăng hiệu suất truyền dữ liệu mã hóa lên mức cao nhất có thể[cite: 17].

---

### II. [cite_start]Thách Thức và Giải Pháp [cite: 18]

#### [cite_start]1. Vấn đề Lỗi Biên dịch và Cấu trúc Mã nguồn [cite: 19]
* [cite_start]**Thách thức:** Xảy ra lỗi biên dịch (incomplete type, lỗi ép kiểu `void*`) khi trình biên dịch C++ (`g++`) cố gắng xử lý các thư viện C thuần (ví dụ: `sqlite3.c`)[cite: 20].
* [cite_start]**Giải pháp:** Áp dụng kỹ thuật phân tách trình biên dịch[cite: 21]. [cite_start]Buộc sử dụng `gcc` để biên dịch riêng các file C, sau đó chỉ sử dụng `g++` để liên kết file đối tượng (`.o`) đã tạo với các file C++ còn lại của ứng dụng[cite: 22].

#### [cite_start]2. Vấn đề Xung đột Hợp nhất (Merge Conflict) Phức tạp [cite: 23]
* [cite_start]**Thách thức:** Gặp xung đột `CONFLICT (modify/delete)` khi thực hiện `git pull`[cite: 24]. [cite_start]Xung đột này xảy ra khi branch remote đã xóa file, nhưng branch cục bộ lại đang có chỉnh sửa đối với file đó[cite: 25].
* **Giải pháp:**
    1. [cite_start]Thực hiện `git pull origin client`[cite: 27].
    2. [cite_start]Sử dụng `git add .` để đánh dấu xung đột đã được giải quyết, chấp nhận giữ lại phiên bản code đã chỉnh sửa cục bộ (HEAD)[cite: 28].
    3. [cite_start]Tạo `git commit` để hoàn tất quá trình hợp nhất[cite: 29].

#### [cite_start]3. Vấn đề Quản lý Phiên bản (Git Upstream) [cite: 30]
* [cite_start]**Thách thức:** Sau khi giải quyết xung đột, lệnh `git push` bị từ chối với lỗi `fatal: The current branch client has no upstream branch`[cite: 31].
* [cite_start]**Giải pháp:** Sử dụng lệnh đầy đủ `git push -u origin client`[cite: 32].
    * [cite_start]Lệnh này không chỉ đẩy code mà còn thiết lập branch cục bộ `client` để theo dõi branch remote `origin/client`[cite: 33].
    * [cite_start]Điều này cho phép sử dụng lệnh `git push` và `git pull` đơn giản hơn từ lần sau[cite: 34].

#### [cite_start]4. Vấn đề Truy cập Hệ thống khi Xóa File [cite: 35]
* [cite_start]**Thách thức:** Không thể xóa thư mục dự án cũ (ví dụ: Note-Sharing-App) do lỗi `Access to the path... is denied` hoặc `The process cannot access the file...`[cite: 36].
* [cite_start]**Giải pháp:** Tìm và **đóng tất cả các tiến trình** đang sử dụng bất kỳ file nào trong thư mục đó (bao gồm các terminal đang trỏ vào thư mục, cửa sổ VS Code đang mở project, hoặc tiến trình của file thực thi `client_app.exe` thông qua Task Manager)[cite: 37].

---

### III. [cite_start]Logic và Tương Tác Chi tiết Giữa Các File (Data Flow) [cite: 38]
[cite_start]Phần này mô tả vai trò logic của từng file và cách chúng điều phối dữ liệu với nhau để tạo ra luồng giao tiếp[cite: 39].

#### [cite_start]A. Vai trò Tương tác của từng File [cite: 40]
* [cite_start]**`client_app_logic.cpp`:** Đóng vai trò là **Bộ điều khiển (Controller)** chính[cite: 41]. [cite_start]Nó là nơi quyết định logic nghiệp vụ và điều phối toàn bộ luồng dữ liệu[cite: 42]. [cite_start]File này không tự thực hiện mã hóa hay gửi/nhận mạng, mà **gọi** các dịch vụ từ các file khác[cite: 43].
* [cite_start]**`network.cpp`:** Đảm nhận vai trò **Truyền tải** cấp thấp[cite: 44]. [cite_start]Nó nhận các gói tin nhị phân đã được chuẩn bị và chỉ tập trung vào việc gửi/nhận qua socket TCP, xử lý các tác vụ như `connectToServer` và `sendRawData`[cite: 45].
* [cite_start]**`Protocol.cpp`:** Cung cấp dịch vụ **Định dạng Dữ liệu**[cite: 46]. [cite_start]Nó nhận dữ liệu thô từ `client_app_logic.cpp` và chuyển thành gói tin nhị phân chuẩn (có Header) để mạng có thể truyền tải, hoặc ngược lại, mở gói dữ liệu nhận được[cite: 47].
* [cite_start]**`Crypto.cpp`:** Cung cấp dịch vụ **Bảo mật**[cite: 48]. [cite_start]Nó thực hiện các tác vụ mã hóa/giải mã Session Key, mã hóa/giải mã dữ liệu và băm mật khẩu theo yêu cầu của `client_app_logic.cpp`[cite: 49].

#### [cite_start]B. Luồng Dữ liệu Yêu cầu (Request Flow) [cite: 50]
[cite_start]Mọi yêu cầu từ phía Client (ví dụ: Đăng nhập, Tạo ghi chú) đều tuân theo Quy trình 5 bước sau[cite: 51]:
1. [cite_start]`client_app_logic.cpp` khởi tạo yêu cầu và dữ liệu thô (ví dụ: username/password)[cite: 52].
2. [cite_start]Dữ liệu được chuyển đến `Crypto.cpp` để **Mã hóa** (cho Payload) hoặc **Băm** (cho mật khẩu)[cite: 53].
3. [cite_start]Dữ liệu đã xử lý được chuyển đến `Protocol.cpp` để **Đóng gói** thành gói tin nhị phân hoàn chỉnh, bao gồm Header và Mã lệnh (Command ID)[cite: 54].
4. [cite_start]Gói nhị phân được chuyển đến `network.cpp` để **Gửi** qua socket TCP[cite: 55].
5. [cite_start]Dữ liệu được gửi đến Server[cite: 56].

#### [cite_start]C. Luồng Dữ liệu Phản hồi (Response Flow) [cite: 57]
[cite_start]Khi nhận được phản hồi từ Server, luồng dữ liệu được xử lý ngược lại[cite: 58]:
1. [cite_start]`network.cpp` nhận Buffer dữ liệu thô từ mạng[cite: 59].
2. [cite_start]Buffer thô được chuyển đến `Protocol.cpp` để **Mở gói**, tách Header và Payload ra[cite: 60].
3. [cite_start]Nếu Payload bị mã hóa, nó được chuyển đến `Crypto.cpp` để **Giải mã** bằng Session Key[cite: 61].
4. [cite_start]Dữ liệu cuối cùng được chuyển lại cho `client_app_logic.cpp` để xử lý logic nghiệp vụ (ví dụ: hiển thị lỗi, lưu trữ Session Key, cập nhật giao diện)[cite: 62].