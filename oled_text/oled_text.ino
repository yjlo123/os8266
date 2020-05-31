#include <math.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>

const char* ssid = "Ghost_2.4";
const char* password = "j62u*2qr";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     LED_BUILTIN // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int btnPin1 = D6;
int btnPin2 = D7;
int btnPin3 = D8;
int temp = 0; 


const int menuItemCount = 5;
int currentApp = 0;
int currentMenuItem = 0;
int currentMenuFirstItem = 0;
int timeRefresh = 5;

int cacheHour = 0;
int cacheMinute = 0;
int cacheSecond = 0;

void tick() {
  cacheSecond++;
  if (cacheSecond >= 60) {
    cacheMinute++;
    cacheSecond = 0;
  }
  if (cacheMinute >= 60) {
    cacheHour++;
    cacheMinute = 0;
  }
  if (cacheHour == 24) {
    cacheHour = 0;
  }
}

void setup() {
  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();


  pinMode(btnPin1, INPUT);
  pinMode(btnPin2, INPUT);
  pinMode(btnPin3, INPUT);

  WiFi.begin(ssid, password);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int dots = 0;
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    //Serial.println("Connecting...");
    display.setCursor(0,0);
    display.clearDisplay();
    display.print(F("Connecting"));
    for (int16_t i=0; i<dots; i++) {
      display.print(F("."));
    }
    dots++;
    if (dots == 6) {
      WiFi.begin(ssid, password);
      Serial.println("Retry connecting");
    }
    
    dots = dots % 6;
    display.display();
    Serial.println(WiFi.status());
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.print(F("Connected!"));
  display.display();
  delay(500);
}

void loop() {
  switch(currentApp) {
    case 1:
      // words
      wordApp();
      break;
    case 2:
      timeApp();
      break;
    case 3:
      inputApp();
      break;
    case 4:
      emptyApp();
      break;
    case 5:
      aboutApp();
      break;
    default:
      homeApp();
  }

}

void displayScrollBar(int count, int current) {
  const float barFullHeight = 44;
  int height = round(barFullHeight / count);
  int top = height * current;

  display.fillRect(121, 16, 7, 48, SSD1306_BLACK);
  display.drawRect(121, 16, 7, 48, SSD1306_WHITE);

  int barTop = 18 + top;
  if (barTop + height > 62) {
    height = 62 - barTop;
  }
  if (count == current + 1 && barTop + height < 62) {
    height = 62 - barTop;
  }
  display.fillRect(123, barTop, 3, height, SSD1306_WHITE);
  
  display.display();
  delay(1);
}

void showError(char msg[]) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 8);
  display.print(F("Error..."));

  display.setCursor(0, 20);
  display.print(msg);
  
  display.setCursor(0, 32);
  display.print(F("Press LEFT to return"));
  display.display();
}

void dislayMenu() {
  char *menu[menuItemCount];
  menu[0] = "GRE       ";
  menu[1] = "TIME      ";
  menu[2] = "KEYBOARD  ";
  menu[3] = "SETTINGS  ";
  menu[4] = "ABOUT     ";


  display.setTextSize(2);
  for (int i = 0; i < 3; i++) {
    display.setCursor(0, 16 + 16 * i);
    int menuItemIndex = currentMenuFirstItem + i;
    if (menuItemIndex == currentMenuItem) {
      //menu[first + i][0] = '>';
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else {
      //menu[first + i][0] = ' ';
      display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    display.println(menu[menuItemIndex]);
  }
  display.display();

  displayScrollBar(menuItemCount, currentMenuItem);
}

void homeApp() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print(F("Welcome!"));
  display.display();

  dislayMenu();

  for (;;) {
    temp = digitalRead(btnPin1);
    if (temp == HIGH) {
      delay(200);
      if (currentMenuItem > 0) {
        currentMenuItem--;
        if (currentMenuItem < currentMenuFirstItem) {
          currentMenuFirstItem--;
        }
        dislayMenu();
      }
    }

    temp = digitalRead(btnPin2);
    if (temp == HIGH) {
      delay(200);
      if (currentMenuItem < menuItemCount - 1) {
        currentMenuItem++;
        if (currentMenuItem - currentMenuFirstItem > 2) {
          currentMenuFirstItem++;
        }
        dislayMenu();
      }
    }

    temp = digitalRead(btnPin3);
    if (temp == HIGH) {
      delay(200);
      currentApp = currentMenuItem + 1;
      break;
    }

    delay(10);
  }
}

void timeApp() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);

  display.setCursor(0, 0);
  display.print(F("Time"));
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://128.199.223.52:8088/time");
    int httpCode = http.GET();

    if (httpCode > 0) {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, http.getString());
      if (error) {
        showError("Error API response.");
        return;
      }

      const char* hour = doc["hour"]; 
      const char* minute = doc["minute"];
      const char* second = doc["second"];

      display.setTextSize(3);
      display.setCursor(0, 24);
      display.print(hour);

      display.setCursor(36, 24);
      display.print(F(":"));

      display.setCursor(52, 24);
      display.print(minute);

      display.setTextSize(2);
      display.setCursor(96, 32);
      display.print(second);
  
      display.display();
    } else {
      showError("Error API response.");
    }
    http.end();
  } else {
    showError("Connection Lost");
  }
  timeRefresh = 0;

  tick();
  timeRefresh ++;

  // 1 second
  for (int i=0; i < 100; i++) {
    temp = digitalRead(btnPin3);
    if (temp == HIGH) {
      delay(200);
    }
    
    temp = digitalRead(btnPin1);
    if (temp == HIGH) {
      delay(200);
      currentApp = 0;
      break;
    }
    delay(10);
  }
}

