#include <M5Atom.h>

#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

extern const unsigned char image_NO0[77];
extern const unsigned char image_NO1[77];
extern const unsigned char image_NO2[77];
extern const unsigned char image_NO3[77];
extern const unsigned char image_NO4[77];
extern const unsigned char image_NO5[77];
extern const unsigned char image_NO6[77];
extern const unsigned char image_NO7[77];
extern const unsigned char image_NO8[77];
extern const unsigned char image_NO9[77];
extern const unsigned char image_SYNC[77];

extern const unsigned char image_ZEROZERO[347];

extern const unsigned char image_SCONF[77];


const unsigned char* IMAGES[10] = { image_NO0, image_NO1, image_NO2, image_NO3, image_NO4, image_NO5, image_NO6, image_NO7, image_NO8, image_NO9 };

int lastUpdateHour = 0;

unsigned char display_buffer[347];

const int LINE_PX = 23;
const int PX_SIZE = 3;
const int ROW_OFFSET = LINE_PX * PX_SIZE;

//JST:9Hour = 32400sec
const int UTC_OFFSET  = 32400;

//***********************************************************************
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, UTC_OFFSET);

/**
 * LEDマトリクスへ出力する内容を編集する。
 * 00:00形式である想定。
 */
void setBuffer(const unsigned char* source, unsigned char* dest, int pos) {
  //1Px = 3BYTE
  //数字は4Pxで記載。
  //左1pxが空白なので、5pxごと処理する
  int outputRowPos = (5 * pos) * PX_SIZE;
  if (pos > 1) {
    //「:」は2ピクセル使用
    outputRowPos += 2 * PX_SIZE;
  }
  //先頭２バイトはW*H
  outputRowPos += 2;

  //Serial.println(outputRowPos);

  for (int i = 0; i < 5; ++i) {
    //5行。
    int rowPos = outputRowPos + (i * ROW_OFFSET);
    for (int j = 0; j < 5; ++j) {
      //5列。
      //dest[rowPos + j] = source[2 + (((i*5) + j) * PX_SIZE)];
      for (int k = 0; k < PX_SIZE; ++k) {
        //k:RGBの1byte
        int sourcePos = (i * 5 * PX_SIZE) + (j * PX_SIZE) + k + 2;  //2:W*H
        //Serial.print("Source Pos:");
        //Serial.println(sourcePos);
        int destPos = rowPos + (j * PX_SIZE) + k;
        //Serial.print("Dest Pos:");
        //Serial.println(destPos);
        dest[destPos] = source[sourcePos];
      }
    }
  }
}

/**
 * NTPサーバに問い合わせを行い、時刻を更新する。
 * 本関数内でWi-Fiへの接続及び切断を実施するので注意。
 */
void ntp_refresh() {
  //SYNCアイコンにする
  M5.dis.displaybuff((uint8_t*)image_SYNC);

  WiFi.begin();

  Serial.println("Connecting WiFi..");
  while (true) {
    int status = WiFi.status();
    Serial.printf("status:%d\n", status);
    if (status != WL_CONNECTED) {
      delay(1000);
      continue;
    }
    timeClient.begin();
    timeClient.update();
    timeClient.end();

    WiFi.disconnect();

    Serial.printf("NTP OK. time:%s\n", timeClient.getFormattedTime());
    break;
  }
}

/**
 * SmartConfigを試みる。
 * SmartConfig成功後に切断するので、継続してWiFiを使用する場合は
 * 呼び出し元で接続すること。
 */
void trySmartConfig(){

  if(!M5.Btn.isPressed()){
    return;
  }

  M5.dis.displaybuff((uint8_t*)image_SCONF);
  Serial.println("SmartConfig Begin.");
  WiFi.mode(WIFI_AP_STA);
  WiFi.beginSmartConfig();

  Serial.print("Waiting SmartConfig.");
  while(!WiFi.smartConfigDone()){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nSmartConfig received.");

  //SmartConfigが成功すると、自動的に接続される。
  //NTP同期時にも接続・切断するので、一度切断する。
  while(true){
    int status = WiFi.status();
    if (status != WL_CONNECTED) {
      delay(1000);
      continue;
    }
    WiFi.disconnect();
    break;
  }
}


void setup() {
  
  Serial.begin(115200);
  M5.begin(true, false, true);

  delay(50);
  M5.dis.setBrightness(20);
  memcpy(display_buffer, image_ZEROZERO, 347);

  trySmartConfig();

  ntp_refresh();
  lastUpdateHour = timeClient.getHours();
}

void loop() {
  
  if (abs(timeClient.getHours() - lastUpdateHour) > 0) {
    //「時」が変わったタイミングで同期する。
    Serial.println("refresh.");
    ntp_refresh();
    lastUpdateHour = timeClient.getHours();
  }

  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();

  setBuffer(IMAGES[hours / 10], display_buffer, 0);
  setBuffer(IMAGES[hours % 10], display_buffer, 1);
  setBuffer(IMAGES[minutes / 10], display_buffer, 2);
  setBuffer(IMAGES[minutes % 10], display_buffer, 3);

  M5.dis.setWidthHeight(23, 5);
  M5.dis.animation((uint8_t*)display_buffer, 200, LED_DisPlay::kMoveLeft, 19);

  Serial.printf("Display Updated. time:%s\n", timeClient.getFormattedTime());
  delay(5250);
}
