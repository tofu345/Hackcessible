// https://github.com/miguelbalboa/rfid/blob/master/examples/ReadUidMultiReader/ReadUidMultiReader.ino

#include <MFRC522.h>
#include <SPI.h>

#define RST_PIN 9
#define SS_1_PIN 10
#define SS_2_PIN 8

#define NUM_READERS 2

byte rfid_ss_pins[] = {SS_1_PIN, SS_2_PIN};
MFRC522 rfids[NUM_READERS];

void setup() {
    Serial.begin(9600);
    while (!Serial)
        ;

    SPI.begin(); // Init SPI bus
    for (int idx = 0; idx < NUM_READERS; idx++) {
        rfids[idx].PCD_Init(rfid_ss_pins[idx], RST_PIN);
        Serial.print("Reader ");
        Serial.print(idx);
        Serial.print(": ");
        rfids[idx].PCD_DumpVersionToSerial();
        Serial.println();
    }
}

void loop() {
    for (int idx = 0; idx < NUM_READERS; idx++) {
        if (!rfids[idx].PICC_IsNewCardPresent() &&
            !rfids[idx].PICC_ReadCardSerial()) {
            continue;
        }

        Serial.print("Reader ");
        Serial.print(idx + 1);

        Serial.print(" Card UID:");
        print_byte_array(rfids[idx].uid.uidByte, rfids[idx].uid.size);

        Serial.print(" PICC type: ");
        MFRC522::PICC_Type piccType =
            rfids[idx].PICC_GetType(rfids[idx].uid.sak);
        Serial.println(rfids[idx].PICC_GetTypeName(piccType));

        rfids[idx].PICC_HaltA();      // Halt PICC
        rfids[idx].PCD_StopCrypto1(); // Stop encryption on PCD
    }
}

void print_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
