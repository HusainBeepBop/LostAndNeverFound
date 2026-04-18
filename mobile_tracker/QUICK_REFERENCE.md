# Quick Reference Guide

## Getting Started (30 seconds)

```bash
cd mobile_tracker
npm install
npm start
```

Then scan the QR code with Expo Go app.

## Key File Locations

| Task | File |
|------|------|
| Change target UUID | `app/config.ts` (line: `TARGET_SERVICE_UUID`) |
| Adjust zone thresholds | `app/config.ts` (line: `RSSI_THRESHOLDS`) |
| Change zone colors | `app/config.ts` (line: `ZONE_CONFIG`) |
| Customize messages | `app/config.ts` (line: `ZONE_CONFIG`) |
| Main logic | `app/BleTracker.tsx` |
| UI layout | `app/ZoneDisplay.tsx` |
| Utilities | `app/utils.ts` |
| Types | `app/types.ts` |

## Common Commands

```bash
# Start development
npm start

# Run on iOS
npm run ios

# Run on Android
npm run android

# Start with clean cache
npm start -c

# Build for production
expo prebuild --clean
# Then eas build

# Clear all cache
rm -rf node_modules .expo
npm install
npm start -c
```

## Common Modifications

### Change Target BLE Device

**File**: `app/config.ts`

```typescript
export const TARGET_SERVICE_UUID = 'YOUR-NEW-UUID';
```

### Adjust Zone Thresholds

**File**: `app/config.ts`

```typescript
export const RSSI_THRESHOLDS: RssiThresholds = {
  cold: -85,   // Change to match your device
  warm: -65,
  hot: 0,
};
```

### Change Colors

**File**: `app/config.ts`

```typescript
export const ZONE_CONFIG = {
  cold: {
    color: '#0000FF',  // Change color hex code
    message: 'Freezing',
    description: 'Far Away',
  },
  // ...
};
```

### Adjust Scanning Frequency

**File**: `app/config.ts`

```typescript
export const BLE_CONFIG: BleConfig = {
  scanDuration: 3,           // Seconds per scan
  rescanInterval: 5000,      // Milliseconds between scans
  allowDuplicates: true,     // true = more updates, false = less
};
```

### Change Animation Speed

**File**: `app/config.ts`

```typescript
export const ANIMATION_TIMINGS = {
  colorTransition: 500,    // Make slower
  zoneChange: 600,
  pulseAnimation: 800,
};
```

## RSSI Reference

| Distance | Environment | Typical RSSI |
|----------|-------------|--------------|
| 1m | Open | -40 to -50 dBm |
| 3m | Open | -55 to -65 dBm |
| 5m | Open | -65 to -75 dBm |
| 10m | Open | -75 to -90 dBm |
| 20m | Open | -90 to -110 dBm |
| 1m | Through wall | -70 to -80 dBm |

## Zone Configuration Presets

### Short Range (Room Finder)
```typescript
const RSSI_THRESHOLDS = { cold: -85, warm: -65, hot: 0 };
```

### Long Range (Building Finder)
```typescript
const RSSI_THRESHOLDS = { cold: -100, warm: -75, hot: 0 };
```

### Very Precise
```typescript
const RSSI_THRESHOLDS = { cold: -78, warm: -62, hot: 0 };
```

## Debugging Checklist

- [ ] Is device powered on?
- [ ] Is Bluetooth enabled on phone?
- [ ] Is location enabled (required for Android/iOS)?
- [ ] Are permissions granted?
- [ ] Is correct Service UUID in config?
- [ ] Is device in range?
- [ ] Check console logs: `expo start --verbose`
- [ ] Try clearing cache: `npm start -c`

## Error Messages and Fixes

| Error | Fix |
|-------|-----|
| "Device not found" | 1. Check UUID 2. Ensure device is on 3. Get closer |
| "Permission denied" | Grant Bluetooth permissions in settings |
| "Scanning error" | Restart app, check BleManager initialized |
| "White screen" | Wait 5 seconds, then kill and restart app |
| "RSSI not updating" | Move device slightly, ensure device is advertising |

## Performance Tips

| Goal | Action |
|------|--------|
| Save battery | Increase `scanDuration` and `rescanInterval` |
| Faster response | Decrease `scanDuration` and `rescanInterval` |
| Stable readings | Increase RSSI smoothing: `alpha: 0.5` |
| Remove jitter | Increase zone thresholds width (e.g., -85 to -65) |

## Testing Different Scenarios

