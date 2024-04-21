#include <MFRC522.h>
#include <SPI.h>

#define RST_PIN 5
#define SS_PIN 53

MFRC522 rfid_reader(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

char characters[3] = {'c', 'a', 't'};
int num_characters = 0;

void setup() {
    Serial.begin(9600);
    SPI.begin();

    rfid_reader.PCD_Init();
    delay(4);
    rfid_reader.PCD_DumpVersionToSerial();

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    num_characters = sizeof(characters) / sizeof(characters[0]);

    Serial.println("Scan a MIFARE Classic PICC to demonstrate read.");
    Serial.print("Using key (for A and B):");
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    Serial.println();
}

void loop() {
    if (!rfid_reader.PICC_IsNewCardPresent())
        return;
    if (!rfid_reader.PICC_ReadCardSerial())
        return;

    Serial.print("Card UID:");
    dump_byte_array(rfid_reader.uid.uidByte, rfid_reader.uid.size);
    Serial.println();

    Serial.print("PICC type: ");
    MFRC522::PICC_Type piccType = rfid_reader.PICC_GetType(rfid_reader.uid.sak);
    Serial.println(rfid_reader.PICC_GetTypeName(piccType));
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println("This sample only works with MIFARE Classic cards.");
        return;
    }

    byte sector = 1;
    byte blockAddr = 4;
    byte trailerBlock = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);

    Serial.println("Authenticating using key A...");
    status = (MFRC522::StatusCode)rfid_reader.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key,
        &(rfid_reader.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print("PCD_Authenticate() failed: ");
        Serial.println(rfid_reader.GetStatusCodeName(status));
        return;
    }

    Serial.print("Reading data from block ");
    Serial.print(blockAddr);
    Serial.println(" ...");
    status =
        (MFRC522::StatusCode)rfid_reader.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("MIFARE_Read() failed: ");
        Serial.println(rfid_reader.GetStatusCodeName(status));
    }

    Serial.print("Data in block ");
    Serial.print(blockAddr);
    Serial.println(":");
    dump_byte_array(buffer, 16);
    Serial.println();

    if (rfid_reader.uid.size == 0) {
        Serial.println("no data on card");
    } else {
        int idx = rfid_reader.uid.uidByte[rfid_reader.uid.size - 1];
        Serial.print("Value: ");
        Serial.println(characters[idx]);
    }

    rfid_reader.PICC_HaltA();
    rfid_reader.PCD_StopCrypto1();
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
