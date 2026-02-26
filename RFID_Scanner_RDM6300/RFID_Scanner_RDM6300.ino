#include <HardwareSerial.h>

#define RXD2 16
#define TXD2 17
#define BUZZER 5

HardwareSerial rfidSerial(1);

#define RFID_DATA_SIZE 14
#define CARD_TIMEOUT 1500  // ms with no data = card removed, adjust if needed

void setup() {
  Serial.begin(115200);
  rfidSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  Serial.println("RFID Reader Ready...");
}

void beep(int duration = 150) {
  digitalWrite(BUZZER, HIGH);
  delay(duration);
  digitalWrite(BUZZER, LOW);
}

byte hexCharToByte(byte c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

String lastCardID = "";
unsigned long lastSeenTime = 0;

void loop() {
  // If card hasn't been seen for CARD_TIMEOUT ms, reset so it can be re-read
  if (lastCardID != "" && (millis() - lastSeenTime > CARD_TIMEOUT)) {
    Serial.println("Card removed. Ready for next scan.");
    lastCardID = "";
  }

  if (!rfidSerial.available()) return;

  byte firstByte = rfidSerial.peek();
  if (firstByte != 0x02) {
    rfidSerial.read();
    return;
  }

  if (rfidSerial.available() < RFID_DATA_SIZE) return;

  byte data[RFID_DATA_SIZE];
  rfidSerial.readBytes(data, RFID_DATA_SIZE);

  if (data[0] != 0x02 || data[13] != 0x03) {
    Serial.println("Frame error - bad start/stop byte");
    return;
  }

  // Verify checksum
  byte checksum = 0;
  for (int i = 1; i <= 10; i += 2) {
    byte b = (hexCharToByte(data[i]) << 4) | hexCharToByte(data[i + 1]);
    checksum ^= b;
  }
  byte receivedChecksum = (hexCharToByte(data[11]) << 4) | hexCharToByte(data[12]);

  if (checksum != receivedChecksum) {
    while (rfidSerial.available()) rfidSerial.read();
    return;
  }

  // Build card ID string
  char cardID[11];
  for (int i = 0; i < 10; i++) cardID[i] = data[i + 1];
  cardID[10] = '\0';
  String cardIDStr = String(cardID);

  // Only trigger if it's a NEW card or card was removed and re-presented
  if (cardIDStr != lastCardID) {
    lastCardID = cardIDStr;
    Serial.print("✅ Card ID: ");
    Serial.println(cardIDStr);
    beep(150);
  }

  // Update last seen time every read (keeps timeout from resetting while card is held)
  lastSeenTime = millis();

  while (rfidSerial.available()) rfidSerial.read();
}