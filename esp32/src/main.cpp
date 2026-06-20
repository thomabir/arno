#include <Arduino.h>
#include <HomeSpan.h>
#include <Adafruit_MotorShield.h>

// Bottom-of-reservoir sensor (dry detect), existing wiring.
#define PIN_SENSOR_BOTTOM 5
// Top-of-reservoir sensor (overflow guard). Free GPIO, clear of the I2C SDA/SCL
// pins used by the Motor FeatherWing.
#define PIN_SENSOR_TOP 6

// Hard cap on a single watering, independent of the requested duration.
static const uint32_t MAX_RUNTIME_S = 60;

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor* pump = AFMS.getMotor(1);

// SEN0205 reads HIGH when the probe is wet.
static bool sensorWet(uint8_t pin) {
  return digitalRead(pin) == HIGH;
}

static void pumpOn() {
  pump->setSpeed(255);
  pump->run(FORWARD);
}

static void pumpOff() {
  pump->run(RELEASE);
}

// "Water now" valve with auto-shutoff and overflow safety.
struct PlantValve : Service::Valve {
  SpanCharacteristic* active;
  SpanCharacteristic* inUse;
  SpanCharacteristic* setDuration;
  SpanCharacteristic* remainingDuration;
  uint32_t endTime = 0;
  bool pumping = false;

  PlantValve() : Service::Valve() {
    active = new Characteristic::Active(0);
    inUse = new Characteristic::InUse(0);
    new Characteristic::ValveType(1);  // irrigation
    setDuration = new Characteristic::SetDuration(30);
    remainingDuration = new Characteristic::RemainingDuration(0);
  }

  boolean update() override {
    if (active->updated()) {
      if (active->getNewVal()) {
        if (pumping) return true;  // already running; ignore a repeated on-write
        if (sensorWet(PIN_SENSOR_TOP)) {
          Serial.println("Refusing to water: top reservoir sensor is wet.");
          WEBLOG("Refused watering: top reservoir wet");
          return false;
        }
        uint32_t dur = setDuration->getVal();
        if (dur == 0 || dur > MAX_RUNTIME_S) dur = MAX_RUNTIME_S;
        endTime = millis() + dur * 1000UL;
        pumping = true;
        inUse->setVal(1);
        remainingDuration->setVal(dur);
        pumpOn();
        Serial.printf("Watering for %u s\n", dur);
        WEBLOG("Watering started: %u s (manual)", dur);
      } else {
        stopWatering("stopped by user");
      }
    }
    return true;
  }

  void loop() override {
    if (!active->getVal()) return;

    if (sensorWet(PIN_SENSOR_TOP)) {
      Serial.println("Top sensor wet mid-run, stopping pump.");
      stopWatering("top reservoir wet mid-run");
      return;
    }
    if (millis() >= endTime) {
      Serial.println("Watering timer elapsed, stopping pump.");
      stopWatering("timer elapsed");
      return;
    }

    uint32_t remaining = (endTime - millis()) / 1000;
    if (remaining != (uint32_t)remainingDuration->getVal())
      remainingDuration->setVal(remaining);
  }

  void stopWatering(const char* reason) {
    if (!pumping) return;  // already stopped; avoid a duplicate log entry
    pumping = false;
    pumpOff();
    if (active->getVal()) active->setVal(0);
    if (inUse->getVal()) inUse->setVal(0);
    remainingDuration->setVal(0);
    WEBLOG("Watering stopped: %s", reason);
  }
};

// Overflow probe exposed as a HomeKit leak sensor: a wet top is a real overflow,
// so a critical leak alarm is the right behaviour here.
struct OverflowSensor : Service::LeakSensor {
  SpanCharacteristic* leak;
  uint8_t pin;
  uint32_t lastCheck = 0;

  OverflowSensor(uint8_t pin) : Service::LeakSensor(), pin(pin) {
    leak = new Characteristic::LeakDetected(0);
  }

  void loop() override {
    if (millis() - lastCheck < 1000) return;
    lastCheck = millis();
    bool wet = sensorWet(pin);
    if (wet != (bool)leak->getVal()) {
      leak->setVal(wet ? 1 : 0);
      WEBLOG("Top reservoir %s", wet ? "wet (overflow)" : "dry");
    }
  }
};

// Moisture probe exposed as a silent humidity reading (100% wet, 0% dry).
// Unlike a leak sensor this never raises a critical alarm, so routine watering
// wetting the probe does not notify the phone.
struct MoistureSensor : Service::HumiditySensor {
  SpanCharacteristic* level;
  uint8_t pin;
  uint32_t lastCheck = 0;

  MoistureSensor(uint8_t pin) : Service::HumiditySensor(), pin(pin) {
    level = new Characteristic::CurrentRelativeHumidity(0);
  }

  void loop() override {
    if (millis() - lastCheck < 1000) return;
    lastCheck = millis();
    float wet = sensorWet(pin) ? 100 : 0;
    if (wet != level->getVal<float>()) {
      level->setVal(wet);
      WEBLOG("Bottom reservoir %s", wet > 0 ? "wet" : "dry");
    }
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SENSOR_BOTTOM, INPUT);
  pinMode(PIN_SENSOR_TOP, INPUT);

  while (!AFMS.begin()) {
    Serial.println("Could not find Motor Shield. Check wiring.");
    delay(1000);
  }
  pumpOff();

  // Timestamped event log served on the local network at the URL printed below
  // on boot. RAM-only: the last 50 entries, cleared on reboot or power loss.
  homeSpan.enableWebLog(50, "pool.ntp.org", "CET-1CEST,M3.5.0,M10.5.0/3", "log");

  homeSpan.begin(Category::Bridges, "Arno");

  WEBLOG("Arno booted (bottom probe GPIO%d, top probe GPIO%d)",
         PIN_SENSOR_BOTTOM, PIN_SENSOR_TOP);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Arno");
      new Characteristic::Manufacturer("Arno");
      new Characteristic::Model("Arno MVP");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Water now");
    new PlantValve();

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Reservoir bottom");
    new MoistureSensor(PIN_SENSOR_BOTTOM);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Reservoir top");
    new OverflowSensor(PIN_SENSOR_TOP);
}

void loop() {
  homeSpan.poll();
}
