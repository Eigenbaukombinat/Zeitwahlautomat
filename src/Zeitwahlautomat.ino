#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <NTPClient.h>
#include <TimeLib.h>

const char *ssid     = ""; // WLAN SSID
const char *password = ""; // WLAN Password
const char* mqttServer = ""; // MQTT Server Address
const int mqttPort = 1883;


unsigned long previousMillis1 = 0;
unsigned long interval1 = 60000;

int lcdColumns = 16;
int lcdRows = 2;
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
int menu = 0;
int aender = 1;
int wieh = 0;
int wiemin = 0;
int wie = 0;
int wie1 = 1;
int eingabe = 0;
int rebootcounter = 0;

char * senden3;
String senden = "";
String senden2 = "";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "de.pool.ntp.org", 7200, 60000);

char Time[ ] = "00:00";

char Date[ ] = "00.00.2000";

byte second_, minute_, hour_, day_, month_;
byte last_second = 70;
byte last_hour = 70;

int year_;

int sectest;

unsigned long unix_epoch;


LiquidCrystal_I2C lcd(0x3F, lcdColumns, lcdRows);

char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {32, 33, 25, 26}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {27, 14, 12}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

WiFiClient espClient;
PubSubClient client(espClient);

// Sommerzeit-Berechnung für Deutschland
bool isSommerzeit(int year, int month, int day, int hour) {
  // Letzter Sonntag im März
  int lastMarchSunday = 31 - ((5 * year / 4 + 4) % 7);
  // Letzter Sonntag im Oktober
  int lastOctoberSunday = 31 - ((5 * year / 4 + 1) % 7);

  if (month < 3 || month > 10) return false; // Jan, Feb, Nov, Dez: keine Sommerzeit
  if (month > 3 && month < 10) return true;  // Apr bis Sep: immer Sommerzeit

  if (month == 3) {
    // März: ab letztem Sonntag 2:00 Uhr Sommerzeit
    if (day > lastMarchSunday) return true;
    if (day < lastMarchSunday) return false;
    return hour >= 2;
  }
  if (month == 10) {
    // Oktober: bis letztem Sonntag 2:00 Uhr Sommerzeit
    if (day < lastOctoberSunday) return true;
    if (day > lastOctoberSunday) return false;
    return hour < 3;
  }
  return false;
}

int timezoneOffset() {
  if (isSommerzeit(year_, month_, day_, hour_)) {
    return 7200; // MESZ: UTC+2
  } else {
    return 3600; // MEZ: UTC+1
  }
}

void setup() {
  // initialize LCD
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  // client.setCallback(callback);
  lcd.clear();
  lcd.setCursor(0, 0);
  // print message
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("0000000000000000");
  // print message
  lcd.clear();

  timeClient.begin();
  
  // Erste Zeit holen
  timeClient.update();
  unix_epoch = timeClient.getEpochTime();
  minute_ = minute(unix_epoch);
  hour_   = hour(unix_epoch);
  day_    = day(unix_epoch);
  month_  = month(unix_epoch);
  year_   = year(unix_epoch);
  
  // Offset für Sommer-/Winterzeit setzen
  timeClient.setTimeOffset(timezoneOffset());
  
  // Zeit und Sommerzeitstatus anzeigen
  char zeitBuffer[6];
  snprintf(zeitBuffer, sizeof(zeitBuffer), "%02d:%02d", hour_, minute_);
  char datumBuffer[11];
  snprintf(datumBuffer, sizeof(datumBuffer), "%02d.%02d.%04d", day_, month_, year_);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(zeitBuffer);
  lcd.setCursor(6, 0);
  lcd.print(datumBuffer);
  lcd.setCursor(0, 1);
  if (isSommerzeit(year_, month_, day_, hour_)) {
    lcd.print("MESZ (Sommerzeit)");
  } else {
    lcd.print("MEZ (Winterzeit)");
  }
  delay(3000); // 3 Sekunden anzeigen
  lcd.clear();
}

void setup_wifi() {

  delay(10);
  //We start by connecting to a WiFi network
  Serial.println();
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wifi connected");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  unix_epoch = timeClient.getEpochTime();
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {

    //if (client.connect("Nextion", mqttUser, mqttPassword,"/ebkanwe/on", 0, true, "offline")) {
    if (client.connect("Nextion", "ebkanwe/on", 0, true, "offline")) {

      Serial.println("MQTT");
      client.publish("ebkanwe/on", "online");
    
    } else {
      Serial.println("nicht MQTT");
      delay(5000);
      rebootcounter ++;
      Serial.println("rebootcounter");
      Serial.println(rebootcounter);
        if (rebootcounter == 5)
  {
    ESP.restart();
}
    }
  }
}


