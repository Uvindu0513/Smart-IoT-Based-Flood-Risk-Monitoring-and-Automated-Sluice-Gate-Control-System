#define BLYNK_TEMPLATE_ID "TMPL6A-KXNIWv"
#define BLYNK_TEMPLATE_NAME "FLOODsf"
#define BLYNK_AUTH_TOKEN    "ebxkfVnk_9Gi5V4_g3G2OkdBstfsifshfZ"

#include "Adafruit_VL53L0X.h"
#include <Stepper.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h> 

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>


#include <Adafruit_NeoPixel.h>

char ssid[] = "********";//WIFI ID
char pass[] = "********";//WIFI PASSWORD

const int MAX_DISTANCE_EMPTY = 178; 
const int STEPS_PER_REVOLUTION = 2048;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

const int LED_MODERATE = 13; 
const int HIGH_RISK_PIN = 12; 
const int DHT_PIN = 23;        
const int RAIN_SENSOR_PIN = 34; 
const int GSM_RX_PIN = 16;  
const int GSM_TX_PIN = 17;  
const int HALL_EFFECT_PIN = 35; 

const int SWITCH_CW_PIN = 5;
const int SWITCH_CCW_PIN = 18;

#define NEOPIXEL_PIN    4      
#define NUM_LEDS        6       

#define DHTTYPE DHT22

const String TARGET_PHONE = "+94*********";//Add your phone number.

