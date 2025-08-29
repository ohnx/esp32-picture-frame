#include "DEV_Config.h"
#include "EPD.h"
#include <stdlib.h>
#include "WiFi.h"
#include <HTTPClient.h>

#include "config.h"

// deep sleep for some number of hours. silly wrapper around [esp_deep_sleep]
// just because i mostly think in hours for this application.
// thankfully we're a stateless app so we don't care about losing ram on deep sleep.
__attribute__((noreturn)) void powernap_for_hours(uint64_t time_hr) {
#define US_PER_SECOND       1000000
#define SECONDS_PER_HOUR    3600
    // be sure to put the EPD to sleep before we sleep!
    EPD_7IN3F_Sleep();

    esp_deep_sleep(time_hr * SECONDS_PER_HOUR * US_PER_SECOND);
#undef US_PER_SECOND
#undef SECONDS_PER_HOUR
}

// 1 nibble per pixel
#define IMAGE_SIZE_BYTES (EPD_7IN3F_WIDTH * EPD_7IN3F_HEIGHT / 2)

// need to fetch and blit concurrently because we don't have enough memory to store the whole image!
int fetch_and_blit_latest_picture() {
    int r = -1;
    uint32_t bytes_drawn = 0;
    HTTPClient http;

    // attempt to pull data from the server
    http.begin(picture_host);

    // Send HTTP GET request and check response code
    if (http.GET() == 200) {
        // to the furthest extent possible I want to pretend like I'm writing C here...
        NetworkClient *nc = http.getStreamPtr();

        // start drawing
        EPD_7IN3F_SendCommand(0x10);

        for (; bytes_drawn < IMAGE_SIZE_BYTES; bytes_drawn++) {
            uint8_t b = 0;
            r = nc->read(&b, 1);
            if (r == 1) {
                EPD_7IN3F_SendData(b);
            } else if (r == 0) {
                // connection isn't dead, we just don't have any data.
                // delay 1ms and maybe we'll have more?
                delay(1);
            } else {
                Serial.println("rx from nc failed for some reason?");
                Serial.print("buffer size: ");
                Serial.print(bytes_drawn);
                Serial.print("/");
                Serial.println(IMAGE_SIZE_BYTES);
                goto bail;
            }
        }

        // should have rx'd all data successfully here now.
        r = 0;
    }

bail:
    // always free resources
    http.end();

    // write out white for the remaining pixels, if applicable
    // honestly i don't know what happens if you try to write out image data multiple times in a row.
    // i feel like writing out the image data exactly twice while forgetting to ask for an update in
    // between happens, though? so hopefully this won't do something bad.
    for (; bytes_drawn < IMAGE_SIZE_BYTES; bytes_drawn++) {
        EPD_7IN3F_SendData(EPD_7IN3F_WHITE | (EPD_7IN3F_WHITE << 4));
    }

    // return error code or ok depending on above
    return r;
}

// this function is called when we wake up.
void setup() {
    uint64_t attempts;

    // dev board uses 115200 baud
	Serial.begin(115200);

    // try connecting to wifi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        attempts++;

        if (attempts >= 10*30) {
            // time out after 30 seconds of attempting to connect to the wifi
            // try again in 8 hours
            Serial.println("wifi timeout :(");
            powernap_for_hours(8);
        }
    }

    Serial.println("wifi connected!");

    // set up the e-paper module
    DEV_Module_Init();
    EPD_7IN3F_Init();

    // attempt to pull a new picture
    attempts = 0;
    while (fetch_and_blit_latest_picture()) {
        Serial.println("re-attempting picture pull");

        delay(1000);
        attempts++;
        if (attempts >= 30) {
            // time out after 30 seconds of attempting to pull a picture once a second
            // try again in 8 hours
            powernap_for_hours(8);
        }
    }

    // this is the happy path. if we failed earlier, we'd have hit powernap_for_hours() and never returned.

    // this actually blits the image to the display
    EPD_7IN3F_TurnOnDisplay();
    // delay 1s to allow the picture to paint
    delay(1000);
    // we'll want to delay more that just 1s probably. 
    while(!DEV_Digital_Read(EPD_BUSY_PIN)) {      //LOW: busy, HIGH: idle
        delay(100);
    }
    // put the display to sleep again once it's done blitting
    EPD_7IN3F_Sleep();
}

void loop()
{
    // sleep for a day. when we wake back up, expect to be in setup() again.
    powernap_for_hours(24);
}
