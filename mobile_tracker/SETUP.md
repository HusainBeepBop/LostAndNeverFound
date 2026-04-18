# BLE Hot or Cold Tracker - Setup Guide

## Quick Start Guide

### Prerequisites
- Node.js v14+ (check with `node --version`)
- npm or yarn
- Expo CLI (install with `npm install -g expo-cli`)

### Step 1: Install Dependencies

```bash
cd mobile_tracker
npm install
```

If you encounter issues with native modules, try:
```bash
npm install --legacy-peer-deps
```

### Step 2: Configure Your BLE Device

Edit [app/config.ts](app/config.ts) and update the `TARGET_SERVICE_UUID`:

```typescript
export const TARGET_SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
```

Replace with your device's advertised Service UUID.

### Step 3: Adjust RSSI Thresholds (Optional)

In [app/config.ts](app/config.ts), modify the `RSSI_THRESHOLDS`:

```typescript
export const RSSI_THRESHOLDS: RssiThresholds = {
  cold: -80,  // RSSI < -80 dBm
  warm: -60,  // -80 dBm < RSSI < -60 dBm
  hot: 0,     // RSSI > -60 dBm
};
```

**Understanding RSSI Values:**
- -30 to -70 dBm: Device is very close (1-2 meters)
- -70 to -100 dBm: Device is at medium distance (2-10 meters)
- -100 to -120 dBm: Device is far (10-30+ meters)

### Step 4: Start the Development Server

```bash
npm start
```

You should see a QR code in the terminal.

### Step 5: Run on Your Device

#### Option A: Expo Go App (Easiest)

1. Download Expo Go from App Store (iOS) or Play Store (Android)
2. Scan the QR code displayed in the terminal
3. The app will load in Expo Go

#### Option B: Development Build

```bash
# For iOS
npm run ios

# For Android
npm run android
```

#### Option C: Web Preview (BLE won't work)

```bash
npm run web
```

## Platform-Specific Setup

### iOS Setup

1. **System Requirements:**
   - macOS with Xcode installed
   - iOS 13.0+

2. **First Time Build:**
   ```bash
   npm run ios
   ```

3. **Permissions:**
   - The app will request Bluetooth and Location permissions
   - Accept all permissions when prompted

4. **Troubleshooting:**
   - If build fails, try: `sudo gem install cocoapods` and `cd ios && pod install && cd ..`
   - Clear cache: `expo start -c`

### Android Setup

1. **System Requirements:**
   - Android Studio or Android SDK
   - Android SDK level 21+ (API 21)
   - Android Gradle Plugin 7.0+

2. **First Time Build:**
   ```bash
   npm run android
   ```

3. **Permissions:**
   - On Android 12+, the app will request:
     - BLUETOOTH_SCAN
     - BLUETOOTH_CONNECT
     - ACCESS_FINE_LOCATION
   - Accept all permissions when prompted

4. **Troubleshooting:**
   - Ensure Android emulator is running: `emulator -avd <avd_name>`
   - Or connect a physical Android device with USB debugging enabled
   - Clear Gradle cache: `cd android && ./gradlew clean && cd ..`

## Understanding the App

### Zones Explained

| Zone | RSSI Range | Color | Message | Distance |
|------|-----------|-------|---------|----------|
| Cold | < -80 dBm | 🔵 Blue | "Freezing" | > 10m |
| Warm | -80 to -60 dBm | 🟠 Orange | "Getting Warmer" | 2-10m |
| Hot | > -60 dBm | 🔴 Red | "Burning Hot!" | < 2m |

### File Structure

```
app/
├── BleTracker.tsx       # Main BLE scanning logic
├── ZoneDisplay.tsx      # UI components
├── config.ts            # Configuration constants
└── types.ts             # TypeScript type definitions
```

### Key Components

#### BleTracker.tsx
- Handles BLE initialization
- Manages permissions
- Scans for target device
- Updates RSSI values
- Calculates zones

