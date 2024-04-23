/*
 * Rfid Pins
 * ---------
 * SS     53 -- Should be connected to each rfid seperately
 *           -- everything else can be shared
 * RST    5
 * MOSI   51
 * MISO   50
 * SCK    52
 */

#include "SPI.h"
#include "TMRpcm.h"
#include <MFRC522.h>
#include <SPI.h>

#define SPEAKER_PIN 11
#define SD_ChipSelectPin 53

#define RST_PIN 5
#define SS_1_PIN 53
#define NUM_READERS 1

byte rfid_ss_pins[] = {SS_1_PIN};
MFRC522 rfids[NUM_READERS];
MFRC522::MIFARE_Key rfid_key;
TMRpcm audio;

char characters[26] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
};
int num_characters = 0;

void setup() {
    Serial.begin(9600);
    while (!Serial)
        ;

    SPI.begin();

    audio.speakerPin = SPEAKER_PIN;
    audio.setVolume(7);
    audio.quality(0);

    // automagically calculate number of characters
    num_characters = sizeof(characters) / sizeof(characters[0]);

    for (byte i = 0; i < 6; i++) {
        rfid_key.keyByte[i] = 0xFF;
    }

    for (int i = 0; i < NUM_READERS; i++) {
        rfids[i].PCD_Init(rfid_ss_pins[i], RST_PIN);

        Serial.print("Reader ");
        Serial.print(i + 1);
        Serial.print(": ");
        rfids[i].PCD_DumpVersionToSerial();
        Serial.println();
    }
}

void loop() {
    for (int i = 0; i < NUM_READERS; i++) {
        int idx = read_rfid_card_data(&rfids[i]);
        if (idx != -1) {
            Serial.print("Reader ");
            Serial.print(i + 1);
            Serial.print(" read value: ");
            Serial.println(characters[idx]);
        }
    }
}

// Tries to read data stored in the last bit of the rfid card
int read_rfid_card_data(MFRC522 *reader) {
    if (!reader->PICC_IsNewCardPresent())
        return -1;
    if (!reader->PICC_ReadCardSerial())
        return -1;

    MFRC522::PICC_Type piccType = reader->PICC_GetType(reader->uid.sak);
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K)
        return -1;

    byte buffer[18];
    byte blockAddr = 4;
    static const int buffer_size = 16;
    byte size = sizeof(buffer);
    byte trailerBlock = 7;

    // Authenticate with card
    MFRC522::StatusCode status =
        reader->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock,
                                 &rfid_key, &(reader->uid));
    if (status != MFRC522::STATUS_OK) {
        return -1;
    }

    // Read data on card to buffer
    status = reader->MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        return -1;
    }

    reader->PICC_HaltA();
    reader->PCD_StopCrypto1();

    int idx = buffer[buffer_size - 1];
    if (idx >= num_characters) {
        return -1;
    }
    return idx;
}
