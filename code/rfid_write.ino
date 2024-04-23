#include <MFRC522.h>
#include <SPI.h>

#define RST_PIN 5 // Configurable, see typical pin layout above
#define SS_PIN 53 // Configurable, see typical pin layout above

MFRC522 rfid_reader(SS_PIN, RST_PIN); // Create MFRC522 instance.
MFRC522::MIFARE_Key key;

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
    rfid_reader.PCD_Init();

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    num_characters = sizeof(characters) / sizeof(characters[0]);

    Serial.println("\nScan a card to write new data\n");

    // Serial.print("Using key (for A and B):");
    // dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    // Serial.println();
    // Serial.println("BEWARE: Data will be written to the PICC, in sector #1");

    Serial.println("-----------------------------");
    Serial.println("start: write current");
    Serial.println("next: write next");
    Serial.println("res: start from first char");
    Serial.println("-----------------------------");
}

void loop() {
    static byte current_char_idx = 0;

    Serial.print("Preparing to write: ");
    Serial.println(characters[current_char_idx]);
    Serial.println("Waiting for input...");

    // wait until user enters something
    while (Serial.available() <= 0)
        ;

    String input = Serial.readString();
    input.trim();

    if (input == "s" || input == "start") {
        Serial.println();
    } else if (input == "n" || input == "next") {
        current_char_idx++;
        if ((int)current_char_idx > num_characters) {
            Serial.println("Reached last character, going back to first");
            current_char_idx = 0;
        }

        Serial.print("Preparing to write: ");
        Serial.println(characters[current_char_idx]);
    } else if (input == "r" || input == "restart") {
        current_char_idx = 0;
        Serial.println("Restarting from first character");
    } else {
        return;
    }

    Serial.println("Waiting for card...");

    while (1) {
        if (Serial.available() > 0) {
            String input = Serial.readString();
            input.trim();
            if (input == "q" || input == "abort") {
                Serial.println("Write aborted");
                break;
            }
        }

        int ok = write_data_to_card(&rfid_reader, current_char_idx);
        if (ok == 0) {
            break;
        }
    }
}

int write_data_to_card(MFRC522 *reader, int current_char_idx) {
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
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, (byte)current_char_idx};
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
    // dump_byte_array(dataToWrite, 16);
    // Serial.print(" ");
    Serial.println(characters[current_char_idx]);

    // write to card
    status = reader->MIFARE_Write(blockAddr, dataToWrite, 16);
    if (status != MFRC522::STATUS_OK) {
        return -1;
    }

    // read and check if data matches
    status = reader->MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        return -1;
    }

    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        if (buffer[i] == dataToWrite[i])
            count++;
    }

    reader->PICC_HaltA();
    reader->PCD_StopCrypto1();

    if (count == 16) {
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
