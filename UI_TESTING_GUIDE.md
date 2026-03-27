# 🧪 Hướng Dẫn Test Giao Diện (UI Testing)

## 📋 Yêu Cầu
- Board ESP32-C3-DevKitM-1 + Xteink X4 E-ink display + buttons
- PlatformIO IDE cài đặt trên VS Code
- Code đã được compile & upload thành công
- Serial monitor để xem log

## 🎯 Test Plan

### 1️⃣ Test Boot & Initialization

**Mục đích:** Kiểm tra quá trình khởi động

**Bước:**
1. Cấp nguồn cho board (USB hoặc pin)
2. Quan sát:
   - ✅ Splash screen xuất hiện (BootActivity)
   - ✅ Sau ~2-3 giây, chuyển sang HomeActivity
   - ✅ Serial log hiển thị: "Display initialized", "Fonts setup"

**Log kỳ vọng:**
```
[MAIN] Starting CrossPoint version 1.1.0-dev
[MAIN] Display initialized
[MAIN] Fonts setup
[MAIN] Entering Home Activity
```

---

### 2️⃣ Test HomeActivity (Trang Chủ)

**Mục đích:** Kiểm tra trang chủ và menu chính

**Bước:**
1. Lên màn hình chủ
2. Nhấn **Nút Up/Down** → Menu items thay đổi (highlight)
3. Nhấn **Nút OK** → Vào activity tương ứng
4. Nhấn **Nút Back** → Quay lại HomeActivity

**Test items:**
- [ ] "My Library" (Thư viện)
- [ ] "Recent Books" (Sách gần đây)
- [ ] "Settings" (Cài đặt)
- [ ] "File Transfer" (Chuyển file)
- [ ] "OPDS Browser" (Duyệt OPDS)

**Kỳ vọng:**
- Menu items được highlight đúng
- Có cover books hiển thị (nếu có sách gần đây)
- Navigation mượt mà

---

### 3️⃣ Test MyLibraryActivity (Thư Viện)

**Mục đích:** Kiểm tra duyệt thư viện

**Chuẩn bị:**
- Copy một vài file EPUB vào SD card (`/Books/` hoặc root)
- Có cover image cho sách

**Bước:**
1. Từ Home → Chọn "My Library"
2. Quan sát:
   - ✅ Danh sách file/folder hiển thị
   - ✅ Cover books (nếu có) được load
   - ✅ Nút Up/Down di chuyển cursor qua items
3. Chọn sách → Vào ReaderActivity
4. Nút Back → Quay lại MyLibraryActivity

**Test edge cases:**
- [ ] Thư mục rỗng
- [ ] File không phải EPUB/TXT (bỏ qua)
- [ ] Folder lồng nhau (Navigate vào folder)
- [ ] File có tên Unicode/Vietnamese

---

### 4️⃣ Test ReaderActivity (Đọc Sách)

**Mục đích:** Kiểm tra tính năng đọc sách

**Chuẩn bị:**
- EPUB file hợp lệ
- TXT file

**Bước:**
1. Chọn sách từ Library → ReaderActivity
2. Quan sát:
   - ✅ Trang đầu tiên được render
   - ✅ Text hiển thị đúng font & size
   - ✅ Layout hợp lý (page break, margins)
3. Nhấn **Nút Right** → Trang tiếp theo
4. Nhấn **Nút Left** → Trang trước
5. Nhấn **Nút Menu** → Hiển thị menu (TOC, settings, v.v.)

**Test features:**
- [ ] Chuyển trang mượt mà
- [ ] Page counter đúng
- [ ] Font render đúng
- [ ] Hyphenation (nếu có) hoạt động

**Serial log:**
- Xem dòng log về page loading, rendering time
- Kiểm tra memory usage (log cứ 10 giây một lần)

---

### 5️⃣ Test RecentBooksActivity (Sách Gần Đây)

**Mục đích:** Kiểm tra lịch sử đọc

**Bước:**
1. Đọc vài cuốn sách (hoặc load từ RecentBooksStore)
2. Từ Home → "Recent Books"
3. Quan sát:
   - ✅ Danh sách sách đã đọc
   - ✅ Cover thumbnails
   - ✅ Nhấn OK để tiếp tục đọc từ vị trí cũ

**Test:**
- [ ] Tiếp tục từ vị trí cũ (page number phải đúng)
- [ ] Remove/Clear history (nếu có)

---

### 6️⃣ Test SettingsActivity (Cài Đặt)

**Mục đích:** Kiểm tra menu cài đặt

**Bước:**
1. Từ Home → "Settings"
2. Duyệt các menu:
   - Display Settings
   - Font Settings
   - WiFi & Network
   - Device Settings
   - About
3. Chỉnh một vài cài đặt:
   - [ ] Font size (tăng/giảm)
   - [ ] Font family (thay đổi)
   - [ ] Display brightness (nếu có)
4. Lưu cài đặt → Quay lại
5. Kiểm tra thay đổi áp dụng (ví dụ: font size)

**Kỳ vọng:**
- Cài đặt được lưu vào file JSON
- Khi restart, cài đặt được load lại
- UI update theo cài đặt mới

---

### 7️⃣ Test CrossPointWebServerActivity (Chuyển File)

**Mục đích:** Kiểm tra web server & file transfer

**Chuẩn bị:**
- Kết nối board tới WiFi
- Máy tính cùng WiFi