#### ZoneDisplay.tsx
- Displays current zone with color
- Shows RSSI value
- Animates zone transitions
- Shows device status

## Testing with Real Device

### What You Need
- Your BLE device powered on and advertising the target Service UUID
- A smartphone with Bluetooth enabled
- Location services enabled (required for BLE scanning on Android/iOS)

### Testing Steps

1. **Start Scanning:**
   - Open the app
   - Verify it says "Searching for device..."

2. **Move Device Closer:**
   - As RSSI value increases (becomes less negative)
   - Screen color should change from blue → orange → red
   - Text should update accordingly

3. **Test All Zones:**
   - Move device far away (blue screen)
   - Move closer (orange screen)
   - Get very close (red screen)

## Customization Examples

### Change Zone Colors

Edit [app/config.ts](app/config.ts):

```typescript
export const ZONE_CONFIG = {
  cold: {
    color: '#0000FF',        // Change to pure blue
    message: 'Very Cold',
    description: 'Far Away',
  },
  // ...
};
```

### Change Zone Thresholds

```typescript
export const RSSI_THRESHOLDS: RssiThresholds = {
  cold: -75,   // More sensitive
  warm: -50,   // Wider warm zone
  hot: 0,
};
```

### Adjust Animation Timing

```typescript
export const ANIMATION_TIMINGS = {
  colorTransition: 500,    // Slower color changes
  zoneChange: 600,         // Slower zone animations
  pulseAnimation: 800,
};
```

## Troubleshooting

### Device Not Found

**Solution:**
1. Verify Bluetooth is enabled on your phone
2. Check that your BLE device is powered on
3. Verify the Service UUID matches: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
4. Move closer to the device (within 10 meters)
5. Check Android location services are enabled

### Permission Errors

**Solution:**
1. iOS: Go to Settings → Privacy → Bluetooth, and enable for this app
2. Android: Go to Settings → Apps → [App Name] → Permissions, enable all
3. Uninstall and reinstall the app
4. Restart your phone

### App Crashes on Startup

**Solution:**
```bash
# Clear cache and restart
expo start -c

# Or rebuild completely
rm -rf node_modules
npm install
npm start
```

### RSSI Not Updating

**Solution:**
1. Ensure device is still powered on
2. Ensure device is advertising the correct Service UUID
3. Try moving the device slightly
4. Restart the app
5. Check phone Bluetooth settings

## Production Build

### Building for iOS

```bash
# Option 1: Using EAS Build (Recommended)
eas build --platform ios

# Option 2: Local build (requires Xcode)
expo prebuild --clean
cd ios
xcodebuild -workspace LostAndNeverFound.xcworkspace -scheme LostAndNeverFound -configuration Release
```

### Building for Android

```bash
# Option 1: Using EAS Build (Recommended)
eas build --platform android

# Option 2: Local build
expo prebuild --clean
cd android
./gradlew assembleRelease
```

## Performance Optimization

- **Battery Usage:** The app scans intermittently; adjust `rescanInterval` in config.ts if needed
- **Memory:** Limit stored device history to recent entries
- **Scanning:** Reduce `scanDuration` for faster response (tradeoff: might miss devices)

## Next Steps

1. Integrate with your backend/cloud service
2. Add persistent data logging
3. Implement multiple device tracking
4. Add map view functionality
5. Create alerts/notifications when entering zones

## Resources

- Expo Documentation: https://docs.expo.dev
- React Native BLE Manager: https://github.com/innoveit/react-native-ble-manager
- Bluetooth RSSI: https://en.wikipedia.org/wiki/Received_signal_strength_indication
- BLE Specifications: https://www.bluetooth.com/specifications/

## Support

For issues:
1. Check the Troubleshooting section above
2. Review console logs: `expo start --verbose`
3. Search GitHub Issues: https://github.com/innoveit/react-native-ble-manager/issues
4. Check Expo Community: https://forums.expo.dev
