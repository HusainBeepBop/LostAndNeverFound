# BLE Hot or Cold Tracker App

A React Native mobile app built with Expo that uses Bluetooth Low Energy (BLE) to track proximity to a specific device. The app displays proximity information through a "hot or cold" interface with RSSI (Received Signal Strength Indicator) values.

## Features

✨ **BLE Device Scanning**: Automatically scans for and tracks BLE devices with a specific Service UUID
📊 **Real-time RSSI Display**: Shows the signal strength in dBm
🌡️ **Three Temperature Zones**:
  - **Cold Zone** (RSSI < -80 dBm): Blue screen - "Freezing"
  - **Warm Zone** (-80 to -60 dBm): Orange screen - "Getting Warmer"
  - **Hot Zone** (RSSI > -60 dBm): Red screen - "Burning Hot!"

🎨 **Smooth Animations**: Color transitions and scaling animations when zone changes
📱 **Cross-Platform**: Works on both iOS and Android
🔐 **Permission Handling**: Automatically requests necessary Bluetooth and Location permissions

## Requirements

- **Node.js** (v14 or later)
- **npm** or **yarn**
- **Expo CLI**: Install with `npm install -g expo-cli`
- **iOS**: Xcode and CocoaPods (for iOS builds)
- **Android**: Android Studio or Android SDK

## Installation

### 1. Clone and Navigate

```bash
cd mobile_tracker
```

### 2. Install Dependencies

```bash
npm install
# or
yarn install
```

### 3. Start the Development Server

```bash
npm start
# or
expo start
```

### 4. Run on a Device or Emulator

**For iOS:**
```bash
npm run ios
```

**For Android:**
```bash
npm run android
```

**For Web (preview only, BLE not available):**
```bash
npm run web
```

## Project Structure

```
mobile_tracker/
├── App.tsx                 # Root component
├── app/
│   ├── BleTracker.tsx     # BLE scanning logic and permissions
│   └── ZoneDisplay.tsx    # UI component for displaying zones and RSSI
├── app.json               # Expo configuration
├── package.json           # Dependencies and scripts
├── tsconfig.json          # TypeScript configuration
├── babel.config.js        # Babel configuration
├── eas.json              # EAS Build configuration
└── README.md             # This file
```

## Configuration

### Target BLE Device

To scan for a different BLE device, modify the `TARGET_SERVICE_UUID` in [app/BleTracker.tsx](app/BleTracker.tsx):

```typescript
const TARGET_SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';
```

### RSSI Thresholds

Adjust the RSSI thresholds in the `ZONES` array in [app/BleTracker.tsx](app/BleTracker.tsx):

```typescript
const ZONES: Zone[] = [
  { name: 'Cold', text: 'Freezing', color: '#1E90FF', rssiRange: [-100, -80] },
  { name: 'Warm', text: 'Getting Warmer', color: '#FFA500', rssiRange: [-80, -60] },
  { name: 'Hot', text: 'Burning Hot!', color: '#DC143C', rssiRange: [-60, 0] },
];
```

### Colors

You can customize the zone colors in the `ZONES` array. Colors are in hex format:
- Cold: `#1E90FF` (Dodger Blue)
- Warm: `#FFA500` (Orange)
- Hot: `#DC143C` (Crimson)

## Permissions

### Android

Required permissions are defined in `app.json`:
- `android.permission.BLUETOOTH` - Basic Bluetooth access
- `android.permission.BLUETOOTH_ADMIN` - Bluetooth administration
- `android.permission.BLUETOOTH_SCAN` - BLE scanning (Android 12+)
- `android.permission.BLUETOOTH_CONNECT` - Connect to BLE devices (Android 12+)
- `android.permission.ACCESS_FINE_LOCATION` - Location for BLE scanning
- `android.permission.ACCESS_COARSE_LOCATION` - Coarse location for BLE

### iOS

Required permissions in `Info.plist`:
- `NSBluetoothPeripheralUsageDescription` - Peripheral Bluetooth usage
- `NSBluetoothCentralUsageDescription` - Central Bluetooth usage
- `NSLocationWhenInUseUsageDescription` - Location usage

The app will request these permissions automatically on first launch.

## How It Works

### 1. Permission Request
On app launch, it requests all necessary Bluetooth and Location permissions.

### 2. BLE Initialization
Once permissions are granted, `BleManager` is initialized and starts scanning.

### 3. Device Discovery
The app scans for BLE devices broadcasting the target Service UUID.

### 4. RSSI Monitoring
When a device is found, the app continuously monitors its RSSI value.

### 5. Zone Calculation
Based on the RSSI value, the appropriate zone is determined and the UI updates accordingly.

### 6. Animation
Smooth color transitions and scale animations provide visual feedback.

## Usage Tips

1. **Ensure Bluetooth is Enabled**: The device must have Bluetooth turned on.
2. **Check Permissions**: Make sure the app has been granted all required permissions.
3. **Get Close to Device**: The app needs to be in range of the BLE device (typically 30-100 meters depending on environment).
4. **Test Device**: Ensure your BLE device is advertising with the correct Service UUID.

## Troubleshooting

### Device Not Found
- Ensure the target BLE device is powered on and advertising
- Verify the Service UUID matches your device
- Check that Bluetooth is enabled
- Make sure location services are enabled (required for BLE scanning on some Android versions)

### Permission Issues
- On Android 12+, ensure all Bluetooth permissions are granted
- On iOS, check Info.plist for required keys
- Try re-requesting permissions by uninstalling and reinstalling the app

### RSSI Values Not Updating
- Ensure the device is still in range
- Check that the BLE device is still broadcasting
- Try moving closer to the device

### App Crashes
- Check console logs: `expo start --verbose`
- Ensure all dependencies are properly installed
- Try clearing cache: `expo start -c`

## Dependencies

- **expo**: Base framework
- **react-native-ble-manager**: Bluetooth Low Energy management
- **expo-location**: Location permissions on iOS
- **expo-permissions**: Permission handling (legacy)
- **react-native-reanimated**: Smooth animations
- **typescript**: Type safety

## Building for Production

### Using EAS Build

```bash
# Build for iOS
eas build --platform ios

# Build for Android
eas build --platform android

# Build for both
eas build --platform all
```

### Local Build

```bash
# Prebuild native code
expo prebuild

# Build with Xcode (iOS)
npx react-native run-ios

# Build with Gradle (Android)
npx react-native run-android
```

## License

This project is part of the LostAndNeverFound tracker system. See the main repository LICENSE for details.

## Support

For issues or questions:
1. Check the troubleshooting section
2. Review the code comments in [app/BleTracker.tsx](app/BleTracker.tsx)
3. Check Expo documentation: https://docs.expo.dev
4. Check React Native BLE Manager docs: https://github.com/innoveit/react-native-ble-manager

## Future Enhancements

- [ ] Add multiple device tracking
- [ ] Store tracking history
- [ ] Add sound/vibration feedback
- [ ] Add more granular zone levels
- [ ] Implement device connection and data exchange
- [ ] Add map view showing device location
- [ ] Add export functionality for RSSI data
