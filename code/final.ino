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
#include <SD.h>
#include <SPI.h>
#include <string.h>

#define SD_CS_PIN 49 // Chip select pin for SD card
#define SPEAKER_PIN 5

#define RST_PIN 5
#define RFID_1_SS_PIN 53
#define RFID_2_SS_PIN 7
#define NUM_READERS 2

byte rfid_ss_pins[] = {RFID_1_SS_PIN, RFID_2_SS_PIN};
MFRC522 rfids[NUM_READERS];
MFRC522::MIFARE_Key rfid_key;
unsigned long rfid_prev_read_times[NUM_READERS];

TMRpcm audio;
char letters[4] = "---";

unsigned long timeStamp = 0;
unsigned long elapsedTime = 0;
unsigned long cardSampleRate = 1000; // milliseconds
unsigned long cardTimeout = 1500;    // milliseconds

void setup() {
    Serial.begin(9600);
    while (!Serial)
        ;

    SPI.begin();

    // if (!SD.begin(SD_CS_PIN)) {
    //     Serial.println("SD card initialization failed");
    //     while (1)
    //         ;
    // }

    // audio.speakerPin = SPEAKER_PIN;
    // audio.setVolume(7);
    // audio.quality(0);

    for (byte i = 0; i < 6; i++) {
        rfid_key.keyByte[i] = 0xFF;
    }

    Serial.println("if firmware version == (unknown) check connections and "
                   "pins then restart");

    for (int i = 0; i < NUM_READERS; i++) {
        rfid_prev_read_times[i] = 0;
        rfids[i].PCD_Init(rfid_ss_pins[i], RST_PIN);

        Serial.print("Reader ");
        Serial.print(i + 1);
        Serial.print(": ");
        rfids[i].PCD_DumpVersionToSerial();
        Serial.println();
    }
}

void loop() {
    // read from all rfid readers every cardSampleRate ms
    elapsedTime = millis() - timeStamp;
    if (elapsedTime > cardSampleRate) {
        for (int i = 0; i < NUM_READERS; i++) {
            int idx = read_rfid_card_data(&rfids[i]);
            if (idx == -1) {
                // wait cardTimeout ms after card is no longer detected before
                // removing letter from letters
                if (millis() - rfid_prev_read_times[i] > cardTimeout) {
                    letters[i] = '-';
                }
                continue;
            }
            rfid_prev_read_times[i] = millis();

            if (letters[i] == '-') {
                // play letter sound the first time the letter is placed
                // static char filename[] = " .wav";
                // filename[0] = letters[i];
                // audio.play(filename);
            }
            letters[i] = (char)(idx + 97);
        }

        Serial.println(letters);

        timeStamp = millis();
    }

    // play letters (word) sound
    if (letters[0] != '-' && letters[1] != '-' && letters[2] != '-') {
        // char filename[8];
        // sprintf(filename, "%s.wav", letters);
        // if (!SD.exists(filename)) {
        //     return;
        // }
        //
        // audio.play(filename);
    }
}

// Tries to read data stored in the last byte of the rfid card
// returns -1 if read failed or card removed
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

    // reader->PICC_HaltA(); // commenting this makes it read a card only once
    reader->PCD_StopCrypto1();

    int idx = buffer[buffer_size - 1];
    if (idx < 97 || idx > 122) {
        return -1;
    }
    return idx;
}
