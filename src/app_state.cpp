#include "app_state.h"

static AppState g_state{};
static SemaphoreHandle_t g_mutex = nullptr;

void appStateInit() {
    g_state.heartRateBpm = 0.0f;
    g_state.heartRateValid = false;
    g_state.heartRateAlert = false;
    g_state.accelMagnitudeG = 1.0f;
    g_state.rapidMotionAlert = false;
    g_state.sosTriggered = false;
    g_state.sosManual = false;
    g_state.currentScreen = SCREEN_WATCHFACE;
    g_state.wifiConnected = false;
    g_state.mqttConnected = false;
    g_state.publishNow = false;

    g_mutex = xSemaphoreCreateMutex();
}

AppState appStateSnapshot() {
    AppState copy{};
    if (xSemaphoreTake(g_mutex, portMAX_DELAY) == pdTRUE) {
        copy = g_state;
        xSemaphoreGive(g_mutex);
    }
    return copy;
}

void appStateSetHeartRate(float bpm, bool valid, bool alert) {
    if (xSemaphoreTake(g_mutex, portMAX_DELAY) == pdTRUE) {
        g_state.heartRateBpm = bpm;
        g_state.heartRateValid = valid;
        g_state.heartRateAlert = alert;
        xSemaphoreGive(g_mutex);
    }
}

void appStateSetAccel(float magnitudeG, bool rapidMotionAlert) {
    if (xSemaphoreTake(g_mutex, portMAX_DELAY) == pdTRUE) {
        g_state.accelMagnitudeG = magnitudeG;
        g_state.rapidMotionAlert = rapidMotionAlert;
        xSemaphoreGive(g_mutex);
    }
}

void appStateClearRapidMotionAlert() {
    if (xSemaphoreTake(g_mutex, portMAX_DELAY) == pdTRUE) {
        g_state.rapidMotionAlert = false;
        xSemaphoreGive(g_mutex);
    }
}

void appStateTriggerSos(bool manual) {
    if (xSemaphoreTake(g_mutex, portMAX_DELAY) == pdTRUE) {
        g_state.sosTriggered = true;
        g_state.sosManual = manual;
        g_state.currentScreen = SCREEN_SOS;
        g_state.publishNow = true;
        xSemaphoreGive(g_mutex);
    }
}

void appStateSetScreen(ScreenId screen) {
    if (xSemaphoreTake(g_mutex, portMAX_DELAY) == pdTRUE) {
        if (g_state.currentScreen != screen) {
            if (g_state.currentScreen == SCREEN_SOS) {
                g_state.sosTriggered = false;
                g_state.sosManual = false;
            }
            g_state.currentScreen = screen;
            g_state.publishNow = true;
        }
        xSemaphoreGive(g_mutex);
    }
}

void appStateSetNetwork(bool wifiConnected, bool mqttConnected) {
    if (xSemaphoreTake(g_mutex, portMAX_DELAY) == pdTRUE) {
        g_state.wifiConnected = wifiConnected;
        g_state.mqttConnected = mqttConnected;
        xSemaphoreGive(g_mutex);
    }
}

void appStateRequestPublish() {
    if (xSemaphoreTake(g_mutex, portMAX_DELAY) == pdTRUE) {
        g_state.publishNow = true;
        xSemaphoreGive(g_mutex);
    }
}

bool appStateConsumePublishRequest() {
    bool requested = false;
    if (xSemaphoreTake(g_mutex, portMAX_DELAY) == pdTRUE) {
        requested = g_state.publishNow;
        g_state.publishNow = false;
        xSemaphoreGive(g_mutex);
    }
    return requested;
}

const char *screenName(ScreenId screen) {
    switch (screen) {
    case SCREEN_WATCHFACE:
        return "watchface";
    case SCREEN_HEALTH:
        return "health";
    case SCREEN_SOS:
        return "sos";
    default:
        return "unknown";
    }
}
