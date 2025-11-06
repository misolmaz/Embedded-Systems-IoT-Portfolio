#include <HardwareSerial.h>
#include <vector>
#include <string>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// RFID için seri port (UART2)
HardwareSerial RFIDSerial(2);

WiFiClientSecure client;


// EPC Kayıt Yapısı
struct EPCRecord {
  std::string epc;
  byte antennaID;
  unsigned long timestamp;
  std::string action;  // "IN" veya "OUT"
};

// EPC Kayıt Listeleri
std::vector<EPCRecord> epcRecords;
std::vector<EPCRecord> recordsToSend;

// Zaman Ayarları
const unsigned long TIME_WINDOW = 6000;     // 3 saniye (tekilleştirme için)
const unsigned long SEND_INTERVAL = 30000;  // 30 saniye (veri gönderim periyodu)
unsigned long lastSendTime = 0;

// Wi-Fi Ayarları
const char *ssid = "SUPERONLINE_WiFi_CCA8";
const char *password = "NNC3E3M9F4PU";
const char *serverURL = "https://192.168.1.61:7126/api/RFID";


void setup() {
  Serial.begin(115200);
  RFIDSerial.begin(115200, SERIAL_8N1, 16, 17);  // RX=GPIO16, TX=GPIO17
  Serial.println("RFID Reader Test");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi.");
  client.setInsecure();

  setActiveAntennas();
}

void loop() {

  sendInventoryCommand();  // Fast Switch Inventory komutunu gönder
  delay(500);              // Yanıtların alınması için bekleme süresi optimize edildi

  // EPC algılama ve işlem
  if (RFIDSerial.available()) {
    processIncomingData();
  }

  // Tekilleştirme ve giriş/çıkış analizi
  deduplicateAndAnalyze();

  // Periodik veri gönderimi
  if (millis() - lastSendTime > SEND_INTERVAL) {
    sendEPCData();
    lastSendTime = millis();
  }
}

void setActiveAntennas() {
  byte command[] = { 0xA0, 0x03, 0x01, 0x74, 0x03, 0x8A };  // Anten 1 ve 2 aktif
  RFIDSerial.write(command, sizeof(command));
  Serial.println("Set active antennas: 1 and 2.");
}

void sendInventoryCommand() {
  byte command[] = { 0xA0, 0x0D, 0x01, 0x8A, 0x00, 0x01, 0x01, 0x01, 0x02, 0x00, 0x03, 0x00, 0x00, 0x01, 0xBF };
  RFIDSerial.write(command, sizeof(command));
}

void processIncomingData() {
  sendInventoryCommand();  // Yeni komut gönder

  while (RFIDSerial.available()) {
    byte header = RFIDSerial.read();
    if (header == 0xA0) {
      byte len = RFIDSerial.read();
      byte data[len - 1];
      for (int i = 0; i < len - 1; i++) {
        data[i] = RFIDSerial.read();
      }
      byte checksum = RFIDSerial.read();
      if (validateChecksum(header, len, data, checksum)) {
        processFrame(len, data);
      }
    }
  }
}

bool validateChecksum(byte header, byte len, byte *data, byte checksum) {
  int sum = header + len;
  for (int i = 0; i < len - 1; i++) {
    sum += data[i];
  }
  return ((0xFF - (sum & 0xFF) + 1) & 0xFF) == checksum;
}

byte getAntennaID(byte freqAnt) {
  return freqAnt & 0x03;  // Son 2 bit anten ID
}

void processFrame(byte len, byte *data) {
  if (data[1] == 0x8A && len == 0x13) {  // EPC Yanıt
    byte freqAnt = data[2];
    byte antennaID = getAntennaID(freqAnt);

    // EPC verisini al
    char epc[(len - 2) * 2 + 1] = { 0 };
    for (int i = 4; i < len - 2; i++) {
      sprintf(&epc[(i - 4) * 2], "%02X", data[i]);
    }

    // Yeni kayıt oluştur
    unsigned long timestamp = millis();
    EPCRecord record = { std::string(epc), antennaID, timestamp, "" };

    // EPC'yi Tekilleştirme Kontrolü
    if (std::find_if(epcRecords.begin(), epcRecords.end(), [&](const EPCRecord &r) {
          return r.epc == std::string(epc);
        })
        == epcRecords.end()) {
      epcRecords.push_back(record);
      Serial.print("EPC Added: ");  // Sadece yeni EPC'ler için yazdır
      Serial.println(epc);
    }

  } else if (data[1] == 0x8A && len == 0x0A) {  // Tarama tamamlandı
    // Tekilleştirme analizi sonrası kayıtlar yazdırılacak (isteğe göre işlem eklenebilir)
    // Serial.println("Scan complete. Processing data...");
  }
}


void deduplicateAndAnalyze() {
  std::vector<EPCRecord> uniqueRecords;

  for (auto &record : epcRecords) {
    bool isDuplicate = false;
    for (auto &unique : uniqueRecords) {
      if (record.epc == unique.epc && (record.timestamp - unique.timestamp) < TIME_WINDOW) {
        isDuplicate = true;
        break;
      }
    }

    if (!isDuplicate) {
      // Eğer `recordsToSend` içinde bu EPC zaten varsa, tekrar ekleme
      bool alreadySent = false;
      for (auto &sentRecord : recordsToSend) {
        if (record.epc == sentRecord.epc && record.action == sentRecord.action) {
          alreadySent = true;
          break;
        }
      }

      if (!alreadySent) {
        // Giriş/Çıkış analizi
        record.action = (record.antennaID == 0) ? "IN" : "OUT";
        uniqueRecords.push_back(record);
        if (std::find_if(recordsToSend.begin(), recordsToSend.end(), [&](const EPCRecord &r) {
              return r.epc == record.epc && r.antennaID == record.antennaID;
            })
            == recordsToSend.end()) {
          recordsToSend.push_back(record);  // Eğer aynı EPC daha önce gönderilmediyse listeye ekle
        }
      }
    }
  }
  epcRecords = uniqueRecords;
}


void sendEPCData() {
  if (recordsToSend.empty()) return;

  //WiFiClient client;
  HTTPClient https;

  if (WiFi.status() == WL_CONNECTED) {
    https.begin(client, serverURL);
    https.addHeader("Content-Type", "application/json");

    for (auto &record : recordsToSend) {
      String jsonPayload = "{\"epc\":\"" + String(record.epc.c_str()) + "\",\"antenna\":" + String(record.antennaID) + ",\"timestamp\":" + String(record.timestamp) + ",\"action\":\"" + String(record.action.c_str()) + "\"}";

      // HTTP POST işlemi
      int httpResponseCode = https.POST(jsonPayload);

      if (httpResponseCode > 0) {
        // Başarılı gönderim
        Serial.println("Data sent successfully: " + String(httpResponseCode));
        Serial.println("Response Payload: " + https.getString());
      } else {
        // Hata durumunda log
        Serial.println("Error in sending data: " + String(httpResponseCode));
        Serial.println("Retrying after 1 second...");
        delay(1000);  // Yeniden denemeden önce bekle
      }
    }
    recordsToSend.clear();  // Gönderilen verileri temizle
    https.end();
  } else {
    Serial.println("WiFi Disconnected");
    delay(5000);  // Wi-Fi bağlantısını kontrol etmek için 5 saniye bekle
  }
}
