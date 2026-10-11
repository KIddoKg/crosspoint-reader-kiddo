# Giải Thích Giao Diện (UI) của Máy Đọc Sách Xteink X4

## 📋 Tổng Quan

Máy đọc sách Xteink X4 sử dụng kiến trúc **Activity-based** - mỗi màn hình/giao diện là một "Activity" (hoạt động) riêng biệt. Khi người dùng tương tác, ứng dụng chuyển đổi giữa các Activity khác nhau.

## 🏗️ Kiến Trúc Chính

### 1. Main Loop (src/main.cpp)
- **Khởi động**: Khởi tạo GPIO, SD card, cài đặt, display và font.
- **Vòng lặp chính**: 
  - Đọc input từ các nút bấm/cảm biến.
  - Chạy activity hiện tại.
  - Kiểm tra điều kiện sleep để tiết kiệm pin.
  - Cập nhật display (E-ink).

### 2. Activity System (src/activities/)
Một **Activity** là một màn hình/giao diện. Mỗi activity có:
- `onEnter()`: Chuẩn bị khi vào
- `onExit()`: Dọn dẹp khi thoát
- `loop()`: Xử lý logic mỗi frame
- `render()`: Vẽ giao diện lên màn hình

## 📱 Các Giao Diện Chính (Activities)

### 1. **HomeActivity** (Trang Chủ)
📂 Vị trí: `src/activities/home/HomeActivity.h`

**Chức năng:**
- Hiển thị sách gần đây (recent books) với cover
- Menu chính: Thư viện, Sách gần đây, Cài đặt, Chuyển file, Duyệt OPDS
- Cho phép chọn sách để đọc

**Cấu trúc:**
```
┌─────────────────────────────────┐
│   CrossPoint Reader             │
├─────────────────────────────────┤
│  Sách Gần Đây:                  │
│  ┌────────┐  ┌────────┐         │
│  │ Cover  │  │ Cover  │         │
│  │ Book1  │  │ Book2  │         │
│  └────────┘  └────────┘         │
├─────────────────────────────────┤
│  Menu:                          │
│  > Thư Viện (My Library)        │
│    Sách Gần Đây (Recent)        │
│    Cài Đặt (Settings)           │
│    Chuyển File (File Transfer)  │
│    Duyệt Sách (OPDS Browser)    │
└─────────────────────────────────┘
```




**Input handling:**
- Các nút mũi tên: Di chuyển trong menu
- Nút OK/Enter: Chọn item
- Nút back: Quay lại

### 2. **ReaderActivity** (Đọc Sách)
📂 Vị trí: `src/activities/reader/ReaderActivity.h`

**Chức năng:**
- Đọc file EPUB, XTC, TXT
- Hiển thị nội dung sách theo trang
- Cho phép chuyển trang, chỉnh font, v.v.

**Cấu trúc:**
```
┌─────────────────────────────────┐
│  Chapter 1: The Beginning       │ (Title)
├─────────────────────────────────┤
│  This is the first paragraph... │
│  Lorem ipsum dolor sit amet...  │
│  Consectetur adipiscing elit... │
│                                 │
│  [Nội dung sách được hiển thị] │
│                                 │
│  Page 42 / 234                  │ (Footer)
└─────────────────────────────────┘
```

**Input handling:**
- Nút phải: Trang tiếp theo
- Nút trái: Trang trước
- Nút menu: Hiển thị menu đọc (toc, settings, v.v.)

### 3. **MyLibraryActivity** (Thư Viện)
📂 Vị trí: `src/activities/home/MyLibraryActivity.h`

**Chức năng:**
- Duyệt các file EPUB/XTC/TXT trên thẻ SD
- Hiển thị danh sách sách với cover thumbnail
- Cho phép chọn sách để đọc

**Cấu trúc:**
```
┌─────────────────────────────────┐
│  My Library: /Books/            │
├─────────────────────────────────┤
│  [Folder] Fiction               │
│  [Folder] Tech Books            │
│  ┌───────────┐ The Great Gatsby │
│  │  Cover    │                  │
│  └───────────┘ by F.S.          │
│  ┌───────────┐ Python Mastery   │
│  │  Cover    │                  │
│  └───────────┘ by Expert        │
└─────────────────────────────────┘
```

### 4. **RecentBooksActivity** (Sách Gần Đây)
📂 Vị trí: `src/activities/home/RecentBooksActivity.h`

**Chức năng:**
- Hiển thị danh sách các sách đã đọc gần đây
- Lưu tiến độ đọc mỗi cuốn sách
- Cho phép tiếp tục đọc từ vị trí cũ

### 5. **SettingsActivity** (Cài Đặt)
📂 Vị trí: `src/activities/settings/SettingsActivity.h`

**Chức năng:**
- Điều chỉnh cài đặt: font size, kiểu font, độ sáng, v.v.
- Cài đặt WiFi
- Quản lý Bluetooth, storage, v.v.

**Cấu trúc:**
```
┌─────────────────────────────────┐
│  Settings                       │
├─────────────────────────────────┤
│  > Display Settings             │
│    Font Settings                │
│    WiFi & Network               │
│    Device Settings              │
│    About                        │
└─────────────────────────────────┘
```

### 6. **CrossPointWebServerActivity** (Chuyển File)
📂 Vị trí: `src/activities/network/CrossPointWebServerActivity.h`

**Chức năng:**
- Bật web server để chuyển file từ máy tính
- Cho phép truy cập qua WiFi
- Upload EPUB, TXT, v.v.

### 7. **OpdsBookBrowserActivity** (Duyệt OPDS)
📂 Vị trí: `src/activities/browser/OpdsBookBrowserActivity.h`

