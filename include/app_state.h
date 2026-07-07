#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum ScreenId : uint8_t {
    SCREEN_WATCHFACE = 0,
    SCREEN_HEALTH = 1,
    SCREEN_SOS = 2,
    SCREEN_COUNT = 3
};

struct AppState {
    float heartRateBpm;
    bool heartRateValid;
    bool heartRateAlert;

    float accelMagnitudeG;
    bool rapidMotionAlert;

    bool sosTriggered;
    bool sosManual;

    ScreenId currentScreen;

    bool wifiConnected;
    bool mqttConnected;

    bool publishNow;
};

void appStateInit();
AppState appStateSnapshot();

void appStateSetHeartRate(float bpm, bool valid, bool alert);
void appStateSetAccel(float magnitudeG, bool rapidMotionAlert);
void appStateClearRapidMotionAlert();
void appStateTriggerSos(bool manual);
void appStateSetScreen(ScreenId screen);
void appStateSetNetwork(bool wifiConnected, bool mqttConnected);
void appStateRequestPublish();
bool appStateConsumePublishRequest();

const char *screenName(ScreenId screen);