void wordApp() {
 if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http; //Object of class HTTPClient
    http.begin("http://128.199.223.52:8088/word");
    int httpCode = http.GET();

    if (httpCode > 0) {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, http.getString());
      if (error) {
        showError("Error API response.");
        return;
      }

      const char* w = doc["word"]; 
      const char* meaning = doc["meaning"]; 

      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0,0);
      display.print(w);
  
      display.setTextSize(1);
      display.setCursor(0,16);
      display.println(meaning);
      display.display();
    } else {
      showError("Error API response.");
    }
    http.end(); //Close connection
  } else {
    Serial.println("Connection Lost");
    showError("Connection Lost");
  }

  for (int i=0; i < 1500; i++) {
    temp = digitalRead(btnPin3);
    if (temp == HIGH) {
      delay(200);
      break;
    }
    temp = digitalRead(btnPin1);
    if (temp == HIGH) {
      delay(200);
      currentApp = 0;
      break;
    }
    delay(10);
  }
}

void aboutApp() {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print(F("ABOUT"));
  
  display.setTextSize(1);
  display.setCursor(0, 16);
  display.print(F("ver: 1.0"));
  display.setCursor(0, 24);
  display.print(F("liusiwei.com"));
  display.display();

  for (;;) {
    temp = digitalRead(btnPin1);
    if (temp == HIGH) {
      delay(200);
      currentApp = 0;
      break;
    }
    delay(10);
  }
}

void emptyApp() {
  showError("Coming soon.");
  for (int i=0; i < 1500; i++) {
    temp = digitalRead(btnPin3);
    if (temp == HIGH) {
      delay(200);
    }
    
    temp = digitalRead(btnPin1);
    if (temp == HIGH) {
      delay(200);
      currentApp = 0;
      break;
    }
    delay(10);
  }
}

