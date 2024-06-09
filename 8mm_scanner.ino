#define ENGINE_MAX_RPM 800 // rotation per minutes
#define ENGINE_SPR 200 // steps per revolution
#define ENGINE_MICROSTEPS 16
#define ENGINE_MPS (60000000 / ENGINE_SPR / ENGINE_MICROSTEPS / 2) // microseconds per step for 1RPM

#define CONTROLLER_MAX_VAL 1024.

#define FILM_DIR_PIN 4
#define FILM_STEP_PIN 5
#define FILM_SPEED_CONTROLLER_PIN A0

bool gLastFilmStep = false;
unsigned long gFilmTimer = 0;
unsigned long gFilmStepMicros = 0;

#define SONAR_TRIGGER_PIN  3
#define SONAR_ECHO_PIN     2
#define FILM_MAX_DISTANCE 30
#define FILM_TRIGGER_DISTANCE 5

#define SPEED_UPDATE_TIME 1000
static_assert(SPEED_UPDATE_TIME > 50);
unsigned long gSpeedUpdateTimer = 0;

#define SHOT_INDICATOR_PIN A2
#define SHOT_INDICATOR_TRIGGER_VALUE 450
#define MIN_INDICATOR_DELAY 300
unsigned long gLastIndicatorTime = 0;
bool gLastIndicatorValue = false;

#define SHOT_PIN 9
#define SHOT_TIME 100

void setup(){
  ADCSRA = (ADCSRA & 0xf8) | 0x04;
  pinMode(FILM_DIR_PIN, OUTPUT);
  pinMode(FILM_STEP_PIN, OUTPUT);
  pinMode(FILM_SPEED_CONTROLLER_PIN, INPUT);

  pinMode(SHOT_INDICATOR_PIN, INPUT);
  pinMode(SHOT_PIN, OUTPUT);

  digitalWrite(FILM_DIR_PIN, HIGH);

  digitalWrite(SHOT_PIN, HIGH);
  Serial.begin(9600);
}

inline unsigned long getStepMicros(uint8_t pin) {
  int val = analogRead(pin);
  int rpm = max(ENGINE_MAX_RPM * min(val, CONTROLLER_MAX_VAL) / CONTROLLER_MAX_VAL, 1);
  return ENGINE_MPS / rpm;
}

inline void readSpeed() {
  if (millis() - gSpeedUpdateTimer > SPEED_UPDATE_TIME) {
    gFilmStepMicros = getStepMicros(FILM_SPEED_CONTROLLER_PIN);

    gSpeedUpdateTimer = millis();
  }
}

inline void checkShot() {
  if ((millis() - gLastIndicatorTime) > SHOT_TIME)
    digitalWrite(SHOT_PIN, HIGH);

  bool val = analogRead(SHOT_INDICATOR_PIN) < SHOT_INDICATOR_TRIGGER_VALUE;

  if ((millis() - gLastIndicatorTime) > MIN_INDICATOR_DELAY && gLastIndicatorValue != val) {
    if (gLastIndicatorValue == false) {
      digitalWrite(SHOT_PIN, LOW);
      Serial.println("kek");
    }

    gLastIndicatorTime = millis();
    gLastIndicatorValue = !gLastIndicatorValue;
  }
}

inline void moveEngine() {
    if((unsigned long)(micros() - gFilmTimer) > gFilmStepMicros) {
    gLastFilmStep = !gLastFilmStep;
    digitalWrite(FILM_STEP_PIN, gLastFilmStep);
    gFilmTimer = micros();
  }
}

void loop(){
  readSpeed();
  checkShot();
  moveEngine();
}