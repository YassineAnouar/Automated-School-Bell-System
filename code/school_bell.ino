// Automated School Bell System
// Developed by Yassine Anouar


#include <Wire.h>
#include <DS3231.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <avr/wdt.h>

#define MODE_WEEKEND 2   

LiquidCrystal_I2C lcd(0x27,16,2);
DS3231 rtc(SDA, SCL);
Time t;

const uint8_t PIN_BTN_UP = 2;
const uint8_t PIN_BTN_DOWN = 3;
const uint8_t PIN_BTN_ENTER = 4;
const uint8_t PIN_BTN_BACK = 5;
const uint8_t PIN_RELAY = 6;
const uint8_t PIN_LAMP = 7;

const int ADDR_MODE = 0; 

uint8_t currentMode = 0; 
int lastRungMinute = -1; 
unsigned long lastDebounceMillis = 0;
const unsigned long DEBOUNCE_DELAY = 50;

enum Screen { DASHBOARD, MENU, SET_TIME, SET_DATE, CHOOSE_MODE, TEST_BELL };
Screen screen = DASHBOARD;
int menuIndex = 0;

bool readButton(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

bool buttonPressed(uint8_t pin) {
  static uint32_t lastPressTime[8] = {0};
  uint8_t idx = pin; 
  bool pressed = false;
  if (readButton(pin)) {
    unsigned long now = millis();
    if (now - lastPressTime[idx] > 200) { 
      pressed = true;
      lastPressTime[idx] = now;
    }
  }
  return pressed;
}


struct Hm { uint8_t h; uint8_t m; };

const Hm normal_monthu[] = {
  {8,15},{8,30},{9,25},{9,30},{10,25},{10,35},{11,30},{11,35},{12,30},
  {14,15},{14,30},{15,25},{15,30},{16,25},{16,35},{17,30},{17,35},{18,30}
};
const int normal_monthu_len = sizeof(normal_monthu)/sizeof(normal_monthu[0]);


const Hm normal_fri[] = {
  {8,15},{8,30},{9,25},{9,30},{10,25},{10,35},{11,30},{11,35},{12,30},
  {14,45},{15,0},{15,55},{16,0},{16,55},{17,5},{18,0},{18,5},{19,0}
};
const int normal_fri_len = sizeof(normal_fri)/sizeof(normal_fri[0]);


const Hm normal_sat[] = {
  {8,15},{8,30},{9,25},{9,30},{10,25},{10,35},{11,30},{11,35},{12,30}
};
const int normal_sat_len = sizeof(normal_sat)/sizeof(normal_sat[0]);

const Hm ram_monthu[] = {
  {8,25},{8,40},{9,25},{9,30},{10,15},{10,25},{11,10},{11,15},{12,0},
  {12,15},{12,30},{13,15},{13,20},{14,5},{14,15},{15,0},{15,5},{15,50}
};
const int ram_monthu_len = sizeof(ram_monthu)/sizeof(ram_monthu[0]);

const Hm ram_fri[] = {
  {8,15},{8,30},{9,15},{9,20},{10,5},{10,15},{11,0},{11,5},{11,50},
  {13,25},{13,40},{14,25},{14,30},{15,15},{15,25},{16,10},{16,15},{17,0}
};
const int ram_fri_len = sizeof(ram_fri)/sizeof(ram_fri[0]);

const Hm ram_sat[] = {
  {8,25},{8,40},{9,25},{9,30},{10,15},{10,25},{11,10},{11,15},{12,0}
};
const int ram_sat_len = sizeof(ram_sat)/sizeof(ram_sat[0]);


void setup(){

  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_ENTER, INPUT_PULLUP);
  pinMode(PIN_BTN_BACK, INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LAMP, OUTPUT);

  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LAMP, LOW);

  Wire.begin();
  rtc.begin();
  lcd.init();
  lcd.backlight();

  byte m = EEPROM.read(ADDR_MODE);
  if (m == 0xFF) m = 0; 
  if (m > 2) m = 0;     
  currentMode = m;
  digitalWrite(PIN_LAMP, (currentMode == 1) ? HIGH : LOW); 

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Bell System v1");
  lcd.setCursor(0,1);
  lcd.print("Mode: ");
  if (currentMode == 0) lcd.print("Normal");
  else if (currentMode == 1) lcd.print("Ramadan");
  else lcd.print("Weekend");
  delay(900);
  lcd.clear();
}

