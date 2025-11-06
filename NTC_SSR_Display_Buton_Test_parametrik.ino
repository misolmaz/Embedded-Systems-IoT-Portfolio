#include <TM1637Display.h>
#include <math.h>
#include <PID_v1.h>

// 🛠️ Donanım Tanımlamaları
#define TEMP_SENSOR_PIN A0   // NTC Sensörü
#define SSR_PIN 9            // SSR Röle
#define SERIES_RESISTOR 10000
#define BETA 3370

#define CLK 3    // TM1637 CLK pini
#define DIO 2    // TM1637 DIO pini
#define UP_BUTTON 32
#define DOWN_BUTTON 34

// 🖥️ Display Nesnesi
TM1637Display display(CLK, DIO);

#define SMOOTHING_FACTOR 0.5
float smoothedTemperature = 0;

const uint8_t DEGREE_C_SYMBOL[] = {
  0b01100011,  // "°" sembolü
  0b00111001   // "C" harfi
};

// 🔹 **PID Değişkenleri**
double Setpoint, Input, Output;
double Kp = 6.0, Ki = 1.0, Kd = 1.8;  // 🔹 PID ayarları overshoot'u önlemek için optimize edildi

// 🔹 **SSR Röle için PID Kontrolü**
unsigned long lastSSRUpdate = 0;
const int SSR_CYCLE_TIME = 3000;
bool ssrState = false;

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

bool showSetpoint = false;
unsigned long lastButtonPressTime = 0;

void setup() {
  Serial.begin(9600);
  display.setBrightness(7);
  pinMode(SSR_PIN, OUTPUT);
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  Setpoint = 35.0;  // Başlangıç hedef sıcaklığı
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 180);  // 🔹 Maksimum PID çıkışı düşürüldü
}

void loop() {
  // 🛠️ **Buton Kontrolü (Hedef Sıcaklığı Artırma/Azaltma)**
  if (digitalRead(UP_BUTTON) == LOW) {
    Setpoint += 1;
    Serial.print("⬆️ Hedef Sıcaklık Artırıldı: ");
    Serial.println(Setpoint);
    showSetpoint = true;
    lastButtonPressTime = millis();
    delay(200);
  }

  if (digitalRead(DOWN_BUTTON) == LOW) {
    Setpoint -= 1;
    Serial.print("⬇️ Hedef Sıcaklık Azaltıldı: ");
    Serial.println(Setpoint);
    showSetpoint = true;
    lastButtonPressTime = millis();
    delay(200);
  }

  // 🔹 **NTC Sensörden Sıcaklık Okuma**
  int sensorValue = analogRead(TEMP_SENSOR_PIN);
  float voltage = sensorValue * (4.91 / 1023.0);
  float resistance = (4.91 * SERIES_RESISTOR / voltage) - SERIES_RESISTOR;
  float temperature = 1.0 / (log(resistance / SERIES_RESISTOR) / BETA + (1.0 / 298.15)) - 273.15;

  smoothedTemperature = (SMOOTHING_FACTOR * temperature) + ((1 - SMOOTHING_FACTOR) * smoothedTemperature);
  Input = smoothedTemperature;
  myPID.Compute();

  // 🔥 **SSR Röle Kontrolü**
  if (Input < Setpoint - 1) {  
    digitalWrite(SSR_PIN, HIGH);
    Serial.println("🔥 SSR TAM AÇIK - Isınma süreci devam ediyor.");
  } 
  else if (Input >= Setpoint + 0.2) {  
    digitalWrite(SSR_PIN, LOW);
    Serial.println("🚫 SSR KAPALI - Sıcaklık fazla yükseldi.");
  } 
  else if (Output < 30) {  
    digitalWrite(SSR_PIN, LOW);
    Serial.println("🛑 PID Çıkışı Düşük - SSR Kapalı.");
  } 
  else {  
    unsigned long now = millis();
    if (now - lastSSRUpdate >= SSR_CYCLE_TIME) {
      lastSSRUpdate = now;
      ssrState = true;
    }
    if (ssrState && now - lastSSRUpdate >= (Output * 0.6)) {  
      ssrState = false;
    }
    digitalWrite(SSR_PIN, ssrState ? HIGH : LOW);
  }

  Serial.print("Setpoint: ");
  Serial.print(Setpoint);
  Serial.print("  |  Input: ");
  Serial.print(Input);
  Serial.print("  |  Output: ");
  Serial.print(Output);
  Serial.print("  |  SSR: ");
  Serial.println(digitalRead(SSR_PIN) ? "ON" : "OFF");

  // 🔹 **Display Güncelleme**
  if (showSetpoint) {
    displayTemperature(Setpoint);
    if (millis() - lastButtonPressTime > 3000) {
      showSetpoint = false;
    }
  } else {
    displayTemperature(smoothedTemperature);
  }

  delay(500);
}

// 🔹 **Ekranda Sıcaklık Gösterme Fonksiyonu**
void displayTemperature(float temp) {
  int tempInt = (int)temp;
  uint8_t digits[] = {
    display.encodeDigit(tempInt / 10),
    display.encodeDigit(tempInt % 10),
    DEGREE_C_SYMBOL[0],
    DEGREE_C_SYMBOL[1]
  };
  display.setSegments(digits);
}