**Bước:**
1. Từ Home → "File Transfer"
2. Board hiển thị IP address (ví dụ: 192.168.1.100:8080)
3. Trên máy tính, mở browser → `http://192.168.1.100:8080`
4. Thấy giao diện upload file
5. Chọn file EPUB/TXT → Upload
6. Kiểm tra file được lưu trên SD card

**Test:**
- [ ] Web server bật/tắt đúng
- [ ] Upload thành công
- [ ] File được lưu đúng vị trí

---

### 8️⃣ Test OpdsBookBrowserActivity (Duyệt OPDS)

**Mục đích:** Kiểm tra tính năng OPDS

**Chuẩn bị:**
- Cấu hình OPDS server URL (nếu có)
- Kết nối WiFi

**Bước:**
1. Từ Home → "OPDS Browser"
2. Board kết nối server OPDS
3. Duyệt danh sách sách
4. Download sách nếu có

**Note:** Feature này tuỳ vào cấu hình OPDS server

---

### 9️⃣ Test Input & Navigation

**Mục đích:** Kiểm tra tất cả các nút bấm

**Button Layout:**
```
        [UP]
    [LT][OK][RT]
       [DOWN]
    [POWER/MENU]
```

**Test cases:**
- [ ] Up: Di chuyển lên
- [ ] Down: Di chuyển xuống
- [ ] Left: Trang trước / Back
- [ ] Right: Trang tiếp theo
- [ ] OK: Confirm / Select
- [ ] Power: Vào sleep
- [ ] Power + Down: Screenshot (nếu implement)

**Serial log command:**
```
// Gửi lệnh này qua Serial monitor:
CMD:SCREENSHOT
```

---

### 🔟 Test Sleep & Power

**Mục đích:** Kiểm tra power management

**Bước:**
1. Chạy ứng dụng bình thường
2. Không nhấn nút trong ~30 giây (hoặc cấu hình timeout)
3. Board sẽ:
   - ✅ Vào SleepActivity
   - ✅ Display vào deep sleep
   - ✅ CPU frequency giảm
   - ✅ Power consumption tối thiểu
4. Nhấn Power button → Thức dậy
5. Quay lại activity trước đó (nếu đang đọc sách)

**Log kỳ vọng:**
```
[SLP] Auto-sleep triggered after 30000 ms of inactivity
[MAIN] Entering deep sleep
```

---

### 1️⃣1️⃣ Test State Persistence (Lưu Trạng Thái)

**Mục đích:** Kiểm tra lưu/load trạng thái

**Bước:**
1. Đọc sách đến trang 42
2. Vào Settings, thay đổi font size
3. Power off (nhấn Power button → Sleep)
4. Power on (nhấn Power button → Thức dậy)
5. Kiểm tra:
   - ✅ Vẫn đang đọc sách cùng trang
   - ✅ Font size vẫn là cài đặt mới
   - ✅ Sách vẫn ở trong RecentBooks

**File kiểm tra:**
- `/.crosspoint/state.json` - Trạng thái ứng dụng
- `/.crosspoint/settings.json` - Cài đặt
- `/.crosspoint/recentBooks.json` - Danh sách sách gần đây

---

## 📊 Performance Testing

### Memory Usage
Xem log Serial (cứ 10 giây):
```
[MEM] Free: 45234 bytes, Total: 327680 bytes, Min Free: 23456 bytes
```

**Kỳ vọng:**
- Free heap > 20KB (để tránh crash)
- Không tăng liên tục (không memory leak)

### Rendering Time
Xem log:
```
[LOOP] New max loop duration: 150 ms (activity: 120 ms)
```

**Kỳ vọng:**
- Loop duration < 200ms
- Responsive input (< 50ms delay)

### Page Loading Time
Khi chuyển trang:
- Nhấn nút → Page render → Display update
- Thời gian < 500ms (cho EPUB lớn)

---

## 🐛 Debug Tips

### 1. Serial Monitor
```
Cải setting monitor_speed = 115200 trong platformio.ini
Xem log chi tiết từ board
```

### 2. Screenshot
```
Nhấn Power + Down để screenshot
Dữ liệu được gửi qua Serial
```

### 3. Enable Debug Logging
Sửa `platformio.ini`:
```ini
[env:default]
build_flags =
  ${base.build_flags}
  -DLOG_LEVEL=3  ; 0=Error, 1=Info, 2=Debug, 3=Trace
```

### 4. Check File System
Các file dự liệu:
- `/` - Root thẻ SD
- `/.crosspoint/` - Cache & settings
- `/Books/` - Thư viện sách

Sử dụng web server file browser để kiểm tra files

---

## ✅ Checklist Hoàn Thành

- [ ] Boot & initialization thành công
- [ ] HomeActivity hiển thị đúng
- [ ] MyLibraryActivity duyệt file ok
- [ ] ReaderActivity đọc sách ok
- [ ] Settings được lưu/load đúng
- [ ] Sleep/wake hoạt động
- [ ] Navigation mượt mà
- [ ] Memory stable (không leak)
- [ ] Performance tốt (loop time ok)
- [ ] File transfer ok (nếu test)
- [ ] OPDS browser ok (nếu test)

---

## 🚀 Kết Luận

Sau khi pass all tests này, giao diện của máy đọc sách Xteink X4 là hoạt động bình thường! 🎉

Nếu gặp lỗi, kiểm tra:
1. **Serial log** - Xem chi tiết lỗi
2. **Memory usage** - Có bị leak không
3. **File system** - Cấu trúc thư mục đúng không
4. **Hardware** - Board, display, buttons hoạt động không

Chúc testing vui vẻ! 📱✨
