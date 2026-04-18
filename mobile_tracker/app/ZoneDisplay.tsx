import React, { useEffect, useRef, useState } from 'react';
import {
  View,
  Text,
  StyleSheet,
  Animated,
  Dimensions,
  ActivityIndicator,
} from 'react-native';
import { getIntensityDescription } from './utils';
import type { Zone } from './types';

interface ZoneDisplayProps {
  zone: Zone;
  rssi: number;
  deviceFound: boolean;
  deviceName: string;
  scanning: boolean;
}

const ZoneDisplay: React.FC<ZoneDisplayProps> = ({
  zone,
  rssi,
  deviceFound,
  deviceName,
  scanning,
}) => {
  const { width, height } = Dimensions.get('window');
  const colorAnimation = useRef(new Animated.Value(0)).current;
  const scaleAnimation = useRef(new Animated.Value(1)).current;
  const [prevZone, setPrevZone] = useState<Zone>(zone);

  // Animate when zone changes
  useEffect(() => {
    if (zone !== prevZone) {
      setPrevZone(zone);
      
      // Scale animation for zone change
      Animated.sequence([
        Animated.timing(scaleAnimation, {
          toValue: 1.1,
          duration: 200,
          useNativeDriver: true,
        }),
        Animated.timing(scaleAnimation, {
          toValue: 1,
          duration: 200,
          useNativeDriver: true,
        }),
      ]).start();

      // Color animation
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
  }, [zone]);

  // Calculate background color with smooth transition
  const backgroundColor = colorAnimation.interpolate({
    inputRange: [0, 0.5, 1],
    outputRange: [zone.color, '#ffffff', zone.color],
  });

  // Get intensity based on RSSI (removed this function as we now use utility)
  // See utils.ts for getIntensityDescription

  return (
    <Animated.View
      style={[
        styles.container,
        {
          backgroundColor: deviceFound ? backgroundColor : '#CCCCCC',
        },
      ]}
    >
      <View style={styles.topSection}>
        <Text style={styles.headerText}>BLE Tracker</Text>
        <Text style={styles.deviceNameText}>{deviceName}</Text>
        {scanning && !deviceFound && (
          <View style={styles.scanningIndicator}>
            <ActivityIndicator size="small" color="#fff" />
            <Text style={styles.scanningText}>Scanning...</Text>
          </View>
        )}
      </View>

      <View style={styles.contentSection}>
        <Animated.View
          style={[
            styles.rssiContainer,
            {
              transform: [{ scale: scaleAnimation }],
            },
          ]}
        >
          <Text style={styles.rssiLabel}>Signal Strength</Text>
          <Animated.Text style={[styles.rssiValue, { color: zone.color }]}>
            {rssi}
          </Animated.Text>
          <Text style={styles.rssiUnit}>dBm</Text>
          <Text style={styles.intensityText}>{getIntensityDescription(rssi)}</Text>
        </Animated.View>

        {deviceFound && (
          <View style={styles.zoneContainer}>
            <Animated.Text style={[styles.zoneText, { color: zone.color }]}>
              {zone.text}
            </Animated.Text>
            <Text style={styles.zoneLabel}>{zone.name} Zone</Text>
          </View>
        )}

        {!deviceFound && (
          <View style={styles.searchingContainer}>
            <ActivityIndicator size="large" color="#ffffff" />
            <Text style={styles.searchingText}>Searching for device...</Text>
          </View>
        )}
      </View>

      <View style={styles.bottomSection}>
        <View style={styles.zoneIndicators}>
          <View style={[styles.indicator, { backgroundColor: '#1E90FF' }]}>
            <Text style={styles.indicatorLabel}>Cold</Text>
            <Text style={styles.indicatorValue}>&lt; -80</Text>
          </View>
          <View style={[styles.indicator, { backgroundColor: '#FFA500' }]}>
            <Text style={styles.indicatorLabel}>Warm</Text>
            <Text style={styles.indicatorValue}>-80 to -60</Text>
          </View>
          <View style={[styles.indicator, { backgroundColor: '#DC143C' }]}>
            <Text style={styles.indicatorLabel}>Hot</Text>
            <Text style={styles.indicatorValue}>&gt; -60</Text>
          </View>
        </View>
      </View>
    </Animated.View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'space-between',
    paddingTop: 60,
    paddingBottom: 40,
    paddingHorizontal: 20,
  },
  topSection: {
    alignItems: 'center',
    marginBottom: 20,
  },
  headerText: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#ffffff',
    textShadowColor: 'rgba(0, 0, 0, 0.3)',
    textShadowOffset: { width: 1, height: 1 },
    textShadowRadius: 3,
  },
  deviceNameText: {
    fontSize: 16,
    color: '#ffffff',
    marginTop: 8,
    opacity: 0.9,
  },
  scanningIndicator: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 10,
    paddingHorizontal: 15,
    paddingVertical: 8,
    backgroundColor: 'rgba(255, 255, 255, 0.2)',
    borderRadius: 20,
  },
  scanningText: {
    color: '#ffffff',
    fontSize: 12,
    marginLeft: 8,
  },
  contentSection: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  rssiContainer: {
    alignItems: 'center',
    marginBottom: 40,
  },
  rssiLabel: {
    fontSize: 18,
    color: '#ffffff',
    marginBottom: 10,
    opacity: 0.9,
  },
  rssiValue: {
    fontSize: 90,
    fontWeight: 'bold',
    textShadowColor: 'rgba(0, 0, 0, 0.2)',
    textShadowOffset: { width: 2, height: 2 },
    textShadowRadius: 4,
  },
  rssiUnit: {
    fontSize: 20,
    color: '#ffffff',
    marginTop: 10,
    opacity: 0.8,
  },
  intensityText: {
    fontSize: 14,
    color: '#ffffff',
    marginTop: 8,
    opacity: 0.7,
    letterSpacing: 1,
  },
  zoneContainer: {
    alignItems: 'center',
  },
  zoneText: {
    fontSize: 48,
    fontWeight: 'bold',
    textShadowColor: 'rgba(0, 0, 0, 0.3)',
    textShadowOffset: { width: 2, height: 2 },
    textShadowRadius: 4,
    marginBottom: 10,
  },
  zoneLabel: {
    fontSize: 16,
    color: '#ffffff',
    opacity: 0.8,
    letterSpacing: 1,
  },
  searchingContainer: {
    alignItems: 'center',
  },
  searchingText: {
    fontSize: 18,
    color: '#ffffff',
    marginTop: 20,
    opacity: 0.9,
  },
  bottomSection: {
    marginTop: 30,
  },
  zoneIndicators: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    gap: 10,
  },
  indicator: {
    flex: 1,
    paddingVertical: 12,
    paddingHorizontal: 10,
    borderRadius: 8,
    alignItems: 'center',
  },
  indicatorLabel: {
    fontSize: 12,
    fontWeight: 'bold',
    color: '#ffffff',
    marginBottom: 4,
  },
  indicatorValue: {
    fontSize: 11,
    color: '#ffffff',
    opacity: 0.8,
  },
});

export default ZoneDisplay;