void loop() {
  while (!client.connected()) {
    Serial.println("kein MQTT");
    lcd.print("kein MQTT ... reconnect");
    reconnect();
  }


  if ( aender == 1) {
    if (menu == 0) {
      lcd.noBlink();
      lcd.clear();
      lcd.setCursor(0, 0);
      // print message
      lcd.print(Time);
      lcd.setCursor(6, 0);
      // print message
      lcd.print(Date);
      lcd.setCursor(0, 1);
      // print message
      lcd.print("9 bis wann");
    }
    if (menu == 1) {
      //lcd.print("Wie lange");
    }


    if (menu == 2) {
      Serial.println("Menu 2");
      lcd.blink();
      lcd.clear();
      lcd.setCursor(0, 0);
      // print message
      lcd.print("Bis wann");
      lcd.setCursor(0, 1);
      // print message
      lcd.print(wieh);
      lcd.setCursor(2, 1);
      lcd.print(":");
      lcd.setCursor(3, 1);
      lcd.print(wiemin);
      switch (wie1) {
        case 1:
          lcd.setCursor(0, 1);
          break;
        case 2:
          lcd.setCursor(1, 1);
          break;
        case 3:
          lcd.setCursor(3, 1);
          break;
        case 4:
          lcd.setCursor(4, 1);
          break;
          lcd.noBlink();
      }
    }


    if (menu == 3) {
      lcd.clear();
      lcd.setCursor(0, 0);
      // print message
      lcd.print("Solang bleiben");
      lcd.setCursor(0, 1);
      // print message
      lcd.print(wieh);
      lcd.setCursor(2, 1);
      lcd.print(":");
      lcd.setCursor(3, 1);
      lcd.print(wiemin);
      lcd.setCursor(5, 1);
      lcd.print("*=JA #=NEIN");
    }
    if (menu == 4) {
      lcd.clear();
      lcd.setCursor(0, 0);
      // print message
      lcd.print("Bis bleiben");
      lcd.setCursor(0, 1);
      // print message
      lcd.print(wieh);
      lcd.setCursor(2, 1);
      lcd.print(":");
      lcd.setCursor(3, 1);
      lcd.print(wiemin);
      lcd.setCursor(5, 1);
      lcd.print("*=JA #=NEIN");
    }

    aender = 0;

  }
  char key = keypad.getKey();

  if (key != NO_KEY) {
    Serial.println(key);

    if (menu == 3) {
      if (key == '*') {
        Serial.println("JA");
        menu = 0;
        aender = 1;
        wie1 = 1;
        wie = wieh * 100 + wiemin;
        client.publish("space/status/wielange", String(wie).c_str());
        Serial.println(wieh);
        Serial.println(wiemin);
        wieh = 0;
        wiemin = 0;
      }
      if (key == '#') {
        menu = 0;
        aender = 1;
        wie1 = 1;
        wieh = 0;
        wiemin = 0;
      }

    }
    if (menu == 4) {
      if (key == '*') {
        Serial.println("JA");
        menu = 0;
        aender = 1;
        wie1 = 1;
        wie = wieh * 100 + wiemin;
        client.publish("space/status/biswann", String(wie).c_str());
        Serial.println(wieh);
        Serial.println(wiemin);
        Serial.println(wie);
        wieh = 0;
        wiemin = 0;
      }
      if (key == '#') {
        menu = 0;
        aender = 1;
        wie1 = 1;
        wieh = 0;
        wiemin = 0;
      }

    }
    if (menu == 1 || menu == 2) {

      switch (key) {
        case '1':
          eingabe = 1;
          break;
        case '2':
          eingabe = 2;
          break;
        case '3':
          eingabe = 3;
          break;
        case '4':
          eingabe = 4;
          aender = 1;
          break;
        case '5':
          eingabe = 5;
          break;
        case '6':
          eingabe = 6;
          break;
        case '7':
          eingabe = 7;
          break;
        case '8':
          eingabe = 8;
          break;
        case '9':
          eingabe = 9;
          break;
        case '0':
          eingabe = 0;
          break;

      }
      Serial.println(eingabe);
      if (key != '*' && key != '#' && key != 'C') {
        switch (wie1) {
          case 1:
            if ((eingabe * 10) < 24) {
              wieh = eingabe * 10;

              aender = 1;
              wie1 = 2;
            }
            break;
          case 2:
            if ((wieh + eingabe) < 24) {
              wieh = wieh + eingabe;
              aender = 1;
              wie1 = 3;
            }
            break;
          case 3:
            if ((eingabe * 10) < 60) {
              wiemin = eingabe * 10;
              aender = 1;
              wie1 = 4;
            }
            break;
          case 4:
            if ((wiemin + eingabe) < 60) {
              wiemin = wiemin + eingabe;
              aender = 1;
              wie1 = 1;
              if (menu == 1) {
                menu = 3;
              } else {
                menu = 4;
              }
            }
            break;
        }
      }
    }
    if (menu == 0) {
      if (key == '8') {
        menu = 1;
        aender = 1;
        wie1 = 1;
        wieh = 0;
        wiemin = 0;
      }
      if (key == '9') {
        menu = 2;
        aender = 1;
        wie1 = 1;
        wieh = 0;
        wiemin = 0;
      }
    }
    if (key == 'D') {
      menu = 0;
      aender = 1;
    }
  }
  client.loop();
  //delay(10);
  timeClient.update();
  unix_epoch = timeClient.getEpochTime();


  minute_ = minute(unix_epoch);

  hour_   = hour(unix_epoch);

  day_    = day(unix_epoch);

  month_  = month(unix_epoch);

  year_   = year(unix_epoch);

  // Offset für Sommer-/Winterzeit setzen
  timeClient.setTimeOffset(timezoneOffset());

  if (last_second != minute_) {
    last_second = minute_;
    Serial.println("Neue Minute");
    aender = 1;
  }


  if (last_hour != hour_) {
    last_hour = hour_;
    Serial.println("Neue Stunde");
    unix_epoch = timeClient.getEpochTime();
  }


  Time[4]  = minute_ % 10 + 48;

  Time[3]  = minute_ / 10 + 48;

  Time[1]  = hour_   % 10 + 48;

  Time[0]  = hour_   / 10 + 48;



  Date[0]  = day_   / 10 + 48;

  Date[1]  = day_   % 10 + 48;

  Date[3]  = month_  / 10 + 48;

  Date[4]  = month_  % 10 + 48;

  Date[8] = (year_   / 10) % 10 + 48;

  Date[9] = year_   % 10 % 10 + 48;



  // Send time and date to serial monitor

  // Serial.println(Time);

  //  Serial.println(Date);

client.loop();
}
