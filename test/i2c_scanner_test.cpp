/*
 * I2C Scanner Test для PN532
 * Простейший тест для проверки подключения PN532 по I2C
 * 
 * ПОДКЛЮЧЕНИЕ:
 * PN532    ESP32
 * ------   -----
 * VCC  →   3.3V
 * GND  →   GND
 * SDA  →   GPIO 21
 * SCL  →   GPIO 22
 */

#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(); // Использует GPIO 21 (SDA) и GPIO 22 (SCL) по умолчанию
  
  Serial.println("\n=== I2C Scanner Test для PN532 ===");
  Serial.println("Поиск устройств на I2C шине...");
  Serial.println("Ожидаемый адрес PN532: 0x24");
  Serial.println("=====================================\n");
}

void loop() {
  byte error, address;
  int nDevices = 0;
  
  Serial.println("Сканирование I2C шины...");
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.printf("✅ I2C устройство найдено по адресу: 0x%02X", address);
      
      if (address == 0x24) {
        Serial.print(" <- PN532 NFC/RFID модуль!");
      }
      
      Serial.println();
      nDevices++;
    }
    else if (error == 4) {
      Serial.printf("❌ Неизвестная ошибка по адресу: 0x%02X\n", address);
    }
  }
  
  Serial.println("=====================================");
  
  if (nDevices == 0) {
    Serial.println("❌ Устройства I2C не найдены!");
    Serial.println("\nПроверьте подключение:");
    Serial.println("- VCC → 3.3V");
    Serial.println("- GND → GND");  
    Serial.println("- SDA → GPIO 21");
    Serial.println("- SCL → GPIO 22");
    Serial.println("- Переключатели PN532 в режим I2C");
  }
  else {
    Serial.printf("✅ Найдено устройств: %d\n", nDevices);
    
    if (nDevices == 1) {
      Serial.println("🎯 Готово к загрузке основного кода!");
    }
  }
  
  Serial.println("=====================================\n");
  delay(5000);
} 