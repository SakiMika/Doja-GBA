# Corpse Party — New Chapter 1 / GBA Butano port

Bản port C++ native dành cho Game Boy Advance, sử dụng Butano. 

## Chơi

Mở `corpse_party_chapter1.gba` bằng mGBA hoặc thiết bị chạy homebrew GBA.

- D-pad: di chuyển / chọn câu trả lời.
- A: tương tác, xác nhận, hiện nhanh / sang trang hội thoại.
- B: hủy/quay lại hoặc đóng menu; chọn Không ở câu hỏi hai lựa chọn. B không đẩy tiếp hội thoại.
- Start: menu Resume / Save / Load / Items / Title screen.
- Select: bật/tắt nhạc.
- Màn hình đầu: dùng D-pad chọn **New Game** hoặc **Load**, nhấn A/Start xác nhận. Load mở danh sách ba slot; B quay lại.

Câu hỏi “View the opening?” là lựa chọn gốc. Chọn No sẽ chuyển vào lớp học giới thiệu nhân vật. Nhân vật con mắt tại đoạn này là con trỏ/actor trong dữ liệu gốc, không phải sprite bị thiếu.

Save dùng SRAM 32 KB với **3 slot độc lập**, mỗi slot có phiên bản và checksum riêng. Chọn Start → Save → Slot 1/2/3; tải bằng Load ở màn hình đầu hoặc menu trong game. Slot đã có dữ liệu sẽ hỏi xác nhận ghi đè (mặc định No); B hủy. Slot trống/hỏng không thể tải. New Game không xóa save. Save trong menu có thể lưu cả hội thoại/script đang chạy; điểm lưu trong game cũng mở danh sách ba slot.

Save từ bản một slot cũ được đọc trực tiếp ở **Slot 1**. Giữ file `.sav` cùng ROM và cùng tên gốc khi đổi phiên bản (ví dụ đổi cả tên ROM và `.sav` cho khớp). Ba slot đều nằm trong một file `.sav`.

## Phạm vi đã chuyển

- 30 bản đồ, 682 nhóm sự kiện, 1.229 event page.
- 3.947 lệnh script; đủ cả 17 opcode xuất hiện trong chương này.
- 1.166 lệnh hội thoại tiếng Anh lấy từ bản vá trong JAR.
- Điều kiện công tắc / vật phẩm, lựa chọn và nhánh, chuyển phòng, actor/party, chuyển động theo script, NPC tự di chuyển/đuổi theo, va chạm và tương tác.
- 70 ảnh, gồm tileset, sprite, portrait, title và tài nguyên bổ sung trong bản vá.
- 12 track MIDI gốc được render thành WAV và phát qua Butano/Maxmod.

## Khác biệt và giới hạn kiểm thử

Khung nhìn được chuyển từ 240×240 xuống 240×160. Sprite/bản đồ giữ kích thước pixel; camera và hộp thoại được bố trí lại. Portrait và title được thu nhỏ. Màu được gom vào palette GBA chung. Nhạc dùng bộ tổng hợp General MIDI của Java, không mô phỏng chính xác chip âm thanh điện thoại.

Bộ vẽ dùng nền 8 bpp và hai framebuffer. Nhắm 30 tick/giây; cảnh nhiều thao tác vẽ có thể chậm hơn. Chuyển động party và thời gian hiệu ứng là bản thích nghi, không cam kết trùng từng frame với DoJa.

Đã kiểm tra ROM bằng mGBA: khởi động, lựa chọn mở đầu, chuyển đến lớp học, hội thoại có portrait, di chuyển/tương tác, PCM âm thanh, lưu SRAM/checksum và tải đúng vị trí. Đã kiểm tra 300 frame sau tải save không bị mất nền. Host test chạy 2.458 lượt event page (hai lựa chọn mỗi page), kiểm tra 30 map và khôi phục trạng thái hội thoại. Đây là kiểm tra kỹ thuật; chưa chơi thủ công toàn bộ chương hoặc xác nhận từng ending trên máy GBA thật. Xem `validation.json`.

## Build

Cần devkitARM/devkitPro, Butano và Python trong PATH. Các assets C++ và WAV đã được tạo sẵn; build bình thường không cần file ZIP gốc hoặc Java.

Windows PowerShell:

```powershell
./build.ps1 -ButanoPath 'D:/abc/gbc/butano/butano' -DevkitProPath 'D:/devkitPro'
```

Hoặc đặt `DEVKITPRO`, `DEVKITARM`, rồi:

```text
make LIBBUTANO=/path/to/butano/butano -j8
```

Hướng dẫn engine: https://gvaliente.github.io/butano/getting_started_dka.html

Sau khi đổi audio backend hoặc phiên bản toolchain, chạy `make clean` rồi build lại.


