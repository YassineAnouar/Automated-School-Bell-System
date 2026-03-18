// Automated School Bell System
// Developed by Yassine Anouar

#include <RTClib.h>

RTC_DS3231 rtc;

const int relayPin = 7;

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  rtc.begin();
}

void loop() {
  DateTime now = rtc.now();

  // Example bell time: 08:00
  if (now.hour() == 8 && now.minute() == 0) {
    digitalWrite(relayPin, HIGH);
    delay(5000);  // Bell rings for 5 seconds
    digitalWrite(relayPin, LOW);
  }

  delay(1000);
}