```bash
# Test 1: Close proximity
npm start
# Hold device 1 meter away, expect red screen

# Test 2: Medium range  
# Move to 5 meters, expect orange screen

# Test 3: Far away
# Move to 15+ meters, expect blue screen

# Test 4: Obstacles
# Place device behind wall, observe RSSI drop ~15-20 dBm
```

## Useful TypeScript Types

```typescript
// From types.ts - useful when extending app

interface Zone {
  name: 'Cold' | 'Warm' | 'Hot';
  text: string;
  color: string;
  rssiRange: [number, number];
}

interface BleDevice {
  id: string;
  name?: string;
  rssi: number;
  serviceUUIDs?: string[];
}

interface TrackingState {
  scanning: boolean;
  deviceFound: boolean;
  rssiValue: number;
  currentZone: Zone;
  permissionsGranted: boolean;
  loading: boolean;
  deviceName: string;
}
```

## One-Line Solutions

```typescript
// Get zone from RSSI
const zone = calculateZone(rssi, RSSI_THRESHOLDS, ZONES);

// Convert RSSI to distance
const distance = rssiToDistance(rssi);

// Smooth RSSI value
const smooth = smoothRssi(newRssi, prevRssi, 0.3);

// Get intensity text
const intensity = getIntensityDescription(rssi);

// Get signal quality
const quality = getSignalQuality(rssi);

// Format for display
const display = formatRssi(rssi);

// Validate RSSI
const valid = isValidRssi(rssi);
```

## File Tree

```
mobile_tracker/
├── App.tsx                    # Entry point
├── app/
│   ├── BleTracker.tsx        # Main logic
│   ├── ZoneDisplay.tsx       # UI component
│   ├── config.ts             # Settings ⭐ (Modify this!)
│   ├── types.ts              # Type definitions
│   └── utils.ts              # Helper functions
├── app.json                  # Expo config
├── package.json              # Dependencies
├── tsconfig.json             # TypeScript config
├── babel.config.js           # Babel config
├── eas.json                  # EAS config
├── README.md                 # Overview
├── SETUP.md                  # Installation guide
├── TESTING.md                # Testing procedures
├── EXAMPLES.md               # Configuration examples
├── DEVELOPER.md              # Developer reference
└── QUICK_REFERENCE.md        # This file!
```

## Keyboard Shortcuts (Expo Go)

| Key | Action |
|-----|--------|
| `m` | Open menu |
| `h` | Refresh app |
| `i` | Open iOS simulator |
| `a` | Open Android emulator |
| `w` | Open web |
| `d` | Open debug menu |
| `q` | Quit |

## Environment Variables

Create `.env` in `mobile_tracker/`:

```env
# Example env variables
BLUETOOTH_SCAN_DURATION=5
BLUETOOTH_RESCAN_INTERVAL=4000
TARGET_SERVICE_UUID=4fafc201-1fb5-459e-8fcc-c5c9c331914b
```

Then in config.ts:
```typescript
import * as env from '.env.json';
export const TARGET_SERVICE_UUID = env.TARGET_SERVICE_UUID;
```

## Measuring RSSI Values

**On Android (adb):**
```bash
adb logcat | grep BleTracker
```

**On iOS (Xcode):**
```
Xcode > Device > Console
Search for "BleTracker"
```

**In App Console:**
```bash
expo start --verbose
# Watch for RSSI logs
```

## Next Steps

1. ✅ Get app running with `npm start`
2. ✅ Test with your BLE device
3. ✅ Adjust `config.ts` to match your device
4. ✅ Customize colors and messages
5. ✅ Build for production with `expo prebuild`
6. ✅ Submit to app stores

## Quick Help

```
Need help?
- README.md      - Project overview
- SETUP.md       - Installation steps  
- TESTING.md     - How to test app
- EXAMPLES.md    - Configuration examples
- DEVELOPER.md   - How to extend app
- This file!     - Quick answers
```

## Version Info

- **Expo**: 50.0.0+
- **React Native**: 0.73.0+
- **React**: 18.2.0+
- **Node.js**: 14.0.0+
- **npm**: 6.0.0+

## Support Resources

- [Expo Docs](https://docs.expo.dev)
- [React Native Docs](https://reactnative.dev)
- [BLE Manager Repo](https://github.com/innoveit/react-native-ble-manager)
- [Bluetooth Basics](https://www.bluetooth.com)

---

**Last Updated**: 2026-04-18
**For Questions**: Check DEVELOPER.md or create an issue on GitHub
