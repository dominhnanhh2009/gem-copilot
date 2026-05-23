# Specification: Gemini Binary Wrapper (Stateless)

## 1. Mục tiêu
Tạo giao diện C++ cho phép gửi prompt tới Gemini API theo tư tưởng "Request/Response" thuần túy. Mỗi lần gọi là một yêu cầu độc lập (stateless), loại bỏ mọi sự phức tạp của quản lý hội thoại ở tầng API.

## 2. Tư tưởng thiết kế
*   **Stateless by design:** Mỗi request tự mang đầy đủ dữ liệu (text/files/images).
*   **Multi-modal Support:** Hỗ trợ truyền tải đa phương thức (multi-part) trong cùng một request.
*   **Binary-first:** API đơn giản, che giấu sự phức tạp của JSON/MIME.

## 3. Giao diện (Interface)
Định nghĩa giao diện chi tiết được quản lý tại tệp header: `include/gemini_engine.h`.

Giao diện tập trung vào việc:
*   Chấp nhận đầu vào dạng danh sách các phần nội dung (multi-part).
*   Thực thi việc gọi API dưới dạng hàm đồng bộ (hoặc bất đồng bộ tùy chọn).
*   Trả về kết quả trực tiếp hoặc mã lỗi.

### Lưu ý về Đường dẫn File
*   Dự án sử dụng `std::wstring` cho tất cả các đường dẫn file để đảm bảo hỗ trợ Unicode (hỗ trợ tiếng Việt có dấu, ký tự đặc biệt).
*   Khi cung cấp đường dẫn trong mã nguồn, hãy luôn sử dụng tiền tố `L` (ví dụ: `L"C:\\path\\to\\file.jpg"`).

