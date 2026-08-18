# Correct Posture Electronic

A wearable posture alarm that calibrates to you instead of to a textbook angle.

Most posture trackers decide in advance what a correct spine looks like and nag you toward it. Bodies differ, chairs differ, and the angle that feels fine at a desk is not the angle that feels fine on a sofa. So this device asks first. You sit the way you want to sit, press a button, and that becomes the reference. Everything after is measured as drift from your own baseline.

It runs on an M5Stack Core2 strapped to the sternum, reading trunk inclination off the onboard IMU.

## How it works

The Core2 boots, shows the instructions, beeps, and waits. Press any of the three buttons while sitting comfortably and the current roll angle is stored as the baseline.

From then on the loop samples the IMU every 10 ms and compares. Inside the tolerance window the screen is green. Outside it the screen turns red immediately, which is the cheap feedback: glance down, correct, done. Only if you hold the bad angle for more than five seconds does it escalate to a vibration pulse and a beep, then reset the timer and start counting again.

The tolerance window is 25 degrees below the baseline and 10 above (`TOLERANCE_BELOW` and `TOLERANCE_ABOVE` in the sketch). The five second delay is the part that makes it wearable. Without it the thing fires every time you reach for a mug.

Two implementation notes. The five second timer is the Core2's real-time clock, reset on every good reading, so "seconds elapsed" is just the RTC's own second counter. And the beep is 120 KB of raw 44.1 kHz PCM compiled into the binary as a byte array (`beep_audio.c`) and pushed straight out over I2S, which is blunt but it means no SD card and no decoder.

## Hardware

An M5Stack Core2 and a chest strap. The Core2 carries the IMU, the screen, the speaker and the vibration motor, so there is nothing to wire.

## Build and flash

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Add the M5Stack board package. In Preferences, set Additional Boards Manager URLs to `https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json`, then install `M5Stack` from Boards Manager.
3. Install the `M5Core2` library from Library Manager.
4. Select Tools > Board > M5Stack > M5Core2, pick the port, open `correctPostureElectronic/correctPostureElectronic.ino` and upload.

## Wearing it

Strap the device tightly across the chest, centred on the sternum. Switch on and follow the screen: sit the way you want to sit, and press a button after the beep to record that position. Green means you are inside the window. Red means you are outside it, and the buzz and beep follow if you stay there past five seconds. Switching the device off ends the session.

## Where it stands

A working prototype. I wore it and put it on a few other people to check the calibration step held up across different builds, which it did, but that was informal and nothing was logged. There is no data in this repo and no study behind it.

The obvious limits: it reads roll only, so it catches you slumping but not twisting; the baseline is fixed once set, so a genuine change of chair means a restart; and nothing is recorded, so you cannot look back at how a day went. Logging to the SD card would be the next thing worth doing.