Stepper myStepper(STEPS_PER_REVOLUTION, 25, 27, 26, 14);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
DHT dht(DHT_PIN, DHTTYPE);
Adafruit_BMP280 bmp; 
RTC_DS3231 rtc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel pixels(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

HardwareSerial gsmSerial(2);


int currentState = 0; 
int measuredDistance = 0;
int waterDepth = 0;
float percentage = 0.0;

float temperatureC = 0.0;
float humidity = 0.0;
float pressureHPa = 0.0;

bool isRaining = false;
bool lastRainState = false;
String rainStartTime = "N/A";
String rainStopTime = "N/A";
String rainStatusMessage = "No Rain";

String estimatedOverflowTimeStr = "Stable";
float previousWaterDepthMM = 0.0;
uint32_t previousTimeSec = 0;

const float MM_PER_TIP = 2.0;              
volatile unsigned long totalTips = 0;
volatile unsigned long lastTipTime = 0;
const unsigned long DEBOUNCE_DELAY = 150;  

const int MAX_TIPS_TRACKED = 40;
volatile unsigned long tipTimestamps[MAX_TIPS_TRACKED];
volatile int tipHead = 0;
volatile int tipTail = 0;
volatile int tipCountInWindow = 0;

float rainIntensity = 0.0; 


long targetStepPosition = 0;   
long currentStepPosition = 0;   

unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_INTERVAL = 1000; 

unsigned long lastOledUpdateTime = 0;
const unsigned long OLED_INTERVAL = 4000; 
int oledScreenPage = 0;          


unsigned long lastBlynkUpdateTime = 0;
const unsigned long BLYNK_INTERVAL = 2000; 


void IRAM_ATTR resetBucketTipISR();
void calculateRainIntensity();
void readDistanceAndCalculate();
void calculateOverflowEstimation();
void evaluateRiskState();
void readEnvironment();
void readBarometer();
void checkRainStatus();
bool checkPriorityManualControl();
void runMotorNonBlocking();
void updateOLEDDisplay();
String getFormattedTime(DateTime dt);
void sendSMS(String message);
void triggerEnvironmentalSMS(String eventLabel, String timestamp);
void sendDataToBlynk(); 
void updateLEDs();


void IRAM_ATTR resetBucketTipISR() {
  unsigned long currentTime = millis();
  if (currentTime - lastTipTime > DEBOUNCE_DELAY) {
    totalTips++;
    lastTipTime = currentTime;

    int nextHead = (tipHead + 1) % MAX_TIPS_TRACKED;
    if (nextHead != tipTail) { 
      tipTimestamps[tipHead] = currentTime;
      tipHead = nextHead;
      tipCountInWindow++;
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  gsmSerial.begin(9600, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN); 

  pinMode(LED_MODERATE, OUTPUT);
  pinMode(HIGH_RISK_PIN, OUTPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT); 
  

  pinMode(SWITCH_CW_PIN, INPUT_PULLUP);
  pinMode(SWITCH_CCW_PIN, INPUT_PULLUP);
  
  pinMode(HALL_EFFECT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HALL_EFFECT_PIN), resetBucketTipISR, FALLING);
  
  digitalWrite(LED_MODERATE, LOW);
  digitalWrite(HIGH_RISK_PIN, LOW);


  pixels.begin();
  pixels.setBrightness(50); 
  updateLEDs();            

  myStepper.setSpeed(8); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 OLED allocation failed"));
    while(1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Connecting Blynk...");
  display.display();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  if (!lox.begin()) {
    Serial.println("Failed to boot VL53L0X. Check wiring!");
    while(1);
  }

  dht.begin();
  bmp.begin(0x76);
  rtc.begin();
  
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  delay(1000);
  gsmSerial.println("AT"); 
  delay(500);
  gsmSerial.println("AT+CMGF=1"); 
  delay(500);
  
  Serial.println("All Systems Ready!");
}

void loop() {
  Blynk.run();

  bool isManualActive = checkPriorityManualControl();

  checkRainStatus();
  calculateRainIntensity(); 

  if (millis() - lastSensorReadTime >= SENSOR_INTERVAL) {
    lastSensorReadTime = millis();

    bool isMotorMoving = (currentStepPosition != targetStepPosition) || isManualActive;

    if (!isMotorMoving) {
      readDistanceAndCalculate(); 
      calculateOverflowEstimation();
      evaluateRiskState();        
    } else {
      Serial.print("ToF Paused (Motor Active) | ");
    }

    readEnvironment();
    readBarometer();
    
    Serial.print("DHT_T: "); Serial.print(temperatureC, 1);
    Serial.print("C | Hum: "); Serial.print(humidity, 1);
    Serial.print("% | Pres: "); Serial.print(pressureHPa, 1);
    Serial.print(" hPa | Rain: "); Serial.print(rainStatusMessage);
    Serial.print(" | Intensity: "); Serial.print(rainIntensity, 1); Serial.print(" mm/min");
    Serial.print(" | Est. Overflow: "); Serial.print(estimatedOverflowTimeStr);
    if (currentState == 2) Serial.println(" | STATUS: HIGH RISK");
    else if (currentState == 1) Serial.println(" | STATUS: Moderate Risk");
    else Serial.println(" | STATUS: No Risk");
  }

  if (millis() - lastBlynkUpdateTime >= BLYNK_INTERVAL) {
    lastBlynkUpdateTime = millis();
    sendDataToBlynk();
  }

  if (millis() - lastOledUpdateTime >= OLED_INTERVAL) {
    lastOledUpdateTime = millis();
    updateOLEDDisplay();
    oledScreenPage = (oledScreenPage + 1) % 4; 
  }

  if (!isManualActive) {
    runMotorNonBlocking();
  }
}

bool checkPriorityManualControl() {
  bool cwActive = (digitalRead(SWITCH_CW_PIN) == LOW);
  bool ccwActive = (digitalRead(SWITCH_CCW_PIN) == LOW);

  if (cwActive) {
    myStepper.setSpeed(8); 
    myStepper.step(10);    
    currentStepPosition += 10;
    targetStepPosition = currentStepPosition; 
    myStepper.setSpeed(8); 
    return true;
  } 
  else if (ccwActive) {
    myStepper.setSpeed(8);
    myStepper.step(-10);   
    currentStepPosition -= 10;
    targetStepPosition = currentStepPosition; 
    myStepper.setSpeed(8); 
    return true;
  }

  return false; 
}

void calculateRainIntensity() {
  unsigned long now = millis();
  
  noInterrupts();
  while (tipCountInWindow > 0 && (now - tipTimestamps[tipTail] > 60000)) {
    tipTail = (tipTail + 1) % MAX_TIPS_TRACKED;
    tipCountInWindow--;
  }
  int currentValidTips = tipCountInWindow;
  interrupts();

  rainIntensity = currentValidTips * MM_PER_TIP;
}

void sendDataToBlynk() {
  if (Blynk.connected()) {
    DateTime now = rtc.now();
    String currentTimeStr = getFormattedTime(now);

    Blynk.virtualWrite(V0, measuredDistance);      
    Blynk.virtualWrite(V1, percentage);            
    Blynk.virtualWrite(V2, temperatureC);          
    Blynk.virtualWrite(V3, humidity);              
    Blynk.virtualWrite(V4, pressureHPa);           
    Blynk.virtualWrite(V5, rainStatusMessage);     
    Blynk.virtualWrite(V6, currentTimeStr);        
    Blynk.virtualWrite(V7, rainIntensity);         
    Blynk.virtualWrite(V8, estimatedOverflowTimeStr); 
  }
}

void readDistanceAndCalculate() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {
    measuredDistance = measure.RangeMilliMeter;
    waterDepth = MAX_DISTANCE_EMPTY - measuredDistance;

    if (waterDepth < 0) waterDepth = 0;
    if (waterDepth > MAX_DISTANCE_EMPTY) waterDepth = MAX_DISTANCE_EMPTY;
    percentage = ((float)waterDepth / MAX_DISTANCE_EMPTY) * 100.0;
  }
}

void calculateOverflowEstimation() {
  DateTime now = rtc.now();
  uint32_t currentTimeSec = now.unixtime();

  if (previousTimeSec == 0) {
    previousTimeSec = currentTimeSec;
    previousWaterDepthMM = waterDepth;
    estimatedOverflowTimeStr = "Calculating...";
    return;
  }

  uint32_t timeDeltaSec = currentTimeSec - previousTimeSec;
  if (timeDeltaSec < 2) return; 

  float depthDeltaMM = (float)waterDepth - previousWaterDepthMM;
  float rateOfRiseMMPerSec = depthDeltaMM / (float)timeDeltaSec;

  float remainingDepthMM = (float)MAX_DISTANCE_EMPTY - waterDepth;

  if (remainingDepthMM <= 0) {
    estimatedOverflowTimeStr = "OVERFLOWING!";
  } 
  else if (rateOfRiseMMPerSec > 0.05) { 
    uint32_t secondsToOverflow = (uint32_t)(remainingDepthMM / rateOfRiseMMPerSec);
    DateTime overflowTime = now + TimeSpan(secondsToOverflow);

    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", overflowTime.hour(), overflowTime.minute(), overflowTime.second());
    estimatedOverflowTimeStr = String(buffer);
  } 
  else {
    estimatedOverflowTimeStr = "Stable/Falling";
  }

  previousWaterDepthMM = waterDepth;
  previousTimeSec = currentTimeSec;
}

void readEnvironment() {
  humidity = dht.readHumidity();
  temperatureC = dht.readTemperature();
}

void readBarometer() {
  pressureHPa = bmp.readPressure() / 100.0; 
}

void checkRainStatus() {
  isRaining = (digitalRead(RAIN_SENSOR_PIN) == LOW);

  if (isRaining && !lastRainState) {
    DateTime now = rtc.now();
    rainStartTime = getFormattedTime(now);
    rainStatusMessage = "Raining";
    triggerEnvironmentalSMS("RAIN STARTED", rainStartTime);
  }
  else if (!isRaining && lastRainState) {
    DateTime now = rtc.now();
    rainStopTime = getFormattedTime(now);
    rainStatusMessage = "No Rain";
    triggerEnvironmentalSMS("RAIN STOPPED", rainStopTime);
  }
  
  lastRainState = isRaining; 
}

void triggerEnvironmentalSMS(String eventLabel, String timestamp) {
  String textPayload = "[" + eventLabel + "]\n";
  textPayload += "Time: " + timestamp + "\n";
  textPayload += "Water Lvl: " + String(percentage, 1) + "%\n";
  textPayload += "Est Overflow: " + estimatedOverflowTimeStr + "\n";
  textPayload += "Intensity: " + String(rainIntensity, 1) + "mm/min\n";
  textPayload += "Temp: " + String(temperatureC, 1) + "C\n";
  textPayload += "Humidity: " + String(humidity, 1) + "%\n";
  textPayload += "Pressure: " + String(pressureHPa, 1) + "hPa\n";
  
  sendSMS(textPayload);
}

void evaluateRiskState() {
  if (percentage >= 50.0) {
    digitalWrite(HIGH_RISK_PIN, HIGH);
    digitalWrite(LED_MODERATE, LOW);
    
    if (currentState == 1) {
      targetStepPosition += (-5 * STEPS_PER_REVOLUTION); 
      sendSMS("ALERT: Water level entered HIGH RISK!\nLevel: " + String(percentage, 1) + "%\nEst Overflow: " + estimatedOverflowTimeStr + "\nIntensity: " + String(rainIntensity, 1) + "mm/min");
      if (Blynk.connected()) Blynk.logEvent("high_risk", "Water level entered HIGH RISK! Est Overflow: " + estimatedOverflowTimeStr);
    } 
    else if (currentState == 0) {
      targetStepPosition += (-5 * STEPS_PER_REVOLUTION); 
      sendSMS("CRITICAL ALERT: Water level jumped to HIGH RISK!\nLevel: " + String(percentage, 1) + "%\nEst Overflow: " + estimatedOverflowTimeStr + "\nIntensity: " + String(rainIntensity, 1) + "mm/min");
      if (Blynk.connected()) Blynk.logEvent("high_risk", "CRITICAL: Water level jumped to HIGH RISK! Est Overflow: " + estimatedOverflowTimeStr);
    }
    currentState = 2;
  }
  else if (percentage >= 30.0 && percentage < 50.0) {
    digitalWrite(HIGH_RISK_PIN, LOW);
    digitalWrite(LED_MODERATE, HIGH);
    
    if (currentState == 0) {
      targetStepPosition += (-5 * STEPS_PER_REVOLUTION); 
      sendSMS("RISK ALERT: Water level is MODERATE.\nLevel: " + String(percentage, 1) + "%\nEst Overflow: " + estimatedOverflowTimeStr + "\nIntensity: " + String(rainIntensity, 1) + "mm/min");
      if (Blynk.connected()) Blynk.logEvent("moderate_risk", "Water level is MODERATE. Est Overflow: " + estimatedOverflowTimeStr);
    } 
    else if (currentState == 2) {
      targetStepPosition += (5 * STEPS_PER_REVOLUTION); 
      sendSMS("STATUS UPDATE: Risk downgraded to MODERATE.\nLevel: " + String(percentage, 1) + "%\nEst Overflow: " + estimatedOverflowTimeStr);
      if (Blynk.connected()) Blynk.logEvent("moderate_risk", "Risk downgraded to MODERATE. Est Overflow: " + estimatedOverflowTimeStr);
    }
    currentState = 1;
  }
  else {
    digitalWrite(HIGH_RISK_PIN, LOW);
    digitalWrite(LED_MODERATE, LOW);
    
    if (currentState == 1) {
      targetStepPosition += (5 * STEPS_PER_REVOLUTION); 
      sendSMS("STATUS UPDATE: Back to LOW RISK.\nLevel: " + String(percentage, 1) + "%\nEst Overflow: " + estimatedOverflowTimeStr);
      if (Blynk.connected()) Blynk.logEvent("low_risk", "System back to LOW RISK. Est Overflow: " + estimatedOverflowTimeStr);
    } 
    else if (currentState == 2) {
      targetStepPosition += (5 * STEPS_PER_REVOLUTION); 
      sendSMS("STATUS UPDATE: Risk cleared to LOW RISK.\nLevel: " + String(percentage, 1) + "%\nEst Overflow: " + estimatedOverflowTimeStr);
      if (Blynk.connected()) Blynk.logEvent("low_risk", "Risk cleared completely to LOW RISK. Est Overflow: " + estimatedOverflowTimeStr);
    }
    currentState = 0;
  }
  
  updateLEDs(); 
}

void updateLEDs() {
  pixels.clear();

  if (currentState == 0) {
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.setPixelColor(1, pixels.Color(0, 255, 0));
  } 
  else if (currentState == 1) {
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.setPixelColor(1, pixels.Color(0, 255, 0));
    pixels.setPixelColor(2, pixels.Color(255, 140, 0)); 
    pixels.setPixelColor(3, pixels.Color(255, 140, 0));
  } 
  else if (currentState == 2) {
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.setPixelColor(1, pixels.Color(0, 255, 0));
    pixels.setPixelColor(2, pixels.Color(255, 140, 0)); 
    pixels.setPixelColor(3, pixels.Color(255, 140, 0));
    pixels.setPixelColor(4, pixels.Color(255, 0, 0));   
    pixels.setPixelColor(5, pixels.Color(255, 0, 0));
  }
  
  pixels.show(); 
}

void sendSMS(String message) {
  Serial.println("--- Sending SMS Outbound ---");
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(TARGET_PHONE);
  gsmSerial.println("\"");
  delay(500); 
  gsmSerial.print(message);
  delay(500);
  gsmSerial.write(26); 
  delay(3000); // Scaled down delay to improve loop responsiveness
  Serial.println("--- SMS Sent Confirmed ---");
}

void updateOLEDDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  switch(oledScreenPage) {
    case 0: 
      display.setCursor(0, 0);
      display.setTextSize(2);
      display.println(" WATER SYS");
      display.drawFastHLine(0, 18, 128, SSD1306_WHITE);
      
      display.setTextSize(1);
      display.setCursor(0, 23);
      display.print("Water Depth: "); display.print(waterDepth); display.println(" mm");
      display.print("Percentage:  "); display.print(percentage, 1); display.println(" %");
      display.print("Overflow:    "); display.println(estimatedOverflowTimeStr);
      display.setCursor(0, 53);
      display.print("RISK: ");
      if (currentState == 2) display.println("HIGH RISK");
      else if (currentState == 1) display.println("MODERATE");
      else display.println("NO RISK");
      break;

    case 1: 
      display.setCursor(0, 0);
      display.setTextSize(2);
      display.println(" ENV SENSE");
      display.drawFastHLine(0, 18, 128, SSD1306_WHITE);
      
      display.setTextSize(1);
      display.setCursor(0, 25);
      display.print("Temp:        "); display.print(temperatureC, 1); display.println(" C");
      display.print("Humidity:    "); display.print(humidity, 1); display.println(" %");
      display.print("Pressure:    "); display.print(pressureHPa, 1); display.println(" hPa");
      break;

    case 2: 
      display.setCursor(0, 0);
      display.setTextSize(2);
      display.println(" RAIN LOG");
      display.drawFastHLine(0, 18, 128, SSD1306_WHITE);
      
      display.setTextSize(1);
      display.setCursor(0, 23);
      display.print("Status: "); display.println(rainStatusMessage);
      display.print("Rate  : "); display.print(rainIntensity, 1); display.println(" mm/min");
      display.setCursor(0, 43);
      display.print("Start : "); display.println(rainStartTime);
      display.print("Stopped: "); display.println(rainStopTime);
      break;

    case 3:
      display.setCursor(0, 0);
      display.setTextSize(2);
      display.println(" OVERFLOW");
      display.drawFastHLine(0, 18, 128, SSD1306_WHITE);
      
      display.setTextSize(1);
      display.setCursor(0, 23);
      display.print("Current Time:");
      display.setCursor(0, 33);
      display.println(getFormattedTime(rtc.now()));
      
      display.setCursor(0, 45);
      display.print("Est Overflow Time:");
      display.setCursor(0, 55);
      display.setTextSize(1);
      display.println(estimatedOverflowTimeStr);
      break;
  }
  display.display();
}

String getFormattedTime(DateTime dt) {
  char buf[] = "hh:mm:ss";
  return String(dt.toString(buf));
}

void runMotorNonBlocking() {
  if (currentStepPosition < targetStepPosition) {
    myStepper.step(1); 
    currentStepPosition++;
  } 
  else if (currentStepPosition > targetStepPosition) {
    myStepper.step(-1); 
    currentStepPosition--;
  }
}
