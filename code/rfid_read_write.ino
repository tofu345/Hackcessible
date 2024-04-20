// https://circuitdigest.com/microcontroller-projects/interfacing-rfid-reader-module-with-arduino

#include <MFRC522.h>
#include <SPI.h>
#define RESET_PIN 9 // reset pin
#define RFID_SS_PIN 10 // SS or the slave select pin

MFRC522 rfid(RFID_SS_PIN, RESET_PIN);
MFRC522::MIFARE_Key rfid_key; // stores the card information

int writeBlock(MFRC522 *rfid, int blockNumber, byte data[]);
int readBlock(MFRC522 *rfid, int blockNumber, byte data[]);

void setup() {
    Serial.begin(115200);
    SPI.begin();
    rfid.PCD_Init();

    Serial.println("Scan a MIFARE Classic card");
    for (byte i = 0; i < 6; i++) {
        // Prepare the security key for the read and write operations.
        rfid_key.keyByte[i] = 0xFF;
    }
}

void loop() {
    if (!rfid.PICC_IsNewCardPresent() && !rfid.PICC_ReadCardSerial()) {
        return;
    }

    Serial.println("Card detected. Writing data");
    byte data1[15] = {"Circuit-Digest"};
    byte data2[13] = {"Jobit-Joseph"};
    writeBlock(&rfid, 1, data1);
    writeBlock(&rfid, 2, data2);

    Serial.println("Reading data from the tag");
    byte readData1[18];
    int err = readBlock(&rfid, 1, readData1);
    if (err != 0) {
        return;
    }

    Serial.print("Read block 1: ");
    for (int j = 0; j < 14; j++) {
        Serial.write(readData1[j]);
    }
    Serial.println();

    byte readData2[18];
    err = readBlock(&rfid, 2, readData2);
    if (err != 0) {
        return;
    }

    Serial.print("Read block 2: ");
    for (int j = 0; j < 12; j++) {
        Serial.write(readData2[j]);
    }
    Serial.println();

    // uncomment to see the entire memory dump.
    // rfid.PICC_DumpToSerial(&(rfid.uid));
}

int writeBlock(MFRC522 *rfid, int blockNumber, byte data[]) {
    int largestModulo4Number = blockNumber / 4 * 4;
    int trailerBlock = largestModulo4Number + 3;
    if (blockNumber > 2 && (blockNumber + 1) % 4 == 0) {
        Serial.print(blockNumber);
        Serial.println(" is a trailer block: Error");
        return 2;
    }

    MFRC522::StatusCode status = rfid->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                        trailerBlock, &rfid_key, &(rfid->uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Authentication failed: ");
        Serial.println(rfid->GetStatusCodeName(status));
        return 3;
    }

    // writing data to the block
    status = rfid->MIFARE_Write(blockNumber, data, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Data write failed: ");
        Serial.println(rfid->GetStatusCodeName(status));
        return 4;
    }
    Serial.print("Data written to block ");
    Serial.println(blockNumber);
    return 0;
}

int readBlock(MFRC522 *rfid, int blockNumber, byte data[]) {
    int largestModulo4Number = blockNumber / 4 * 4;
    int trailerBlock = largestModulo4Number + 3;
    MFRC522::StatusCode status = rfid->PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                        trailerBlock, &rfid_key, &(rfid->uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Authentication failed : ");
        Serial.println(rfid->GetStatusCodeName(status));
        return 3;
    }

    // reading data from the block
    byte buffersize = 18;
    // MIFARE_Read requires a pointer to buffersize
    status = rfid->MIFARE_Read(blockNumber, data, &buffersize);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Data read failed: ");
        Serial.println(rfid->GetStatusCodeName(status));
        return 4;
    }

    // Serial.println("Data read successfully");
    return 0;
}
