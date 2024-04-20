#include "SD.h"
#include "SPI.h"
#include "TMRpcm.h"

#define SD_PIN 5
#define SPEAKER_PIN 4

// https://github.com/TMRh20/TMRpcm/wiki
TMRpcm audio;

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    audio.speakerPin = SPEAKER_PIN;
    audio.setVolume(4);
    audio.quality(0);

    if (!SD.begin(SD_PIN)) {
        Serial.println("SD card setup failed");
        while (1)
            ;
    }

    char filename[] = "yes.wav";
    audio.play(filename);
}

void loop() {}
