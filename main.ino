#include <SPI.h>
//#include <esp_task_wdt.h>
#include <MFRC522.h>
#include <SD.h>
#include <RTClib.h> // RTClib kütüphanesini ekleyin
RTC_DS3231 rtc; // DS3231 RTC nesnesini oluşturun


String clockString, dateString, name, path, cardID="";

unsigned long currentTime=0 ,lastUpdateTime=0;

//#define WDT_TIMEOUT 10 
#define WAIT_TIME_BEFORE_WATCHDOG 60

bool status;

File user,logg;


MFRC522 rfid(2, 27);


String get_date(){

  String day, mon;
  DateTime now = rtc.now(); // DS3231'den tarih bilgisini alın
  if (now.day() < 10) {
    day = "0" + String(now.day());
  } else {
    day = String(now.day());
  }
  if (now.month() < 10) {
    mon = "0" + String(now.month());
  } else {
    mon = String(now.month());
  }
  dateString = String(day) + String(".") + String(mon) + String(".") + String(now.year());
  return dateString;

}

String get_time(){

  String hour, minute, seconds;  
  DateTime now = rtc.now(); 
  if (now.hour() < 10) {
    hour = "0" + String(now.hour());
  } else {
    hour = String(now.hour());
  }
  if (now.minute() < 10) {
    minute = "0" + String(now.minute());
  } else {
    minute = String(now.minute());
  }
  if (now.second() < 10) {
    seconds = "0" + String(now.second());
  } else {
    seconds = String(now.second());
  }
  clockString = String(hour) + String(":") + String(minute) + String(":") + String(seconds);
  return clockString;

}

void add_logg(){

  String path = String("/log/") + String(get_date()) + String(".txt");
  logg = SD.open(path, FILE_APPEND);
  if (logg) {
    Serial.println("Log tutuluyor");
    logg.print(get_time());
    logg.print(",");
    logg.print(name);
    logg.print(",");
    logg.println(cardID);
    logg.close();
    Serial.println("ok");
    Serial.println(name);


  } else {
    Serial.println("hata oluştu");

  }

}

void accept_login(){

  Serial.println("entry succesful");
  digitalWrite(25, HIGH);
  delay(3000); 
  digitalWrite(25, LOW);
  ESP.restart();


}

void reject_login(){
  Serial.println("entry rejected");
  delay(1000);
  ESP.restart();




}

bool login(){
  String path = String("/users/") + String(cardID) + String(".txt");
  user = SD.open(path);
  if (user) {

    name = String(user.readStringUntil('\n'));
    //Serial.println(name);
    user.close();
    status = 1;

  } else {

    Serial.println("user not found");
    user.close();
    status = 0;

  }
  return status;
}

void read_card(){

  if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      //Serial.print("RFID/NFC Tag Type: ");
      //Serial.println(rfid.PICC_GetTypeName(piccType));

      // print UID in Serial Monitor in the hex format
      cardID = "";
      for (int i = 0; i < rfid.uid.size; i++) {

        cardID += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        cardID += String(rfid.uid.uidByte[i], HEX);
        if (i < rfid.uid.size - 1) {
          cardID += " ";
        }
      }

      //Serial.print("UID:");
      //Serial.println(cardID);

      rfid.PICC_HaltA(); // halt PICC
      rfid.PCD_StopCrypto1(); // stop encryption on PCD
    }
  }
}

void setup() {

  Serial.begin(9600);
  pinMode(25, OUTPUT);


  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522

  if (!rtc.begin()) {
    Serial.println("RTC module NOT found");
    while (1);
  }
  Serial.println("RTC module connected..");

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  if (!SD.begin(5)) {
    Serial.println("SD card not connected");
    return;
  }
  Serial.println("SD card connected..");




  cardID = "";
}

void loop() {

  currentTime = millis();

  if (currentTime - lastUpdateTime > (WAIT_TIME_BEFORE_WATCHDOG * 1000)) {
    // Belirli bir süre geçtiğinde izleme köpeği zamanlayıcısını etkinleştir

    ESP.restart();

  }

  read_card();

  if(cardID != ""){
    Serial.println(cardID);

    lastUpdateTime = currentTime; 

    status = login();
    if(status){
      Serial.println(cardID);
      //accept_login();
      add_logg();
      cardID = "";
      accept_login();

    }
    else{ 
      cardID = "";
      reject_login();

    }
  }
  
  
}
