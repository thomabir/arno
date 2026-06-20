# Arno: Waters my plants

My houseplants in Lechuza pots tend to dry out when I'm on holiday for more than a week.
I'm building a device that can measure when the water reservoir is dry, and pump in more from a bigger container.

Status: HomeKit MVP working on the device. Exposes a "Water now" valve (with auto-shutoff and overflow guard), a moisture reading for the bottom reservoir probe, and an overflow leak alarm for the top probe, all controllable from Apple Home.

Next steps: On-device auto-watering (water when the bottom probe is dry, with min-interval and overflow safety) so it runs unattended on holiday.

## Goal

I can go on holiday for three weeks, and my plants remain well hydrated.

## Implementation

- Two moisture sensors (SEN0205):
  - One on the bottom of the pot's reservoir, to detect when it has run dry
  - One near the top of the reservoir, to avoid accidentally overfilling
- A peristaltic pump with some tubing
- A big water container
- An IoT-convenient microcontroller (ESP32, on Adafruit ESP32-S3 Feather board), so I can monitor from my phone

## BOM (incomplete)

- 1x [Adafruit 5477 Feather ESP32-S3](https://www.adafruit.com/product/5477): 17.50$
- 2x [SEN0205 water sensor](https://www.digikey.ch/en/products/detail/dfrobot/SEN0205/6588614) ([datahsheet](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/2331/SEN0205_Web.pdf)): 7$
- 1x [Adafruit 1150 peristaltic pump with silicon tube](https://www.adafruit.com/product/1150): 24.95$
- 1x [Adafruit 2927 Motor FeatherWing](https://www.adafruit.com/product/2927): 19.95$

## Notes

- ESP32 programmed via PlatformIO
- Native HomeKit via the HomeSpan library (no bridge or cloud). WiFi credentials and pairing are set over the serial monitor using HomeSpan's setup CLI; the pairing code and QR are printed there on first boot.
- Earlier standalone hardware-test sketches are kept in [esp32/scratch/](esp32/scratch/) for reference.
