# LostAndNeverFound Setup Guide

I've helped get the setup started for you! This guide will walk you through running the full system. Here's a breakdown of what we need to run: the **Find My Apple Back-End (Docker)** and the **Arduino Firmware (ESP32)**.

I have already run the key-generation script (`generate_keys.py`) for you and placed the necessary key files into your project:
- The `findmy_key.h` C-header was copied to your `tracker_main/` and `checkpoint/` folders.
- The `LNF_devices.json` was generated and moved to `backend/data/`.

Here are the step-by-step instructions to get everything completely up and running.

---

## 1. Setting up the Apple Find My Backend

The backend fetches location reports for your tracker. It uses Docker to run `anisette-v3` and `macless-haystack`. 

> [!IMPORTANT]
> **Docker Desktop is not currently fully installed or available in your terminal.** You need to install Docker Desktop for Windows to proceed.

### Steps:
1. Download and install [Docker Desktop for Windows](https://www.docker.com/products/docker-desktop/).
2. Start Docker Desktop and ensure the Docker Engine is running on your machine.
3. You need to configure your Apple Credentials in the `docker-compose.yml` file. 
   - Open `backend/docker-compose.yml`
   - Find the `environment:` section and update your Apple ID credentials:
     ```yaml
     environment:
       - ANISETTE_URL=http://anisette:6969
       - APPLE_MAIL=your_apple_id@icloud.com
       - APPLE_PASS=your-app-specific-password
     ```
   - **Note:** It's recommended to create an **App-Specific Password** by going to your [Apple Account](https://appleid.apple.com), rather than using your main Apple password.
4. Open a PowerShell/Terminal window inside the `backend` folder and run:
   ```bash
   docker compose up -d
   ```
5. You can view the web UI by visiting [http://localhost:6176](http://localhost:6176) in your browser.

---

## 2. Flashing the Tracker (ESP32 Firmware)

Now that the keys are in place, you can upload the code to your physical ESP32 Hardware.

### Steps:
1. Open up the Arduino IDE. 
2. Make sure you have the ESP32 Board packages installed (Preferences → Additional Boards Manager URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`, then install 'esp32' via Boards Manager).
3. Make sure the **ArduinoJson** library is installed (Sketch → Include Library → Manage Libraries → search "ArduinoJson").
4. Open the `tracker_main/tracker_main.ino` sketch in Arduino IDE.
5. In the sketch, update the `CONFIG` section with your actual **WiFi credentials** and **Firebase Configuration** (as outlined in the README).
6. Connect your ESP32 board via USB.
7. Select **ESP32 Dev Module** from the Boards menu, and ensure the Port is selected. Set the Baud Rate to `115200`.
8. Hit **Upload** to flash the code to your device!

> [!TIP]
> If you encounter SPIFFS mount errors in the serial terminal, you may need to format the flash via `Tools → Erase All Flash Before Sketch Upload`.

---

## 3. Viewing it in the Haystack App

Finally, to view your tag on an Android device:
1. Download the [Haystack app](https://github.com/dchristl/macless-haystack) on your Android device.
2. Send the file `LNF_devices.json` (located in your main folder root or `backend/data/` folder) to your Android device. 
   *(**CRITICAL**: Keep this JSON and the `LNF_private.key` file in your root folder secure and never upload them online!)*
3. Import the JSON file in the Haystack app to start viewing locations polled from your backend.

Let me know once you have Docker installed and if you want assistance configuring the Apple parameters or debugging the Arduino uploads!
