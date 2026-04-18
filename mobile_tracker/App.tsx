import React from 'react';
import { StatusBar } from 'expo-status-bar';
import BleTracker from './app/BleTracker';

export default function App() {
  return (
    <>
      <StatusBar barStyle="light-content" />
      <BleTracker />
    </>
  );
}