void loop(){
  t = rtc.getTime();
  if (t.year < 2000 || t.year > 2099) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("RTC ERROR!");
    lcd.setCursor(0,1);
    lcd.print("Set time in menu");
    delay(1000);
    if (buttonPressed(PIN_BTN_ENTER)) { screen = MENU; menuIndex = 0; }
    return;
  }

  if (screen == DASHBOARD) {
    showDashboard();
    if (buttonPressed(PIN_BTN_ENTER)) { screen = MENU; menuIndex = 0; delay(200); return; }
    checkSchedulesAndRing();
  }
  else if (screen == MENU) {
    showMenu();
    handleMenuInput();
  }
  delay(150);
}

void showDashboard(){
  lcd.clear();
  lcd.setCursor(0,0);
  char buf[17];
  sprintf(buf,"Time:%02d:%02d:%02d", t.hour, t.min, t.sec);
  lcd.print(buf);
  lcd.setCursor(0,1);
  const char *days[] = {"","Mon","Tue","Wed","Thu","Fri","Sat"};
  lcd.print(days[t.dow]);
  lcd.print(" ");
  if (currentMode == 0) lcd.print("Normal");
  else if (currentMode == 1) lcd.print("Ramadan");
  else lcd.print("Weekend");
}

const char* menuItems[] = {"Set Time","Set Date","Choose Mode","Test Bell","Save & Exit"};
const int menuCount = sizeof(menuItems)/sizeof(menuItems[0]);

void showMenu(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(">"); lcd.print(menuItems[menuIndex]);
  lcd.setCursor(0,1);
  lcd.print("UP/DN Enter Back");
}

void handleMenuInput(){
  if (buttonPressed(PIN_BTN_UP)) {
    menuIndex = (menuIndex - 1 + menuCount) % menuCount;
    delay(120);
    return;
  }
  if (buttonPressed(PIN_BTN_DOWN)) {
    menuIndex = (menuIndex + 1) % menuCount;
    delay(120);
    return;
  }
  if (buttonPressed(PIN_BTN_ENTER)) {
    if (menuIndex == 0) { menuSetTime(); }
    else if (menuIndex == 1) { menuSetDate(); }
    else if (menuIndex == 2) { menuChooseMode(); }
    else if (menuIndex == 3) { testBell(); }
    else if (menuIndex == 4) { 
      EEPROM.write(ADDR_MODE, currentMode);   
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Saved to EEPROM");
      delay(700);
      screen = DASHBOARD;
    }
    delay(200);
  }
  if (buttonPressed(PIN_BTN_BACK)) {
    screen = DASHBOARD;
    delay(200);
  }
}


void menuSetTime(){
  int hh = t.hour; int mm = t.min; int ss = t.sec;
  bool editing = true;
  int field = 0; 

  unsigned long lastBlink = millis();
  bool blinkState = true; 

  while (editing) {
    if (millis() - lastBlink > 500) {
      blinkState = !blinkState;
      lastBlink = millis();
    }

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Set Time:");
    lcd.setCursor(0,1);

    char timeStr[17];
    sprintf(timeStr, "%02d:%02d:%02d", hh, mm, ss);

    if (!blinkState) {
      switch (field) {
        case 0: 
          timeStr[0] = '-';
          timeStr[1] = '-';
          break;
        case 1: 
          timeStr[3] = '-';
          timeStr[4] = '-';
          break;
        case 2: 
          timeStr[6] = '-';
          timeStr[7] = '-';
          break;
      }
    }

    lcd.print(timeStr);

    
    delay(150); 

    if (buttonPressed(PIN_BTN_UP)) {
      if (field==0) hh = (hh+1)%24;
      else if (field==1) mm = (mm+1)%60;
      else ss = (ss+1)%60;
    }
    if (buttonPressed(PIN_BTN_DOWN)) {
      if (field==0) hh = (hh+23)%24;
      else if (field==1) mm = (mm+59)%60;
      else ss = (ss+59)%60;
    }
    if (buttonPressed(PIN_BTN_ENTER)) {
      field++;
      if (field>2) {
        rtc.setTime(hh, mm, ss);
        lcd.clear(); lcd.setCursor(0,0); lcd.print("Time Set");
        delay(600);
        editing = false;
      } else {
        blinkState = true;
        lastBlink = millis();
      }
    }
    if (buttonPressed(PIN_BTN_BACK)) { editing = false; }
  }
}

