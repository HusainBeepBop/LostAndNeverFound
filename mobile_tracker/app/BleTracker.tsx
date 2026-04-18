import React, { useState, useEffect, useRef } from 'react';
import { 
  View, 
  Text, 
  StyleSheet, 
  ActivityIndicator, 
  Alert,
  Animated,
  Dimensions,
  PermissionsAndroid,
  Platform
} from 'react-native';
import * as Location from 'expo-location';
import BleManager from 'react-native-ble-manager';
import ZoneDisplay from './ZoneDisplay';
import { 
  TARGET_SERVICE_UUID, 
  RSSI_THRESHOLDS, 
  BLE_CONFIG,
  ZONE_CONFIG,
  ALERTS 
} from './config';
import { calculateZone, getIntensityDescription, smoothRssi } from './utils';
import type { Zone, BleDevice } from './types';

// Define zones based on configuration
const ZONES: Zone[] = [
  { 
    name: 'Cold', 
    text: ZONE_CONFIG.cold.message, 
    color: ZONE_CONFIG.cold.color, 
    rssiRange: [-100, RSSI_THRESHOLDS.cold] 
  },
  { 
    name: 'Warm', 
    text: ZONE_CONFIG.warm.message, 
    color: ZONE_CONFIG.warm.color, 
    rssiRange: [RSSI_THRESHOLDS.cold, RSSI_THRESHOLDS.warm] 
  },
  { 
    name: 'Hot', 
    text: ZONE_CONFIG.hot.message, 
    color: ZONE_CONFIG.hot.color, 
    rssiRange: [RSSI_THRESHOLDS.warm, 0] 
  },
];

