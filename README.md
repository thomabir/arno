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

## Setup

Step-by-step from bare board to a working accessory in Apple Home.

### 1. Wire the hardware

- Stack the Motor FeatherWing on the ESP32-S3 Feather.
- Pump: connect to motor port **M1** on the FeatherWing.
- Bottom reservoir probe (dry detect): signal to **GPIO 5**, power to 3V/GND.
- Top reservoir probe (overflow guard): signal to **GPIO 6**, power to 3V/GND.
- Put the pump's intake in the water container and the outlet into the pot.

### 2. Build and flash

Install [PlatformIO](https://platformio.org/), then from the `esp32/` directory:

```sh
pio run -t upload
```

(Or use the "Upload and Monitor" button in the VSCode platformio extension)

The upload port is set to `/dev/cu.usbmodem2101` in [esp32/platformio.ini](esp32/platformio.ini); adjust it if the board enumerates elsewhere.

### 3. Configure WiFi and pairing code (first boot)

Open the serial monitor, where HomeSpan's setup CLI runs:

```sh
pio device monitor -b 115200
```

- WiFi: type `W` and follow the prompts to enter the network name and password. The board reboots and connects, printing its IP address. Wifi credentials are stored onboard.
- HomeKit pairing code: an 8-digit code is needed to add the accessory. It defaults to `466-37-726`. To set your own, type `S` followed by 8 digits (e.g. `S 11122333`); overly simple codes (sequential or repeating digits) are rejected. The code is stored as one-way verification data and is never printed back, so record whatever you set. Change it later by setting a new one with `S`.

### 4. Pair in the Home app

On the iPhone: Home app → **+** → Add Accessory → **More options** → select **Arno** → enter the 8-digit code from the previous step. (No QR is shown unless a Setup ID is configured with the `Q` CLI command.)

Four tiles appear: the **Arno** bridge, **Water now** (valve), **Reservoir bottom** (humidity reading), and **Reservoir top** (leak/overflow sensor).

### 5. Use it

- Tap **Water now**, optionally set a duration, and the pump runs until the timer elapses or the top probe detects overflow. It refuses to start if the top probe is already wet.
- **Reservoir bottom** shows whether water is detected as a "humidity" reading (either 0 or 100%), there is no alarm for that sensor; **Reservoir top** is a "leak detector" that alarms on overflow.
- With a home hub (HomePod/Apple TV) the valve and sensors are reachable remotely, e.g. while on holiday.

### 6. Check the event log

Watering and sensor events are timestamped and viewable in a browser on the same network at the `http://HomeSpan-[id].local/log` URL printed on the serial monitor at boot, or at `http://<device-ip>/log`. It is local-network only and held in RAM (last 50 entries, cleared on reboot or power loss).

Safari resolves the `.local` name. Firefox on macOS does not, and also needs Local Network permission under System Settings → Privacy & Security. A DHCP reservation for the board keeps the IP stable for bookmarking.

## Notes

- ESP32 programmed via PlatformIO.
- Native HomeKit via the HomeSpan library (no bridge or cloud).
- Earlier standalone hardware-test sketches are kept in [esp32/scratch/](esp32/scratch/) for reference.