void menuSetDate(){
  int day = t.date; int mon = t.mon; int yr = t.year;
  int dow = t.dow;
  bool editing = true;
  int field = 0; 

  unsigned long lastBlink = millis();
  bool blinkState = true;

  while (editing) {
    if (millis() - lastBlink > 500) {
      blinkState = !blinkState;
      lastBlink = millis();
    }

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Set Date:");
    lcd.setCursor(0,1);

    char dateStr[17];
    sprintf(dateStr, "%02d/%02d/%04d D%u", day, mon, yr, dow);

    if (!blinkState) {
      switch (field) {
        case 0: 
          dateStr[0] = '-';
          dateStr[1] = '-';
          break;
        case 1: 
          dateStr[3] = '-';
          dateStr[4] = '-';
          break;
        case 2: 
          dateStr[6] = '-';
          dateStr[7] = '-';
          dateStr[8] = '-';
          dateStr[9] = '-';
          break;
        case 3: 
          dateStr[14] = '-';
          break;
      }
    }

    lcd.print(dateStr);
    delay(150);

    if (buttonPressed(PIN_BTN_UP)) {
      if (field==0) day = constrain(day+1,1,31);
      else if (field==1) mon = constrain(mon+1,1,12);
      else if (field==2) yr = constrain(yr+1,2000,2099);
      else dow = (dow%6)+1; 
    }
    if (buttonPressed(PIN_BTN_DOWN)) {
      if (field==0) day = constrain(day-1,1,31);
      else if (field==1) mon = constrain(mon-1,1,12);
      else if (field==2) yr = constrain(yr-1,2000,2099);
      else dow = (dow+4)%6 + 1; 
    }
    if (buttonPressed(PIN_BTN_ENTER)) {
      field++;
      if (field>3) {
        rtc.setDate(day, mon, yr);
        rtc.setDOW((Time::DayOfWeek)dow); 
        lcd.clear(); lcd.setCursor(0,0); lcd.print("Date Set");
        delay(700);
        editing = false;
      } else {
        blinkState = true;
        lastBlink = millis();
      }
    }
    if (buttonPressed(PIN_BTN_BACK)) { editing = false; }
  }
}

void menuChooseMode(){
  bool choosing = true;
  int sel = currentMode; 
  while (choosing) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Choose Mode:");
    lcd.setCursor(0,1);
    if (sel == 0) lcd.print("Normal");
    else if (sel == 1) lcd.print("Ramadan");
    else lcd.print("Weekend");
    delay(150);
    
    if (buttonPressed(PIN_BTN_UP)) {
      sel = (sel + 1) % 3;   
      delay(200);
    }
    if (buttonPressed(PIN_BTN_DOWN)) {
      sel = (sel + 2) % 3;   
      delay(200);
    }
    if (buttonPressed(PIN_BTN_ENTER)) {
      currentMode = sel;
      digitalWrite(PIN_LAMP, (currentMode == 1) ? HIGH : LOW);
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Mode Saved");
      delay(600);
      choosing = false;
    }
    if (buttonPressed(PIN_BTN_BACK)) {
      choosing = false;
    }
  }
}

void testBell(){
  lcd.clear(); lcd.setCursor(0,0); lcd.print("Test Bell...");
  ringBell(1500);
  lcd.setCursor(0,1); lcd.print("Done");
  delay(700);
}

void checkSchedulesAndRing(){
  if (currentMode == MODE_WEEKEND) return;

  int dow = t.dow;
  int minuteOfDay = t.hour*60 + t.min;

  const Hm* arr = nullptr;
  int len = 0;
  if (currentMode == 0) {
    if (dow >=1 && dow <=4) { arr = normal_monthu; len = normal_monthu_len; }
    else if (dow == 5) { arr = normal_fri; len = normal_fri_len; }
    else if (dow == 6) { arr = normal_sat; len = normal_sat_len; }
  } else if (currentMode == 1) {
    if (dow >=1 && dow <=4) { arr = ram_monthu; len = ram_monthu_len; }
    else if (dow == 5) { arr = ram_fri; len = ram_fri_len; }
    else if (dow == 6) { arr = ram_sat; len = ram_sat_len; }
  }

  if (arr == nullptr || len==0) return;

  for (int i=0;i<len;i++){
    if (arr[i].h == t.hour && arr[i].m == t.min) {
      if (lastRungMinute != minuteOfDay) {
        ringBell(3500); 
        lastRungMinute = minuteOfDay;
      }
      return; 
    }
  }
}

void ringBell(unsigned long ms) {
  digitalWrite(PIN_RELAY, HIGH);
  delay(ms);
  digitalWrite(PIN_RELAY, LOW);
}
