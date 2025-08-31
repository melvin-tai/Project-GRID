#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <RTClib.h>
#include <Adafruit_INA219.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define BUTTON_PIN 5

DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
Adafruit_SSD1306 display(128, 64, &Wire);
Adafruit_INA219 ina219;

bool showData = true;

void setup() {
  Serial.begin(115200);
  dht.begin();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("RTC lost power. Time reset to compilation time.");
  } else {
    Serial.println("RTC running normally.");
  }

  if (!ina219.begin()) {
    Serial.println("Couldn't find INA219");
    while (1);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  delay(1000);
}

void updateRTCFromSerial() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    if (input.length() >= 19) {
      int year = input.substring(0, 4).toInt();
      int month = input.substring(5, 7).toInt();
      int day = input.substring(8, 10).toInt();
      int hour = input.substring(11, 13).toInt();
      int minute = input.substring(14, 16).toInt();
      int second = input.substring(17, 19).toInt();

      rtc.adjust(DateTime(year, month, day, hour, minute, second));
      Serial.println("RTC updated from Serial input.");
    } else {
      Serial.println("Invalid format. Use YYYY-MM-DD HH:MM:SS.");
    }
  }
}

float calculateBatteryPercentage(float voltage) { 
  float minVoltage = 3.0;
  float maxVoltage = 4.2;
  float percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0; 
  if (percentage > 100.0) percentage = 100.0; 
  if (percentage < 0.0) percentage = 0.0; 
  return percentage; 
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    showData = !showData;
    delay(100);
  }

  if (showData) {
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();

    DateTime now = rtc.now();

    float busVoltage = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();

    float batteryPercentage = calculateBatteryPercentage(busVoltage);

    if (isnan(temp) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; 
    String day = daysOfTheWeek[now.dayOfTheWeek()]; 

    char date[11]; 
    snprintf(date, sizeof(date), "%02d/%02d/%04d", now.day(), now.month(), now.year());
    
    int hour = now.hour(); 
    String ampm = "am"; 
    if (hour == 0) { 
      hour = 12; 
    } else if (hour == 12) { 
      ampm = "pm"; 
    } else if (hour > 12) { 
      hour -= 12; 
      ampm = "pm"; 
    } 

    char time[11]; 
    snprintf(time, sizeof(time), "%02d:%02d:%02d%02s", hour, now.minute(), now.second(), ampm.c_str());
        
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("%s - %s\n", day.c_str(), date);
    display.printf("Time: %s\n", time);
    display.printf("Temperature: %.1f\370C\n", temp);
    display.printf("Humidity: %.1f%%\n\n", humidity);
    display.printf("Battery: %.1f%%\n", batteryPercentage);
    display.printf("Voltage: %.1f V\n", busVoltage);
    display.printf("Current: %.1f mA\n", current_mA);
    display.display();
  } else {
    display.clearDisplay();
    display.display();
  }
  delay(1000);
}
