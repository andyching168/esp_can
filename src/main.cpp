#include <SPI.h>
#include <mcp_can.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDConsumerControl.h"

/* * 硬體腳位定義 (ESP32-S3)
 * 請根據您的實際接線修改
 */
#define CAN0_INT 9   // INT pin (雖然本範例用 polling，但定義著備用)
#define CAN0_CS  10  // CS pin

// 初始化 CAN 物件
MCP_CAN CAN0(CAN0_CS);

// 初始化 HID 物件
USBHIDKeyboard Keyboard;
USBHIDConsumerControl ConsumerControl;

// 前向宣告，避免在 loop 中呼叫時找不到定義
void handleCanMessage(unsigned char *data);

void setup() {
  // 開啟 Serial 方便除錯 (透過 USB CDC)
  Serial.begin(115200);
  
  // 啟動 USB HID
  Keyboard.begin();
  ConsumerControl.begin();
  USB.begin();

  // 初始化 CAN Bus
  // 注意: MCP_ANY 表示接收所有 ID，MCP_8MHZ 是震盪器頻率 (請確認您的模組是 8MHz 還是 16MHz)
  // 500KBPS 是您之前提到的速率
  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("Error Initializing MCP2515...");
    while(1); // 初始化失敗就停在這裡
  }
  
  // 設定成正常模式
  CAN0.setMode(MCP_NORMAL);
}

// 用來記錄上一次的按鍵狀態，避免重複觸發(如果需要的話)
// 但針對這種飛梭旋鈕，通常收到一次訊號就送一次按鍵最單純
void loop() {
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];

  // 檢查是否有收到 CAN 訊息
  if(CAN0.checkReceive() == CAN_MSGAVAIL) {
    
    CAN0.readMsgBuf(&rxId, &len, rxBuf);

    // 只處理 ID 0x475
    if(rxId == 0x475) {
      handleCanMessage(rxBuf);
    }
  }
}

void handleCanMessage(unsigned char *data) {
  // Debug: 印出收到的數據
  Serial.print("Data: ");
  for(int i=0; i<8; i++) {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();

  // ==========================================
  // Byte 1: 旋鈕與方向 (對應 Android 常用的導航鍵)
  // ==========================================
  
  // 飛梭左旋 (模擬 Volume Down 或 Left)
  if (data[1] == 0x40) {
    ConsumerControl.press(HID_KEY_COMMA);
    ConsumerControl.release();
  }
  // 飛梭右旋 (模擬 Volume Up 或 Right)
  else if (data[1] == 0x20) {
    ConsumerControl.press(HID_KEY_PERIOD);
    ConsumerControl.release();
  }
  // 方向上
  else if (data[1] == 0x02) {
    Keyboard.press(KEY_UP_ARROW);
    delay(50); Keyboard.releaseAll();
  }
  // 方向下
  else if (data[1] == 0x04) {
    Keyboard.press(KEY_DOWN_ARROW);
    delay(50); Keyboard.releaseAll();
  }
  // 方向左
  else if (data[1] == 0x08) {
    Keyboard.press(KEY_LEFT_ARROW);
    delay(50); Keyboard.releaseAll();
  }
  // 方向右
  else if (data[1] == 0x10) {
    Keyboard.press(KEY_RIGHT_ARROW);
    delay(50); Keyboard.releaseAll();
  }
  // Enter 鍵 (0x60)
  else if (data[1] == 0x60) {
    Keyboard.press(KEY_RETURN);
    delay(50); Keyboard.releaseAll();
  }
  // Radio 鍵 (對應 Home 或 音樂)
  else if (data[1] == 0x01) {
    ConsumerControl.press(HID_KEY_R); // 回首頁
    ConsumerControl.release();
  }

  // ==========================================
  // Byte 3: 媒體控制
  // ==========================================
  if (data[3] == 0x20) { // Play/Pause
    ConsumerControl.press(CONSUMER_CONTROL_PLAY_PAUSE);
    ConsumerControl.release();
  }

  // ==========================================
  // Byte 6: 方向盤按鍵
  // ==========================================
  if (data[6] == 0x80) { // Voice
    // Android 通常用 Search 鍵或特定的 Consumer Key 喚醒語音助理
    ConsumerControl.press(HID_KEY_V); 
    ConsumerControl.release();
  }
  else if (data[6] == 0x40) { // 接通
    // 這個鍵在電腦鍵盤沒有標準對應，通常我們會映射到 F1 或特殊功能鍵
    // 讓 Android 車機軟體去抓
    Keyboard.press(HID_KEY_C); 
    delay(50); Keyboard.releaseAll();
  }
  else if (data[6] == 0x20) { // 掛斷
    Keyboard.press(HID_KEY_D);
    delay(50); Keyboard.releaseAll();
  }

  // ==========================================
  // Byte 0: 六大功能鍵 (自定義映射到 F3~F8)
  // ==========================================
  // 為了讓 Button Mapper 容易抓取，我們將這些特殊功能鍵映射到 F 鍵
  if (data[0] == 0x80) { Keyboard.press(HID_KEY_1); delay(50); Keyboard.releaseAll(); } // 左上
  if (data[0] == 0x40) { Keyboard.press(HID_KEY_2); delay(50); Keyboard.releaseAll(); } // 左中
  if (data[0] == 0x20) { Keyboard.press(HID_KEY_3); delay(50); Keyboard.releaseAll(); } // 左下
  if (data[0] == 0x10) { Keyboard.press(HID_KEY_4); delay(50); Keyboard.releaseAll(); } // 右上
  if (data[0] == 0x08) { Keyboard.press(HID_KEY_5); delay(50); Keyboard.releaseAll(); } // 右中
  if (data[0] == 0x04) { Keyboard.press(HID_KEY_6); delay(50); Keyboard.releaseAll(); } // 右下

  // ==========================================
  // Byte 2: 系統模式 (Mode)
  // ==========================================
  if (data[2] == 0x80) { // NAVI -> 開啟地圖?
    ConsumerControl.press(HID_KEY_N); // 範例：對應下一首
    ConsumerControl.release();
  }
}