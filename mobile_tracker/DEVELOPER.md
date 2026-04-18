# Developer Guide

## Architecture Overview

The app is built with a modular architecture:

```
┌─────────────────────────────────────────┐
│         App.tsx (Entry Point)           │
└──────────────────┬──────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────┐
│    BleTracker.tsx (Business Logic)      │
│  - Permissions                          │
│  - BLE Scanning                         │
│  - Device Discovery                     │
│  - RSSI Updates                         │
└──────────────┬──────────────┬───────────┘
               │              │
               ▼              ▼
    ┌──────────────────┐  ┌──────────────┐
    │ ZoneDisplay.tsx  │  │ utils.ts     │
    │ (UI Component)   │  │ (Helpers)    │
    └──────────────────┘  └──────────────┘
               ▲
               │
         ┌─────┴──────┐
         │            │
    ┌────────────┐ ┌─────────────┐
    │ config.ts  │ │ types.ts    │
    │(Constants) │ │ (Types)     │
    └────────────┘ └─────────────┘
```

## Module Descriptions

### App.tsx
**Purpose**: Application entry point
**Responsibilities**:
- Render status bar
- Mount BleTracker component

### BleTracker.tsx
**Purpose**: Core BLE functionality and state management
**Key Functions**:
- `requestPermissions()`: Handle OS permissions
- `initiateBleManager()`: Initialize BLE manager
- `startScanning()`: Begin BLE device scanning
- `updateZone()`: Update zone based on RSSI
- State management with hooks

### ZoneDisplay.tsx
**Purpose**: Visual UI component
**Key Features**:
- Animated background color
- Real-time RSSI display
- Zone messages and indicators
- Smooth transitions

### utils.ts
**Purpose**: Helper functions and calculations
**Utilities**:
- `rssiToDistance()`: Estimate distance from RSSI
- `calculateZone()`: Determine current zone
- `smoothRssi()`: Apply smoothing filter
- `getIntensityDescription()`: Get proximity text

### config.ts
**Purpose**: Centralized configuration
**Contains**:
- Target Service UUID
- RSSI thresholds
- Zone colors and messages
- BLE scanning parameters
- Animation timings

### types.ts
**Purpose**: TypeScript type definitions
**Defines**:
- `BleDevice`: Device structure
- `Zone`: Zone type definition
- `TrackingState`: App state type
- `BleConfig`: Configuration type

## Adding New Features

### Example: Adding Sound Notifications

1. **Install dependency**:
   ```bash
   npm install expo-av
   ```

2. **Create sound utility** (`app/soundManager.ts`):
   ```typescript
   import { Audio } from 'expo-av';

   export class SoundManager {
     private soundObjects: Map<string, Audio.Sound> = new Map();

     async loadSound(name: string, source: any) {
       const { sound } = await Audio.Sound.createAsync(source);
       this.soundObjects.set(name, sound);
     }

     async playSound(name: string) {
       const sound = this.soundObjects.get(name);
       if (sound) await sound.replayAsync();
     }
   }
   ```

3. **Use in BleTracker**:
   ```typescript
   const soundManager = new SoundManager();
   
   // When zone changes
   if (newZone.name === 'Hot') {
     await soundManager.playSound('hot');
   }
   ```

### Example: Adding Haptic Feedback

1. **Install dependency**:
   ```bash
   npm install expo-haptics
   ```

2. **Import and use**:
   ```typescript
   import * as Haptics from 'expo-haptics';

   const updateZone = async (rssi: number) => {
     const newZone = calculateZone(rssi, RSSI_THRESHOLDS, ZONES);
     if (newZone !== currentZone) {
       await Haptics.notificationAsync(Haptics.NotificationFeedbackType.Success);
       setCurrentZone(newZone);
     }
   };
   ```

### Example: Adding Data Logging

