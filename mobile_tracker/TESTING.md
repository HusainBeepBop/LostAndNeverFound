# Testing Guide for BLE Hot or Cold Tracker

## Pre-Testing Checklist

- [ ] BLE device is powered on and advertising the target Service UUID
- [ ] Mobile device has Bluetooth enabled
- [ ] Mobile device has location services enabled
- [ ] App permissions have been granted
- [ ] You're in a controlled environment (minimize interference)

## Unit Testing the App Components

### Test 1: Permission Request Flow

**Objective**: Verify permissions are requested correctly

1. Uninstall app completely
2. Launch app fresh
3. **iOS**: Verify two permission prompts appear (Bluetooth + Location)
4. **Android**: Verify three permission prompts appear (Bluetooth Scan + Connect + Location)
5. Accept all permissions
6. Verify app proceeds to scanning screen

**Expected Result**: ✅ All permissions granted, no errors

### Test 2: BLE Initialization

**Objective**: Verify BLE Manager initializes correctly

1. App should display "Searching for device..." message
2. Check console logs: `expo start --verbose`
3. Look for: `BleManager initialized` and `Scanning started`

**Expected Result**: ✅ BLE manager initialized, scanning begins

### Test 3: Device Discovery

**Objective**: Verify target device is discovered

1. Ensure BLE device is in range (< 10 meters)
2. Wait 5-10 seconds for scan
3. App should display device name when found
4. Screen color should appear (blue/orange/red)

**Expected Result**: ✅ Device found and displayed

## Functional Testing

### Test 4: RSSI Display

**Objective**: Verify RSSI value updates in real-time

**Setup**: Device 3 meters away (expected RSSI: -60 to -70 dBm)

1. Launch app and wait for device discovery
2. Observe RSSI number on screen
3. Move device around slightly
4. Verify RSSI changes ± 3-5 dBm

**Expected Result**: ✅ RSSI updates every 1-2 seconds

### Test 5: Zone Detection - Cold Zone

**Objective**: Verify Cold Zone behavior

**Setup**: Place device 15+ meters away or behind multiple walls

1. Launch app
2. Observe screen color (should be blue)
3. Verify text shows "Freezing"
4. Verify RSSI is below -80 dBm

**Expected Result**: ✅ Blue screen with "Freezing" message

### Test 6: Zone Detection - Warm Zone

**Objective**: Verify Warm Zone behavior

**Setup**: Place device 3-5 meters away, clear line of sight

1. Move closer from cold zone
2. Observe color transition to orange
3. Verify text shows "Getting Warmer"
4. Verify RSSI is between -80 and -60 dBm

**Expected Result**: ✅ Orange screen with "Getting Warmer" message

### Test 7: Zone Detection - Hot Zone

**Objective**: Verify Hot Zone behavior

**Setup**: Place device 1-2 meters away

1. Move very close to device
2. Observe color change to red
3. Verify text shows "Burning Hot!"
4. Verify RSSI is greater than -60 dBm

**Expected Result**: ✅ Red screen with "Burning Hot!" message

### Test 8: Smooth Color Transitions

**Objective**: Verify animations between zones

1. Stand with device at cold zone boundary (-80 dBm)
2. Slowly move closer to device
3. Observe smooth color gradient transitions
4. Verify no sudden jumps between colors

**Expected Result**: ✅ Smooth transitions, no flickering

### Test 9: RSSI Stability

**Objective**: Verify RSSI doesn't fluctuate wildly

**Setup**: Device stationary 5 meters away

1. Observe RSSI values over 30 seconds
2. Record min and max values
3. Calculate variance: max - min
4. Should be ≤ 10 dBm for stable reading

**Expected Result**: ✅ RSSI variance < 10 dBm

### Test 10: Distance Accuracy

**Objective**: Verify distance estimates are reasonable

**Setup**: Measure actual distances with tape measure

1. Place device at exactly 1 meter
   - Expected RSSI: -45 to -55 dBm → Hot Zone
2. Move to exactly 5 meters
   - Expected RSSI: -65 to -75 dBm → Warm Zone
3. Move to exactly 15 meters
   - Expected RSSI: -85 to -100 dBm → Cold Zone

**Expected Result**: ✅ RSSI matches distance

## Stress Testing

### Test 11: Extended Runtime

**Objective**: Verify app stability over long operation

1. Launch app
2. Let it run for 1 hour
3. Periodically check:
   - RSSI is updating
   - No memory leaks
   - App doesn't crash
   - Animations still smooth

**Expected Result**: ✅ App stable after 1+ hour

### Test 12: Multiple Zone Transitions

**Objective**: Verify app handles rapid zone changes

1. Stand at zone boundary (around -70 dBm RSSI)
2. Move slowly back and forth
3. Trigger 20+ zone transitions over 2 minutes
4. Verify smooth transitions each time
5. Check console for errors

**Expected Result**: ✅ No crashes, smooth transitions

### Test 13: Low Signal Conditions

**Objective**: Verify behavior in poor signal environment

