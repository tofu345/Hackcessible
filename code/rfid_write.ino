#include <MFRC522.h>
#include <SPI.h>

#define RST_PIN 5
#define SS_PIN 53

MFRC522 rfid_reader(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
    Serial.begin(9600);
    while (!Serial)
        ;
    SPI.begin();
    rfid_reader.PCD_Init();

    // TODO: create a secure key
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println("\nWrite characters to rfid card");
}

void loop() {
    char current_char = 'a';

    while (1) {
        Serial.print("Waiting for card to write ");
        Serial.println(current_char);
        Serial.println("Enter a different character to write it instead");

        while (1) {
            if (Serial.available() > 0) {
                char input = Serial.read();
                if (input < 'a' || input > 'z') {
                    Serial.println("Invalid character");
                    break;
                }

                current_char = input;
            }

            if (write_data_to_card(&rfid_reader, current_char) == 0) {
                Serial.println("Card write success!");
            } else {
                Serial.println("Card write failed");
                break;
            }
        }

        current_char++;
        if (current_char > 'z') {
            current_char = 'a';
            Serial.println("You've reached the end\n");
        }
    }
}

int write_data_to_card(MFRC522 *reader, int current_char) {
    if (!reader->PICC_IsNewCardPresent())
        return -1;
    if (!reader->PICC_ReadCardSerial())
        return -1;

    MFRC522::StatusCode status;
    MFRC522::PICC_Type piccType = reader->PICC_GetType(reader->uid.sak);

    // check card type
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        return -1;
    }

    byte sector = 1;
    byte blockAddr = 4;
    byte dataToWrite[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, (byte)current_char
    };
    int dataToWriteSize = sizeof(dataToWrite) / sizeof(dataToWrite[0]);
    byte trailerBlock = 7;
    byte buffer[18];
    byte size = sizeof(buffer);

    // authenticate card
    status = reader->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B,
                                      trailerBlock, &key, &(reader->uid));
    if (status != MFRC522::STATUS_OK) {
        return -1;
    }

    Serial.print("Writing char: ");
    // dump_byte_array(dataToWrite, dataToWriteSize);
    // Serial.print(" ");
    Serial.println(current_char);

    // write to card
    status = reader->MIFARE_Write(blockAddr, dataToWrite, dataToWriteSize);
    if (status != MFRC522::STATUS_OK) {
        return -1;
    }

    // read and check if data matches
    status = reader->MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        return -1;
    }

    byte count = 0;
    for (byte i = 0; i < dataToWriteSize; i++) {
        if (buffer[i] == dataToWrite[i])
            count++;
    }

    reader->PICC_HaltA();
    reader->PCD_StopCrypto1();

    if (count == dataToWriteSize) {
        Serial.println("Data write success :-)\n");
        return 0;
    } else {
        Serial.println("Failure :-(\n");
        return -1;
    }
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
