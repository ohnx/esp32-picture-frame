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
    esp_deep_sleep(time_hr * SECONDS_PER_HOUR * US_PER_SECOND);
    Serial.println("would have power napped here, but am not going to.");
    while(1) { delay(100); }
#undef US_PER_SECOND
#undef SECONDS_PER_HOUR
}

// we know we only need exactly 800*480/2 bytes to store the response data.
#define IMAGE_BUF_SIZE (EPD_7IN3F_WIDTH * EPD_7IN3F_HEIGHT / 4)
uint8_t *image_buf;

int fetch_latest_picture() {
    int r;
    HTTPClient http;

    // attempt to pull data from the server
    http.begin(picture_host);

    // Send HTTP GET request and check response code
    if (http.GET() == 200) {
        // because the server responds with binary data, we have to pull out the stream
        // ourselves and read from it (instead of using e.g. http.getString).
        // I'm not too sad about this.
        // to the furthest extent possible I want to pretend like I'm writing C here...
        NetworkClient *nc = http.getStreamPtr();
        // really we should be using [HTTPClient.writeToStream] or similar, but the sad
        // thing is that this allocates so much memory. we know exactly how much data
        // we'll want to read, so we don't really need to do anything crazy.
        for (int bytes_read = 0, n = 0; bytes_read < IMAGE_BUF_SIZE; bytes_read += n) {
            if ((n = nc->readBytes(&image_buf[bytes_read], IMAGE_BUF_SIZE - bytes_read)) <= 0) {
                // some sort of read error, give up
                goto bail;
            } else {
                // give some time to rx more data, i guess?
                delay(10);
            }
        }
        // should have rx'd all data successfully here now.
        r = 0;
    } else {
        r = -1;
    }

bail:
    // always free resources
    http.end();

    // return error code or ok depending on above
    return r;
}

// this function is called when we wake up.
void setup() {
    uint64_t attempts;

    // dev board uses 115200 baud
	Serial.begin(115200);

    Serial.println("attempting malloc, lol");
    Serial.flush();

    // this is so cursed, but idk how to static allocate memory on spi bus.
    image_buf = (uint8_t *)malloc(IMAGE_BUF_SIZE);
    if (image_buf) {
        Serial.println("okay doing work now");
    } else {
        Serial.println("failed to allocate memory. sad.");
        while(1) { delay(100); }
    }

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
            powernap_for_hours(8);
        }
    }

    // attempt to pull a new picture
    attempts = 0;
    while (fetch_latest_picture()) {
        Serial.println("re-attempting picture pull");

        delay(1000);
        attempts++;
        if (attempts >= 30) {
            // time out after 30 seconds of attempting to pull a picture once a second
            // try again in 8 hours
            powernap_for_hours(8);
        }
    }

    // disconnect from wifi while we paint the screen to reduce max current draw
    WiFi.disconnect(true, false, 100);

    // ready to blit the picture now!
    DEV_Module_Init();
    EPD_7IN3F_Init();

    EPD_7IN3F_DisplayPart(image_buf, 0, 0, 800, 240);
    // delay 1s to allow the picture to paint
    delay(1000);
    // we'll want to delay more probably. 
    while(!DEV_Digital_Read(EPD_BUSY_PIN)) {      //LOW: busy, HIGH: idle
        delay(100);
    }

    EPD_7IN3F_Sleep();
    free(image_buf);
}

void loop()
{
    // don't ever expect to get here
    powernap_for_hours(24);
}
