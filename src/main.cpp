#include <SPI.h>
#include <mcp_can.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDConsumerControl.h"

// ============================================
// 硬體腳位定義 (ESP32-S3)
// ============================================
#define CAN0_CS  10  // CS pin

// ESP32-S3 SPI 腳位
#define SPI_SCK  12
#define SPI_MISO 13
#define SPI_MOSI 11

// MCP2515 設定
#define MCP2515_CLOCK MCP_8MHZ
#define CAN_SPEED CAN_500KBPS
#define SPI_SPEED 500000

// 初始化 CAN 物件
MCP_CAN CAN0(CAN0_CS);

// 初始化 HID 物件
USBHIDKeyboard Keyboard;
USBHIDConsumerControl ConsumerControl;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("=== ESP32-S3 CAN to USB Keyboard ===");

  // 啟動 USB HID
  Keyboard.begin();
  ConsumerControl.begin();
  USB.begin();

  // 設定 SPI 腳位和速度
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, CAN0_CS);
  SPI.setFrequency(SPI_SPEED);

  // 初始化 MCP2515
  if (CAN0.begin(MCP_ANY, CAN_SPEED, MCP2515_CLOCK) == CAN_OK) {
    Serial.println("MCP2515 Init OK!");
  } else {
    Serial.println("MCP2515 Init Failed!");
  }
  
  CAN0.setMode(MCP_NORMAL);
  Serial.println("Ready!");
}

unsigned char lastData[8] = {0};

void loop() {
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8] = {0};

  if (CAN0.checkReceive() == CAN_MSGAVAIL) {
    memset(rxBuf, 0, 8);
    CAN0.readMsgBuf(&rxId, &len, rxBuf);
    
    // 只處理 ID 0x475
    if (rxId == 0x475) {
      // 檢查資料是否有變化
      if (memcmp(rxBuf, lastData, 8) == 0) return;
      memcpy(lastData, rxBuf, 8);
      
      // 全 0 = 沒按任何鍵
      bool allZero = true;
      for (int i = 0; i < 8; i++) {
        if (rxBuf[i] != 0) { allZero = false; break; }
      }
      if (allZero) return;
      
      // ==========================================
      // Byte 0: 六大功能鍵 -> 數字鍵 1-6
      // ==========================================
      if (rxBuf[0] == 0x80) { Serial.println("左上 -> 1"); Keyboard.press('1'); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[0] == 0x40) { Serial.println("左中 -> 2"); Keyboard.press('2'); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[0] == 0x20) { Serial.println("左下 -> 3"); Keyboard.press('3'); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[0] == 0x10) { Serial.println("右上 -> 4"); Keyboard.press('4'); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[0] == 0x08) { Serial.println("右中 -> 5"); Keyboard.press('5'); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[0] == 0x04) { Serial.println("右下 -> 6"); Keyboard.press('6'); delay(50); Keyboard.releaseAll(); }
      
      // ==========================================
      // Byte 1: 旋鈕與方向
      // ==========================================
      if (rxBuf[1] == 0x40) { Serial.println("飛梭左旋 -> ,"); Keyboard.press(','); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[1] == 0x20) { Serial.println("飛梭右旋 -> ."); Keyboard.press('.'); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[1] == 0x02) { Serial.println("方向上"); Keyboard.press(KEY_UP_ARROW); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[1] == 0x04) { Serial.println("方向下"); Keyboard.press(KEY_DOWN_ARROW); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[1] == 0x08) { Serial.println("方向左"); Keyboard.press(KEY_LEFT_ARROW); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[1] == 0x10) { Serial.println("方向右"); Keyboard.press(KEY_RIGHT_ARROW); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[1] == 0x60) { Serial.println("Enter"); Keyboard.press(KEY_RETURN); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[1] == 0x01) { Serial.println("Radio -> R"); Keyboard.press('r'); delay(50); Keyboard.releaseAll(); }
      
      // ==========================================
      // Byte 2: 系統模式
      // ==========================================
      if (rxBuf[2] == 0x80) { Serial.println("NAVI -> N"); Keyboard.press('n'); delay(50); Keyboard.releaseAll(); }
      
      // ==========================================
      // Byte 3: 媒體控制
      // ==========================================
      if (rxBuf[3] == 0x20) { Serial.println("Play/Pause"); ConsumerControl.press(CONSUMER_CONTROL_PLAY_PAUSE); ConsumerControl.release(); }
      
      // ==========================================
      // Byte 6: 方向盤按鍵
      // ==========================================
      if (rxBuf[6] == 0x80) { Serial.println("Voice -> V"); Keyboard.press('v'); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[6] == 0x40) { Serial.println("接通 -> C"); Keyboard.press('c'); delay(50); Keyboard.releaseAll(); }
      if (rxBuf[6] == 0x20) { Serial.println("掛斷 -> D"); Keyboard.press('d'); delay(50); Keyboard.releaseAll(); }
    }
  }
}