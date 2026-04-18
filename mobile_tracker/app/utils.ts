/**
 * Utility functions for RSSI calculations and device tracking
 */

import { Zone, RssiThresholds } from './types';

/**
 * RSSI Conversion and Calculation Functions
 */

/**
 * Convert RSSI in dBm to percentage (0-100)
 * This is useful for visualizations and progress bars
 * 
 * @param rssi - RSSI value in dBm (negative number)
 * @returns Percentage from 0 to 100
 */
export function rssiToPercentage(rssi: number): number {
  // RSSI range: -120 to 0
  // Convert to 0-100 percentage
  const percentage = (rssi + 120) * (100 / 120);
  return Math.max(0, Math.min(100, percentage));
}

/**
 * Estimate distance from RSSI value
 * Using the free space path loss model
 * 
 * @param rssi - RSSI value in dBm
 * @param txPower - TX Power of the device (typically -50 to 0 dBm, default -50)
 * @returns Estimated distance in meters
 */
export function rssiToDistance(rssi: number, txPower: number = -50): number {
  if (rssi >= 0) return 0;
  if (txPower >= 0) return 0;

  const ratio = rssi * 1.0 / txPower;
  if (ratio < 1.0) {
    return Math.pow(ratio, 10);
  } else {
    return Math.pow(ratio, 10) * 0.89976 + 0.111772 * Math.pow(ratio, 7);
  }
}

/**
 * Get intensity/proximity description based on RSSI
 */
export function getIntensityDescription(rssi: number): string {
  if (rssi > -60) return 'VERY CLOSE';
  if (rssi > -70) return 'CLOSE';
  if (rssi > -80) return 'MEDIUM DISTANCE';
  if (rssi > -90) return 'FAR';
  return 'VERY FAR';
}

/**
 * Calculate zone based on RSSI and thresholds
 * 
 * @param rssi - RSSI value in dBm
 * @param thresholds - RSSI thresholds for zones
 * @param zones - Available zones
 * @returns The appropriate zone
 */
export function calculateZone(
  rssi: number,
  thresholds: RssiThresholds,
  zones: Zone[]
): Zone {
  if (rssi > thresholds.warm) {
    return zones.find(z => z.name === 'Hot') || zones[2];
  } else if (rssi > thresholds.cold) {
    return zones.find(z => z.name === 'Warm') || zones[1];
  } else {
    return zones.find(z => z.name === 'Cold') || zones[0];
  }
}

/**
 * Smooth RSSI value with exponential moving average
 * Helps reduce noise in RSSI readings
 * 
 * @param currentRssi - Current RSSI reading
 * @param previousRssi - Previous smoothed RSSI value
 * @param alpha - Smoothing factor (0-1, typically 0.2-0.5)
 * @returns Smoothed RSSI value
 */
export function smoothRssi(
  currentRssi: number,
  previousRssi: number,
  alpha: number = 0.3
): number {
  return alpha * currentRssi + (1 - alpha) * previousRssi;
}

/**
 * Calculate signal quality based on RSSI
 * Returns a quality indicator string
 */
export function getSignalQuality(rssi: number): string {
  if (rssi >= -30) return 'Excellent';
  if (rssi >= -60) return 'Good';
  if (rssi >= -80) return 'Fair';
  if (rssi >= -100) return 'Weak';
  return 'Very Weak';
}

/**
 * Format RSSI value for display
 */
export function formatRssi(rssi: number): string {
  return `${rssi} dBm`;
}

/**
 * Format distance with appropriate unit
 */
export function formatDistance(distanceMeters: number): string {
  if (distanceMeters < 1) {
    return `${(distanceMeters * 100).toFixed(0)} cm`;
  }
  return `${distanceMeters.toFixed(1)} m`;
}

/**
 * Validate RSSI value
 * RSSI should be negative and between -120 and 0
 */
export function isValidRssi(rssi: number): boolean {
  return rssi >= -120 && rssi <= 0;
}

/**
 * Compare two RSSI values to determine if device is getting closer or farther
 * 
 * @param newRssi - New RSSI reading
 * @param oldRssi - Previous RSSI reading
 * @returns 'closer' | 'farther' | 'same'
 */
export function compareRssi(newRssi: number, oldRssi: number, threshold: number = 2): string {
  const difference = newRssi - oldRssi;
  if (difference > threshold) return 'closer';
  if (difference < -threshold) return 'farther';
  return 'same';
}

/**
 * Average multiple RSSI values
 * Useful for getting a stable reading after multiple scans
 */
export function averageRssiValues(values: number[]): number {
  if (values.length === 0) return -100;
  const sum = values.reduce((a, b) => a + b, 0);
  return Math.round(sum / values.length);
}