1. **Create logger** (`app/logger.ts`):
   ```typescript
   import AsyncStorage from '@react-native-async-storage/async-storage';

   export class Logger {
     async logRssi(rssi: number, zone: string) {
       const timestamp = new Date().toISOString();
       const log = { timestamp, rssi, zone };
       const existing = JSON.parse(
         await AsyncStorage.getItem('rssiLogs') || '[]'
       );
       existing.push(log);
       await AsyncStorage.setItem('rssiLogs', JSON.stringify(existing));
     }
   }
   ```

2. **Use in BleTracker**:
   ```typescript
   const logger = new Logger();
   
   const updateZone = (rssi: number) => {
     const newZone = calculateZone(rssi, RSSI_THRESHOLDS, ZONES);
     logger.logRssi(rssi, newZone.name);
   };
   ```

## Modifying RSSI Calculation

### Custom Distance Formula

Edit `utils.ts` to implement your own path loss model:

```typescript
export function rssiToDistance(rssi: number, txPower: number = -50): number {
  // Your custom formula here
  // Example: Linear approximation
  const ratio = rssi / txPower;
  return Math.pow(10, (txPower - rssi) / (10 * 2.4));
}
```

### Applying Different Smoothing

```typescript
// Exponential moving average (current implementation)
export function smoothRssi(current, previous, alpha = 0.3) {
  return alpha * current + (1 - alpha) * previous;
}

// Alternative: Moving average
export function smoothRssiMovingAverage(values: number[], window = 5) {
  return values.slice(-window)
    .reduce((a, b) => a + b) / Math.min(window, values.length);
}
```

## Performance Optimization

### Memory Optimization

```typescript
// Limit RSSI history size
if (rssiHistoryRef.current.length > 10) {
  rssiHistoryRef.current.shift(); // Remove oldest
}

// Clear unused device data
scannedDevices.current.forEach((device, id) => {
  if (Date.now() - device.lastSeen > 60000) {
    scannedDevices.current.delete(id); // Remove if not seen in 1 minute
  }
});
```

### CPU Optimization

```typescript
// Reduce animation complexity
const backgroundColor = colorAnimation.interpolate({
  inputRange: [0, 1],
  outputRange: [zone.color, zone.color], // No transition
});

// Throttle RSSI updates
const shouldUpdate = rssiValue > lastRssi + 2 || rssiValue < lastRssi - 2;
if (shouldUpdate) {
  setRssiValue(rssiValue);
}
```

### Battery Optimization

```typescript
// Reduce scan frequency
export const BLE_CONFIG = {
  scanDuration: 2,      // Shorter scans
  rescanInterval: 8000, // Longer intervals
  allowDuplicates: false, // Fewer events
};
```

## Debugging

### Enable Verbose Logging

```bash
expo start --verbose
```

### Add Custom Debugging

```typescript
// BleTracker.tsx
const DEBUG = true;

const log = (message: string, data?: any) => {
  if (DEBUG) {
    console.log(`[BleTracker] ${message}`, data);
  }
};

// Usage
log('Zone changed', { from: currentZone.name, to: newZone.name });
```

### Monitor Performance

```typescript
import { performance } from 'react-native-performance';

const startTime = performance.now();
// ... operation ...
const duration = performance.now() - startTime;
console.log(`Operation took ${duration}ms`);
```

### Check Memory Usage

```typescript
import { Platform } from 'react-native';

if (Platform.OS === 'android') {
  // Monitor with React Native Debugger
  console.log(JSON.stringify(performance.getEntriesByType('measure')));
}
```

## Testing

### Unit Tests Example

```typescript
// __tests__/utils.test.ts
import { rssiToDistance, calculateZone } from '../app/utils';
import { RSSI_THRESHOLDS } from '../app/config';

describe('RSSI Utilities', () => {
  test('rssiToDistance calculates correctly', () => {
    const distance = rssiToDistance(-50, -50);
    expect(distance).toBeCloseTo(1, 1); // About 1 meter
  });

  test('calculateZone returns correct zone', () => {
    const zone = calculateZone(-90, RSSI_THRESHOLDS, ZONES);
    expect(zone.name).toBe('Cold');
  });
});
```

