#include <Arduino.h>

#define FILM_DIR_PIN 4
#define FILM_STEP_PIN 5
#define FILM_SPEED_CONTROLLER_PIN A0

int gFilmStepMicros = 200;
bool gStop = false;

#define CONTROLLER_TRASH_HOLD 10

#define SPEED_UPDATE_TIME 100

#define SHOT_INDICATOR_PIN A2
#define SHOT_INDICATOR_TRIGGER_VALUE 500
#define MIN_INDICATOR_DELAY 20

#define SHOT_PIN 6
#define SHOT_TIME 100

void setup() {
    ADCSRA = (ADCSRA & 0xf8) | 0x04;
    pinMode(FILM_DIR_PIN, OUTPUT);
    pinMode(FILM_STEP_PIN, OUTPUT);
    pinMode(FILM_SPEED_CONTROLLER_PIN, INPUT);

    pinMode(SHOT_INDICATOR_PIN, INPUT);
    pinMode(SHOT_PIN, OUTPUT);

    digitalWrite(FILM_DIR_PIN, HIGH);

    digitalWrite(SHOT_PIN, HIGH);
}

inline unsigned getStepMicros(uint8_t pin) {
    static int gPrevVal = -100;

    int val = analogRead(pin);

    if (abs(gPrevVal - val) < CONTROLLER_TRASH_HOLD)
        val = gPrevVal;
    else
        gPrevVal = val;

    gStop = val > 1024 - 20;

    if (val <= 256)
        return map(val, 0, 256, 30, 80);

    if (val <= 512)
        return map(val, 256, 512, 80, 200);

    return map(val, 512, 1024, 200, 1000);
}

inline void readSpeed() {
    uint32_t currentTime = millis();
    static uint32_t gSpeedUpdateTimer = millis();

    if (currentTime - gSpeedUpdateTimer <= SPEED_UPDATE_TIME)
        return;

    // smooth speed up
    int newDelay = getStepMicros(FILM_SPEED_CONTROLLER_PIN);
    if (newDelay < 200 && gFilmStepMicros - newDelay > 5)
        if (gFilmStepMicros > 200)
            gFilmStepMicros = 200;
        else
            gFilmStepMicros = gFilmStepMicros - 5;
    else
        gFilmStepMicros = newDelay;

    gSpeedUpdateTimer = currentTime;
}

inline void checkShot() {
    uint32_t currentTime = millis();
    static uint32_t gLastIndicatorTime = currentTime;
    static uint32_t gLastShotTime = currentTime;
    static bool gLastIndicatorValue = false;
    static bool gInShot = false;

    if (currentTime - gLastShotTime < SHOT_TIME)
        return;

    if (gInShot == true) {
        gInShot = false;
        digitalWrite(SHOT_PIN, HIGH);
    }

    if (currentTime - gLastIndicatorTime < MIN_INDICATOR_DELAY)
        return;

    gLastIndicatorTime = currentTime;

    bool val = analogRead(SHOT_INDICATOR_PIN) < SHOT_INDICATOR_TRIGGER_VALUE;

    if (gLastIndicatorValue != val) {
        if (gLastIndicatorValue == false) {
            gInShot = true;
            gLastShotTime = currentTime;
            digitalWrite(SHOT_PIN, LOW);
        }

        gLastIndicatorValue = val;
    }
}

inline void moveEngine() {
    uint32_t currentTime = micros();
    static uint32_t gEngineTimer = currentTime;

    if ((currentTime - gEngineTimer) > (unsigned)gFilmStepMicros) {
        digitalWrite(FILM_STEP_PIN, HIGH);
        digitalWrite(FILM_STEP_PIN, LOW);
        gEngineTimer = currentTime;
    }
}

void loop() {
    readSpeed();
    if (gStop)
        return;

    checkShot();
    moveEngine();
}