**Chức năng:**
- Kết nối với server OPDS (Online Bookshop)
- Tìm kiếm và tải sách từ mạng
- Hỗ trợ các dịch vụ sách online như Project Gutenberg, v.v.

### 8. **BootActivity & SleepActivity** (Boot & Sleep)
📂 Vị trí: `src/activities/boot_sleep/`

**Chức năng:**
- **BootActivity**: Hiển thị splash screen khi khởi động
- **SleepActivity**: Hiển thị màn hình sleep, theo dõi nút wake

## 🎨 Rendering Pipeline (Quy Trình Vẽ)

```
Activity render() → GfxRenderer → E-ink Display
```

1. **Activity** tạo yêu cầu vẽ (drawing commands)
2. **GfxRenderer** xử lý các lệnh:
   - Vẽ hình chữ nhật, đường kẻ
   - Hiển thị text với các font khác nhau
   - Vẽ bitmap (hình ảnh)
3. **E-ink Display** cập nhật màn hình vật lý

**Fonts hỗ trợ:**
- **Bookerly** (12, 14, 16, 18pt): Cho đọc sách
- **NotoSans** (12, 14, 16, 18pt): Giao diện chữ không serif
- **OpenDyslexic** (8-14pt): Font cho người khó đọc
- **UI fonts** (10, 12pt): Giao diện hệ thống

## 💾 State Management (Quản Lý Trạng Thái)

### CrossPointState (src/CrossPointState.h)
Lưu trạng thái toàn cục:
- Sách hiện tại đang đọc
- Trang hiện tại
- Cài đặt display
- Lịch sử đọc
- WiFi credentials

### CrossPointSettings (src/CrossPointSettings.h)
Lưu các cài đặt người dùng:
- Font size, kiểu font
- Độ sáng, contrast
- Auto-sleep time
- Ngôn ngữ giao diện (I18n)

## 🎮 Input Management (Quản Lý Input)

**MappedInputManager** (src/MappedInputManager.h)
- Đọc các nút bấm từ GPIO
- Ánh xạ với actions (up, down, left, right, ok, back, menu)
- Gửi tới Activity hiện tại

**Button Layout (Xteink X4):**
```
    [Up]
[Lt][OK][Rt]
   [Down]
  [Power/Menu]
```

## 🔄 Activity Flow (Luồng Chuyển Đổi)

```
Boot
  ↓
BootActivity (splash screen)
  ↓
Load Settings & State
  ↓
HomeActivity (trang chủ)
  ↓
┌─── Chọn sách ──→ ReaderActivity
│                      ↓
│                  Đọc sách
│                      ↓
│              ← back → HomeActivity
│
├─── Thư viện ──→ MyLibraryActivity
│                      ↓
│                  Chọn sách
│                      ↓
│              ← back → HomeActivity
│
├─── Settings ──→ SettingsActivity
│                      ↓
│                  Điều chỉnh cài đặt
│                      ↓
│              ← back → HomeActivity
│
├─── File Transfer ──→ WebServerActivity
│                      ↓
│                  Upload file via WiFi
│                      ↓
│              ← back → HomeActivity
└
Sleep Mode (khi không dùng)
  ↓
Deep Sleep (tiết kiệm pin)
```

## 📊 Content Processing Pipeline (Xử Lý Nội Dung)

### EPUB Files
```
EPUB file (.epub)
    ↓
Unzip & parse OPF/TOC
    ↓
Parse CSS & HTML
    ↓
Calculate layout (pagination)
    ↓
Cache pages to SD (.crosspoint folder)
    ↓
Display current page via GfxRenderer
```

### XTC Files (Compressed format)
```
XTC file
    ↓
Decompress content
    ↓
Parse structure
    ↓
Display
```

### TXT Files
```
TXT file
    ↓
Read text
    ↓
Calculate layout
    ↓
Display
```

## 🌐 Multi-Language Support (I18n)

📂 Vị trí: `lib/I18n/`

- Hỗ trợ nhiều ngôn ngữ thông qua file translation
- Chuỗi UI được dịch động
- Cài đặt ngôn ngữ được lưu trong settings

## 📱 Key UI Components

### UITheme (src/components/UITheme.h)
- Định nghĩa màu sắc, kích thước, kiểu dáng chung
- Đảm bảo nhất quán giữa các Activity

### Icons (src/components/icons/)
- Các icon SVG cho menu, nút, v.v.
- Được render bằng bitmap

## ⚡ Performance Considerations

1. **E-ink refresh optimization:**
   - Chỉ refresh toàn màn hình khi cần
   - Sử dụng partial refresh khi có thể

2. **Memory optimization:**
   - SD card caching cho content
   - Single buffer mode để tiết kiệm RAM

3. **Power optimization:**
   - Auto deep sleep sau thời gian không dùng
   - GPIO power management qua HalPowerManager

## 🔧 Customization

Để thêm giao diện mới:
1. Tạo class mới kế thừa từ `Activity`
2. Implement `onEnter()`, `onExit()`, `loop()`, `render()`
3. Thêm callback vào main.cpp
4. Tích hợp vào Activity flow

Để thay đổi layout UI:
1. Sửa `render()` method của Activity
2. Dùng `GfxRenderer` API để vẽ
3. Build & upload code

---

**Tóm lại:** Giao diện Xteink X4 hoạt động dựa trên các Activity riêng biệt, mỗi cái quản lý một màn hình. Input từ nút bấm được xử lý, Activity render giao diện, và kết quả hiển thị trên E-ink display. Trạng thái được lưu và load tự động, cho phép tiếp tục từ vị trí cũ.