### Integration Tests

```typescript
// __tests__/BleTracker.integration.test.ts
import React from 'react';
import { render, waitFor } from '@testing-library/react-native';
import BleTracker from '../app/BleTracker';

describe('BleTracker Integration', () => {
  test('app initializes and requests permissions', async () => {
    const { getByText } = render(<BleTracker />);
    
    await waitFor(() => {
      expect(getByText(/Searching for device/i)).toBeTruthy();
    });
  });
});
```

## API Reference

### BleTracker Component

```typescript
interface BleTrackerProps {
  // Component receives no props - uses internal state
}

// State
- scanning: boolean
- deviceFound: boolean
- rssiValue: number
- currentZone: Zone
- permissionsGranted: boolean
- loading: boolean
- deviceName: string
```

### ZoneDisplay Component

```typescript
interface ZoneDisplayProps {
  zone: Zone;              // Current zone
  rssi: number;            // RSSI value in dBm
  deviceFound: boolean;    // Whether device is found
  deviceName: string;      // Device name to display
  scanning: boolean;       // Whether currently scanning
}
```

### Utility Functions

```typescript
// Distance calculation
rssiToDistance(rssi: number, txPower?: number): number

// Zone determination
calculateZone(rssi: number, thresholds: RssiThresholds, zones: Zone[]): Zone

// RSSI smoothing
smoothRssi(current: number, previous: number, alpha?: number): number

// Intensity description
getIntensityDescription(rssi: number): string

// Signal quality
getSignalQuality(rssi: number): string

// Format functions
formatRssi(rssi: number): string
formatDistance(meters: number): string

// Validation
isValidRssi(rssi: number): boolean

// Comparison
compareRssi(newRssi: number, oldRssi: number, threshold?: number): string

// Statistics
averageRssiValues(values: number[]): number
```

## Dependency Management

### Current Dependencies

```json
{
  "expo": "^50.0.0",           // Base framework
  "react-native": "0.73.0",    // React Native
  "react": "18.2.0",           // React
  "react-native-ble-manager": "^9.3.7",  // BLE functionality
  "expo-location": "^15.0.0",  // iOS location permissions
  "expo-permissions": "^14.4.0", // Permission handling
  "react-native-reanimated": "~3.6.0", // Smooth animations
  "react-native-screens": "~3.27.0"  // Screen management
}
```

### Adding Dependencies

```bash
# For npm
npm install package-name

# For expo packages
npm install expo-package-name

# For native packages (requires prebuild)
expo install package-name
expo prebuild --clean
```

## Production Checklist

- [ ] Remove debug logs
- [ ] Set `DEBUG = false`
- [ ] Optimize bundle size
- [ ] Test on real devices
- [ ] Verify permissions
- [ ] Test battery drain
- [ ] Create app icon and splash screen
- [ ] Set version number in app.json
- [ ] Configure app.json for distribution
- [ ] Build for production with EAS

## Troubleshooting Development

| Problem | Solution |
|---------|----------|
| Permissions not working | Clear cache: `expo start -c` |
| BLE not scanning | Restart BleManager: stop app and restart |
| RSSI not updating | Check device is still advertising |
| Animations stuttering | Reduce animation complexity, check `useNativeDriver` |
| Memory leak | Check interval/subscription cleanup in useEffect |
| Device crashes | Check console for uncaught exceptions |

## Resources

- [Expo Documentation](https://docs.expo.dev)
- [React Native BLE Manager](https://github.com/innoveit/react-native-ble-manager)
- [Bluetooth RSSI Theory](https://en.wikipedia.org/wiki/Received_signal_strength_indication)
- [React Native Performance](https://reactnative.dev/docs/performance)
- [TypeScript Handbook](https://www.typescriptlang.org/docs/)