void inputApp() {
  int inputCursor = 0;
  int cursorPos = 0;
  int currentRow = 0;
  int currentKey = -1;
  int keyMode = 0; // 0: selecting row, 1: selecting char
  int shiftMode = 0;

  int highlightKey = 0; // 1: backspace, 2: shift

  const int marginLeft = 10;

  static const unsigned char PROGMEM backspace_bmp[] =
  { B00000000, B00000000,
    B00001000, B00000000,
    B00011000, B00000000,
    B01111111, B11111000,
    B01111111, B11111000,
    B00011000, B00000000,
    B00001000, B00000000,
    B00000000, B00000000 };

  static const unsigned char PROGMEM backspace_inversed_bmp[] =
  { B11111111, B11111111,
    B11110111, B11111111,
    B11100111, B11111111,
    B10000000, B00000111,
    B10000000, B00000111,
    B11100111, B11111111,
    B11110111, B11111111,
    B11111111, B11111111 };

  static const unsigned char PROGMEM shift_bmp[] =
  { B00000000, B00000000,
    B00000001, B10000000,
    B00000110, B01100000,
    B00011000, B00011000,
    B00111100, B00111100,
    B00000100, B00100000,
    B00000111, B11100000,
    B00000000, B00000000 };

  static const unsigned char PROGMEM shift_inversed_bmp[] =
  { B11111111, B11111111,
    B11111110, B01111111,
    B11111001, B10011111,
    B11100111, B11100111,
    B11000011, B11000011,
    B11111011, B11011111,
    B11111000, B00011111,
    B11111111, B11111111 };

  char *keyboard1[5];
  keyboard1[0] = "`1234567890-=  ";
  keyboard1[1] = "qwertyuiop[]{} ";
  keyboard1[2] = "asdfghjkl;:'\"\\|";
  keyboard1[3] = "  zxcvbnm,.<>/?";
  keyboard1[4] = " CANCEL    OK  ";

  char *keyboard2[5];
  keyboard2[0] = "~!@#$%^&*()_+  ";
  keyboard2[1] = "QWERTYUIOP[]{} ";
  keyboard2[2] = "ASDFGHJKL;:'\"\\|";
  keyboard2[3] = "  ZXCVBNM,.<>/?";
  keyboard2[4] = " CANCEL    OK  ";

  char **rows = keyboard1;

  char buf[16] = "";

  int myAppId = currentApp;
  while (currentApp == myAppId) {
    display.clearDisplay();
  
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.print(F(">"));

    for (int i = 0; i < cursorPos; i++) {
      display.setCursor(10 + 6 * i, 0);
      display.print(buf[i]);
    }

    display.setCursor(10 + 6 * cursorPos, 0);
    if (inputCursor > 30) {
      display.print(F("_"));
    }
    inputCursor = ++inputCursor % 60;


    // draw keyboard
    
    display.setTextSize(1);
    display.setCursor(0, 16);

    for (int r = 0; r < 5; r++) {
      int top = 24 + 8 * r;

      // row cursor
      if (r == currentRow && currentRow <= 3) {
        display.setCursor(0, top);
        display.print(F(">"));
      }

      for (int i = 0; i < 15; i++) {
        if (r == currentRow && i == currentKey) {
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else {
          display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        }
        if (r == 0 && i > 12) {
          // backspace
          if (highlightKey == 1) {
            display.drawBitmap(marginLeft + i * 8, top, backspace_inversed_bmp, 16, 8, 1);
          } else {
            display.drawBitmap(marginLeft + i * 8, top, backspace_bmp, 16, 8, 1);
          }
          break;
        }
        else if (r == 3 && i == 0) {
          if (highlightKey == 2) {
            display.drawBitmap(marginLeft + i * 8, top, shift_inversed_bmp, 16, 8, 1);
          } else {
            display.drawBitmap(marginLeft + i * 8, top, shift_bmp, 16, 8, 1);
          }
          i++;
        }
        else {
          // normal char
          display.setCursor(marginLeft + i * 8, top);
          display.print(rows[r][i]);
        }
      }
    }

    if (currentRow == 4) {
      display.setCursor(10, 56);
      display.print(F(">"));
    }

    if (currentRow == 5) {
      display.setCursor(90, 56);
      display.print(F(">"));
    }

    display.display();

    // left
    temp = digitalRead(btnPin1);
    if (temp == HIGH) {
      if (keyMode == 0) {
        // row mode
        currentRow = (--currentRow + 6) % 6;
      } else {
        // char mode
        --currentKey;
        if (currentKey < 0) {
          // switch to row mode
          keyMode = 0;
        }

        if (currentRow == 0 && currentKey == 12) {
          highlightKey = 0;
        } else if (currentRow == 3 && currentKey == 1) {
          highlightKey = 2;
          --currentKey;
        } else if (currentRow == 3 && currentKey == -1) {
          highlightKey = 0;
        }
      }
      delay(150);
    }

    // right
    temp = digitalRead(btnPin2);
    if (temp == HIGH) {
      if (keyMode == 0) {
        // row mode
        currentRow = ++currentRow % 6;
      } else {
        // char mode
        currentKey = (++currentKey + 15) % 15;

        // check highlight key
        if (currentRow == 0 && currentKey == 13) {
          highlightKey = 1;
        }
        else if (currentRow == 3 && currentKey == 0) {
          highlightKey = 2;
        }
        else {
          highlightKey = 0;
        }

        // jump key space
        if (currentRow == 0 && currentKey == 14) {
          currentKey = 0;
        }
        if (currentRow == 3 && currentKey == 1) {
          currentKey = 2;
        }
      }
      delay(150);
    }
    
    temp = digitalRead(btnPin3);
    if (temp == HIGH) {
      if (keyMode == 0) {
        if (currentRow == 4 || currentRow == 5) {
          // go back
          currentApp = 0;
          delay(150);
          break;
        }
        // switch to char mode
        keyMode = 1;
        currentKey = 0;
        if (currentRow == 3) {
          highlightKey = 2;
        }
      } else {
        // input char
        if (currentRow == 0 && currentKey > 12) {
          // backspace
          if (cursorPos > 0) {
            cursorPos--;
          }
        }
        else if (currentRow == 3 && currentKey < 2) {
          // shift key
          if (shiftMode == 0) {
            rows = keyboard2;
            shiftMode = 1;
          } else {
            rows = keyboard1;
            shiftMode = 0;
          }
        }
        else if (cursorPos < 15) {
          // input char
          buf[cursorPos] = rows[currentRow][currentKey];
          cursorPos++;
        }
      }
      delay(150);
    }
 
    delay(10);
  }

}
