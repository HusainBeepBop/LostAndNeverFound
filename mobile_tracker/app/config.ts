/**
 * BLE Configuration
 * 
 * This file contains all the configurable settings for the BLE Hot/Cold Tracker app.
 * Modify these values to customize the app behavior.
 */

import { RssiThresholds, BleConfig } from './types';

/**
 * Target BLE Service UUID
 * This is the Service UUID that the app will scan for.
 * Change this to match your device's advertised Service UUID.
 */
export const TARGET_SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b';

/**
 * RSSI Thresholds (in dBm)
 * 
 * RSSI (Received Signal Strength Indicator) is measured in dBm and is always negative.
 * 
 * Higher values (closer to 0) = stronger signal = closer distance
 * Lower values (more negative) = weaker signal = farther distance
 * 
 * Typical ranges:
 * -30 to -70 dBm: Very close (1-2 meters)
 * -70 to -100 dBm: Close (2-10 meters)
 * -100 to -120 dBm: Far (10-30 meters)
 * 
 * Adjust these values based on your environment and device:
 */
export const RSSI_THRESHOLDS: RssiThresholds = {
  cold: -80,  // Below this = Cold Zone (Freezing)
  warm: -60,  // Between cold and this = Warm Zone (Getting Warmer)
  hot: 0,     // Above warm = Hot Zone (Burning Hot!)
};

/**
 * Zone Colors and Messages
 * Customize the colors and messages for each zone
 */
export const ZONE_CONFIG = {
  cold: {
    color: '#1E90FF',        // Dodger Blue
    message: 'Freezing',
    description: 'Far Away',
  },
  warm: {
    color: '#FFA500',        // Orange
    message: 'Getting Warmer',
    description: 'Getting Close',
  },
  hot: {
    color: '#DC143C',        // Crimson Red
    message: 'Burning Hot!',
    description: 'Very Close',
  },
};

/**
 * BLE Scanning Configuration
 */
export const BLE_CONFIG: BleConfig = {
  targetServiceUuid: TARGET_SERVICE_UUID,
  scanDuration: 5,           // Scan for 5 seconds at a time
  rescanInterval: 4000,      // Wait 4 seconds before scanning again
  allowDuplicates: true,     // Allow duplicate device discoveries (for continuous RSSI updates)
};

/**
 * Animation Timings (in milliseconds)
 */
export const ANIMATION_TIMINGS = {
  colorTransition: 300,      // Color change animation duration
  zoneChange: 400,           // Zone change animation duration
  pulseAnimation: 500,       // Pulse effect duration
};

/**
 * Text Constants
 */
export const TEXT_CONSTANTS = {
  appTitle: 'BLE Tracker',
  searching: 'Searching for device...',
  scanning: 'Scanning...',
  signalStrength: 'Signal Strength',
  coldZone: 'Cold Zone',
  warmZone: 'Warm Zone',
  hotZone: 'Hot Zone',
};

/**
 * Permission Strings
 */
export const PERMISSION_MESSAGES = {
  bluetooth_ios: 'This app uses Bluetooth to scan for nearby devices',
  bluetoothCentral: 'This app uses Bluetooth to find your device',
  location_ios: 'Location is required to scan for Bluetooth devices on iOS',
  locationAndroid: 'Location permission is required for Bluetooth scanning',
};

/**
 * Alert Messages
 */
export const ALERTS = {
  permissionDenied: 'Permission Denied',
  locationPermissionMessage: 'Location permission is required for Bluetooth scanning on iOS',
  bluetoothError: 'Bluetooth Error',
  bluetoothInitError: 'Failed to initialize Bluetooth manager',
  scanError: 'Scanning Error',
};
