# Example Custom Configurations

This directory contains example configurations for different use cases.

## Configuration Examples

### Example 1: Short Range Detection (2-5 meters)
For finding items within a small area like a room.

```typescript
// app/config.ts
export const RSSI_THRESHOLDS: RssiThresholds = {
  cold: -85,   // < -85 dBm (5m+)
  warm: -65,   // -85 to -65 dBm (2-5m)
  hot: 0,      // > -65 dBm (< 2m)
};
```

### Example 2: Long Range Detection (10-50 meters)
For finding items across large areas like a building or outdoor space.

```typescript
// app/config.ts
export const RSSI_THRESHOLDS: RssiThresholds = {
  cold: -100,  // < -100 dBm (40m+)
  warm: -75,   // -100 to -75 dBm (10-40m)
  hot: 0,      // > -75 dBm (< 10m)
};
```

### Example 3: Very High Sensitivity (Precise Distance)
For precise distance measurements.

```typescript
// app/config.ts
export const RSSI_THRESHOLDS: RssiThresholds = {
  cold: -78,   // Fine-tuned for high sensitivity
  warm: -62,
  hot: 0,
};

export const ANIMATION_TIMINGS = {
  colorTransition: 200,  // Faster transitions
  zoneChange: 300,
  pulseAnimation: 400,
};
```

### Example 4: Custom Colors Theme
Customize colors for accessibility or branding.

```typescript
// app/config.ts
export const ZONE_CONFIG = {
  cold: {
    color: '#0066FF',      // Bright Blue
    message: 'Far Away',
    description: 'Not in range',
  },
  warm: {
    color: '#FFAA00',      // Golden Orange
    message: 'Getting Closer',
    description: 'Approaching',
  },
  hot: {
    color: '#FF0000',      // Pure Red
    message: 'Found It!',
    description: 'Very Close',
  },
};
```

## Testing Different Scenarios

### Scenario 1: Stationary Device
1. Place BLE device 5 meters away
2. Observe: Blue screen with "Freezing"
3. Move to 3 meters away
4. Observe: Color transition to orange "Getting Warmer"
5. Get within 1 meter
6. Observe: Red screen "Burning Hot!"

### Scenario 2: Moving Device
1. Start with device 10 meters away
2. Slowly walk toward the device
3. Observe: Smooth color transitions as you get closer
4. RSSI values should increase (become less negative)

### Scenario 3: Obstacles Test
1. Place device in adjacent room
2. Note RSSI value
3. Open door between rooms
4. Observe RSSI improvement (usually 5-10 dBm increase)
5. This shows wall attenuation effects

## Performance Metrics

### Expected RSSI Ranges by Distance

| Distance | Environment | Typical RSSI |
|----------|-------------|--------------|
| 1m | Open space | -40 to -50 dBm |
| 2m | Open space | -50 to -60 dBm |
| 5m | Open space | -65 to -75 dBm |
| 10m | Open space | -75 to -85 dBm |
| 20m | Open space | -85 to -100 dBm |
| 1m | Through wall | -70 to -80 dBm |
| 5m | Through wall | -85 to -100 dBm |

## Optimization Tips

### For Better Accuracy:
1. **Reduce Noise**: Increase smoothing factor in `utils.ts` smoothRssi function
2. **Faster Response**: Reduce `scanDuration` and `rescanInterval` in config.ts
3. **Stability**: Increase `rescanInterval` for less frequent updates (saves battery)

### For Battery Optimization:
```typescript
export const BLE_CONFIG: BleConfig = {
  targetServiceUuid: TARGET_SERVICE_UUID,
  scanDuration: 3,           // Shorter scans
  rescanInterval: 6000,      // Longer intervals (6 seconds)
  allowDuplicates: false,    // Reduce events
};
```

### For Real-Time Responsiveness:
```typescript
export const BLE_CONFIG: BleConfig = {
  targetServiceUuid: TARGET_SERVICE_UUID,
  scanDuration: 5,           // Longer scans per cycle
  rescanInterval: 2000,      // More frequent scans
  allowDuplicates: true,     // More updates
};
```

## Device-Specific Adjustments

### For Different BLE Devices:

1. **High TX Power Device** (Strong signal):
   - Use higher thresholds (less negative)
   - Adjust warm and hot boundaries up by 10-20 dBm

2. **Low TX Power Device** (Weak signal):
   - Use lower thresholds (more negative)
   - Adjust boundaries down by 10-20 dBm

3. **Noisy Environment** (Interference):
   - Increase smoothing factor: `alpha: 0.5` instead of `0.3`
   - Increase rescan interval for stability
   - Widen zone boundaries

## Troubleshooting Custom Configs

### Zones Change Too Frequently
**Solution**: Increase smoothing and rescan interval
```typescript
// Slower updates with more averaging
alpha: 0.5  // More smoothing
rescanInterval: 5000  // 5 seconds between scans
```

### Zones Don't Change at All
**Solution**: Lower the thresholds (make more negative)
```typescript
RSSI_THRESHOLDS = {
  cold: -100,
  warm: -75,
  hot: 0,
};
```

### Device Not Found
**Solution**: Check Service UUID and lower cold threshold
```typescript
// Verify UUID matches your device exactly
TARGET_SERVICE_UUID = 'YOUR-ACTUAL-UUID';

// Lower threshold to catch weaker signals
RSSI_THRESHOLDS.cold = -110;
```

## Creating a Custom Config File

1. Create a new file: `app/customConfig.ts`
2. Copy configuration from `app/config.ts`
3. Modify values for your use case
4. In `BleTracker.tsx`, change imports:
   ```typescript
   import { TARGET_SERVICE_UUID, ... } from './customConfig';
   ```

## Performance Monitoring

Add this logging to monitor performance:

```typescript
// In BleTracker.tsx
console.log(`[${new Date().toISOString()}] RSSI: ${rssiValue}, Zone: ${currentZone.name}`);
```

This will help you understand:
- RSSI fluctuation patterns
- Zone change frequency
- System responsiveness

Use these logs to optimize your configuration.