**Setup**: 
1. Place device behind metal objects
2. Or 30+ meters away outdoors
3. RSSI should be -100 to -120 dBm

1. App should still show "Freezing"
2. No crashes with extreme RSSI values
3. Can still read the display

**Expected Result**: ✅ App handles extreme values gracefully

## Environment-Specific Testing

### Test 14: Open Field

**Setup**: Outdoors, flat open area, no obstacles

1. Place device at 20 meters
2. Measure RSSI: _____ dBm
3. Move to 10 meters
4. Measure RSSI: _____ dBm
5. Move to 5 meters
6. Measure RSSI: _____ dBm

**Expected Result**: ✅ RSSI improves ~6dB for each distance reduction

### Test 15: Urban Environment

**Setup**: City area with buildings, metal structures

1. Place device 5 meters away
2. Note RSSI: _____ dBm
3. Increase distance by 5 meters (10m total)
4. Note RSSI: _____ dBm
5. Expected degradation: 15-25 dBm (higher than open field)

**Expected Result**: ✅ Handles multipath fading

### Test 16: Through Walls

**Setup**: Device in adjacent room

1. Door closed: RSSI: _____ dBm
2. Open door: RSSI: _____ dBm
3. Difference should be 5-15 dBm

**Expected Result**: ✅ Shows wall attenuation effect

## Android-Specific Testing

### Test 17: Android Permissions Runtime

**Objective**: Verify Android permission system integration

1. Install on Android device
2. Verify first-launch permission prompts
3. Accept BLUETOOTH_SCAN
4. Accept BLUETOOTH_CONNECT
5. Accept ACCESS_FINE_LOCATION
6. Verify app starts scanning

**Expected Result**: ✅ All permissions requested in order

### Test 18: Android Location Services

**Objective**: Verify behavior with/without location services

1. Enable location services → App works ✅
2. Disable location services
3. App should show error or notification
4. Re-enable location services
5. App resumes scanning

**Expected Result**: ✅ Graceful handling of location toggle

## iOS-Specific Testing

### Test 19: iOS Background Behavior

**Objective**: Verify app behavior when backgrounded

1. App is scanning
2. Send app to background
3. After 5 seconds, note RSSI updates stop
4. Bring app to foreground
5. RSSI updates resume after 1-2 seconds

**Expected Result**: ✅ Resumes cleanly

### Test 20: iOS Location Permission

**Objective**: Verify iOS location permission requirement

1. Deny location permission → App shows error
2. Go to Settings → Privacy → Location
3. Enable "While Using" for app
4. Relaunch app
5. App should start scanning

**Expected Result**: ✅ Follows iOS guidelines

## Test Report Template

```
App Version: ___________
Device Model: ___________
OS Version: ___________
Test Date: ___________

Test Results:
Test 1:  [ ] PASS  [ ] FAIL  - Permission Request Flow
Test 2:  [ ] PASS  [ ] FAIL  - BLE Initialization
Test 3:  [ ] PASS  [ ] FAIL  - Device Discovery
Test 4:  [ ] PASS  [ ] FAIL  - RSSI Display
Test 5:  [ ] PASS  [ ] FAIL  - Cold Zone Detection
Test 6:  [ ] PASS  [ ] FAIL  - Warm Zone Detection
Test 7:  [ ] PASS  [ ] FAIL  - Hot Zone Detection
Test 8:  [ ] PASS  [ ] FAIL  - Smooth Transitions
Test 9:  [ ] PASS  [ ] FAIL  - RSSI Stability
Test 10: [ ] PASS  [ ] FAIL  - Distance Accuracy

Overall Result: [ ] PASS [ ] FAIL

Issues Found:
_________________________________
_________________________________

Notes:
_________________________________
_________________________________
```

## Performance Benchmarks

After all tests pass, document performance:

| Metric | Expected | Actual |
|--------|----------|--------|
| Cold Zone RSSI Range | < -80 dBm | _______ |
| Warm Zone RSSI Range | -80 to -60 | _______ |
| Hot Zone RSSI Range | > -60 dBm | _______ |
| Update Frequency | 1-2 sec | _______ |
| Animation Duration | ~300ms | _______ |
| Memory Usage | < 100MB | _______ |
| Battery Drain | ~5-10% per hour | _______ |

## Known Limitations

1. **Distance Estimation**: RSSI-based distance is approximate (±20-30%)
2. **Multipath Fading**: Walls and metal objects cause unpredictable RSSI variations
3. **Temperature Sensitivity**: RSSI can vary ±3-5 dBm with temperature
4. **Mobile Interference**: WiFi, microwaves, and other 2.4GHz devices affect RSSI
5. **Battery Status**: Low battery on BLE device may reduce TX power

## Success Criteria

✅ All 20 tests pass
✅ No crashes during extended use
✅ Smooth animations without stuttering
✅ RSSI updates at least every 2 seconds
✅ Correct zone detection at defined thresholds
✅ Proper permission handling on both platforms
✅ No excessive battery drain
