/**
 * Type definitions for BLE Tracker App
 */

/**
 * Represents a BLE device discovered during scanning
 */
export interface BleDevice {
  id: string;
  name?: string;
  rssi: number;
  advertising?: {
    manufacturerData?: any;
    serviceData?: any;
    serviceUUIDs?: string[];
  };
  serviceUUIDs?: string[];
}

/**
 * Represents a temperature zone with characteristics
 */
export interface Zone {
  name: 'Cold' | 'Warm' | 'Hot';
  text: string;
  color: string;
  rssiRange: [number, number];
}

/**
 * Tracking state for the application
 */
export interface TrackingState {
  scanning: boolean;
  deviceFound: boolean;
  rssiValue: number;
  currentZone: Zone;
  permissionsGranted: boolean;
  loading: boolean;
  deviceName: string;
  lastUpdated?: Date;
}

/**
 * Configuration for BLE scanning
 */
export interface BleConfig {
  targetServiceUuid: string;
  scanDuration: number; // in seconds
  rescanInterval: number; // in milliseconds
  allowDuplicates: boolean;
}

/**
 * RSSI to zone mapping configuration
 */
export interface RssiThresholds {
  cold: number; // RSSI threshold for cold zone
  warm: number; // RSSI threshold for warm zone
  hot: number; // RSSI threshold for hot zone (always upper bound)
}