const BleTracker: React.FC = () => {
  const [scanning, setScanning] = useState<boolean>(false);
  const [deviceFound, setDeviceFound] = useState<boolean>(false);
  const [rssiValue, setRssiValue] = useState<number>(-100);
  const [currentZone, setCurrentZone] = useState<Zone>(ZONES[0]);
  const [permissionsGranted, setPermissionsGranted] = useState<boolean>(false);
  const [loading, setLoading] = useState<boolean>(true);
  const [deviceName, setDeviceName] = useState<string>('Unknown Device');
  const colorAnimation = useRef(new Animated.Value(0)).current;
  const scannedDevices = useRef<Map<string, BleDevice>>(new Map());
  const rssiHistoryRef = useRef<number[]>([]);
  const bleScanSubscriptionRef = useRef<any>(null);
  const rescanIntervalRef = useRef<NodeJS.Timeout | null>(null);

  // Request permissions on mount
  useEffect(() => {
    requestPermissions();
  }, []);

  const requestPermissions = async () => {
    try {
      if (Platform.OS === 'ios') {
        const { status } = await Location.requestForegroundPermissionsAsync();
        if (status ===ALERTS.permissionDenied, ALERTS.locationPermissionMessage
          setPermissionsGranted(true);
        } else {
          Alert.alert('Permission Denied', 'Location permission is required for Bluetooth scanning on iOS');
        }
      } else if (Platform.OS === 'android') {
        const bluetoothScan = await PermissionsAndroid.request(
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          {
            title: 'Bluetooth Permission',
            message: 'This app needs Bluetooth permission to scan for devices',
            buttonPositive: 'OK',
          }
        );

        const bluetoothConnect = await PermissionsAndroid.request(
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
          {
            title: 'Bluetooth Permission',
            message: 'This app needs permission to connect to Bluetooth devices',
            buttonPositive: 'OK',
          }
        );

        const locationPermission = await PermissionsAndroid.request(
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
          {
            title: 'Location Permission',
            message: 'Location permission is required for Bluetooth scanning',
            buttonPositive: 'OK',
          }
        );

        if (
          bluetoothScan === PermissionsAndroid.RESULTS.GRANTED &&
          bluetoothConnect === PermissionsAndroid.RESULTS.GRANTED &&
          locationPermission === PermissionsAndroid.RESULTS.GRANTED
        ) {
          setPermissionsGranted(true);
        } else {
          Alert.alert(ALERTS.permissionDenied, 'Required permissions were not granted');
        }
      }
    } catch (error) {
      console.error('Permission error:', error);
      Alert.alert('Permission Error', 'Failed to request permissions');
    } finally {
      setLoading(false);
    }
  };

  const initiateBleManager = async () => {
    try {
      await BleManager.start({ showAlert: false });
      console.log('BleManager initialized');
      startScanning();
    } catch (error) {
      console.error('BleManager initialization error:', error);
      Alert.alert(ALERTS.bluetoothError, ALERTS.bluetoothInitError);
    }
  };

  const startScanning = async () => {
    try {
      setScanning(true);
      scannedDevices.current.clear();
      rssiHistoryRef.current = [];
      
      // Handle discovered devices
      const handleDiscoverPeripheral = (device: any) => {
        console.log('Device discovered:', device.name, 'RSSI:', device.rssi);
        
        // Check if device has the target service UUID
        const serviceUUIDs = device.serviceUUIDs || [];
        const hasTargetService = serviceUUIDs.some((uuid: string) => 
          uuid.toLowerCase() === TARGET_SERVICE_UUID.toLowerCase()
        );

        if (hasTargetService) {
          if (!deviceFound) {
            setDeviceFound(true);
            setDeviceName(device.name || 'Unknown Device');
          }
          
          // Smooth RSSI value
          const smoothedRssi = rssiHistoryRef.current.length > 0
            ? smoothRssi(device.rssi, rssiHistoryRef.current[rssiHistoryRef.current.length - 1], 0.3)
            : device.rssi;
          
          // Keep history for averaging (last 5 readings)
          rssiHistoryRef.current.push(smoothedRssi);
          if (rssiHistoryRef.current.length > 5) {
            rssiHistoryRef.current.shift();
          }
          
          // Update RSSI
          setRssiValue(Math.round(smoothedRssi));
          updateZone(smoothedRssi);
        }
      };

      // Subscribe to device discovery
    const newZone = calculateZone(rssi, RSSI_THRESHOLDS, ZONES);   clearInterval(rescanIntervalRef.current);
      }
      
      rescanIntervalRef.current = setInterval(async () => {
        try {
          await BleManager.scan([], BLE_CONFIG.scanDuration, BLE_CONFIG.allowDuplicates);
          console.log('Rescan triggered');
        } catch (error) {
          console.error('Rescan error:', error);
        }
      }, BLE_CONFIG.rescanInterval);

    } catch (error) {
      console.error('Scanning error:', error);
      setScanning(false);
    }
  };

  const updateZone = (rssi: number) => {
    let newZone = ZONES[0];
    
    if (rssi > -60) {
      newZone = ZONES[2]; // Hot
    } else if (rssi > -80) {
      newZone = ZONES[1]; // Warm
    } else {
      newZone = ZONES[0]; // Cold
    }
// Cleanup function
  const cleanup = () => {
    if (rescanIntervalRef.current) {
      clearInterval(rescanIntervalRef.current);
    }
    if (bleScanSubscriptionRef.current) {
      bleScanSubscriptionRef.current.remove();
    }
    try {
      BleManager.stopScan();
    } catch (error) {
      console.error('Error stopping scan:', error);
    }
  };

  useEffect(() => {
    if (permissionsGranted && !scanning) {
      initiateBleManager();
    }

    return () => {
      cleanup();
    }; // Trigger color animation
      Animated.sequence([
        Animated.timing(colorAnimation, {
          toValue: 1,
          duration: 300,
          useNativeDriver: false,
        }),
        Animated.timing(colorAnimation, {
          toValue: 0,
          duration: 300,
          useNativeDriver: false,
        }),
      ]).start();
    }
  };

  useEffect(() => {
    if (permissionsGranted && !scanning) {
      initiateBleManager();
    }
  }, [permissionsGranted]);

  if (loading) {
    return (
      <View style={styles.container}>
        <ActivityIndicator size="large" color="#1E90FF" />
        <Text style={styles.loadingText}>Requesting Permissions...</Text>
      </View>
    );
  }

  if (!permissionsGranted) {
    return (
      <View style={styles.container}>
        <Text style={styles.errorText}>Permissions not granted</Text>
        <Text style={styles.errorSubText}>Please grant the required permissions to use this app</Text>
      </View>
    );
  }

  return (
    <View style={styles.mainContainer}>
      <ZoneDisplay 
        zone={currentZone} 
        rssi={rssiValue}
        deviceFound={deviceFound}
        deviceName={deviceName}
        scanning={scanning}
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#f0f0f0',
  },
  mainContainer: {
    flex: 1,
  },
  loadingText: {
    marginTop: 10,
    fontSize: 16,
    color: '#333',
  },
  errorText: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#dc143c',
    marginBottom: 10,
  },
  errorSubText: {
    fontSize: 14,
    color: '#666',
    textAlign: 'center',
    paddingHorizontal: 20,
  },
});

export default BleTracker;
