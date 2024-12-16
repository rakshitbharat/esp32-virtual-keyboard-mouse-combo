import React, { useState, useEffect } from "react";
import {
  SafeAreaView,
  StyleSheet,
  View,
  Text,
  TouchableOpacity,
  PanResponder,
} from "react-native";
import { BleManager } from "react-native-ble-plx";

const ESP32_SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const KEYBOARD_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const MOUSE_CHAR_UUID = "beb5483f-36e1-4688-b7f5-ea07361b26a8";

const bleManager = new BleManager();

const App = () => {
  const [device, setDevice] = useState(null);
  const [isConnected, setIsConnected] = useState(false);

  useEffect(() => {
    // Request necessary permissions here
    const subscription = bleManager.onStateChange((state) => {
      if (state === "PoweredOn") {
        scanAndConnect();
      }
    }, true);

    return () => {
      subscription.remove();
      if (device) {
        device.cancelConnection();
      }
    };
  }, []);

  const scanAndConnect = async () => {
    try {
      bleManager.startDeviceScan(null, null, (error, scannedDevice) => {
        if (error) {
          console.error(error);
          return;
        }

        if (scannedDevice.name === "ESP32-HID-Controller") {
          bleManager.stopDeviceScan();
          connectToDevice(scannedDevice);
        }
      });
    } catch (error) {
      console.error("Scan error:", error);
    }
  };

  const connectToDevice = async (scannedDevice) => {
    try {
      const connectedDevice = await scannedDevice.connect();
      await connectedDevice.discoverAllServicesAndCharacteristics();
      setDevice(connectedDevice);
      setIsConnected(true);
    } catch (error) {
      console.error("Connection error:", error);
    }
  };

  // Trackpad handling
  const panResponder = PanResponder.create({
    onStartShouldSetPanResponder: () => true,
    onMoveShouldSetPanResponder: () => true,
    onPanResponderMove: (evt, gestureState) => {
      if (device && isConnected) {
        // Send mouse movement data
        const command = `move:${Math.round(gestureState.dx)},${Math.round(
          gestureState.dy
        )}`;
        sendMouseCommand(command);
      }
    },
    onPanResponderRelease: () => {
      // Handle release if needed
    },
  });

  const sendKeyboardCommand = async (key) => {
    if (device && isConnected) {
      try {
        const service = await device.services(ESP32_SERVICE_UUID);
        const characteristic = await service[0].characteristics(
          KEYBOARD_CHAR_UUID
        );
        await characteristic[0].writeWithResponse(`key:${key}`);
      } catch (error) {
        console.error("Send keyboard command error:", error);
      }
    }
  };

  const sendMouseCommand = async (command) => {
    if (device && isConnected) {
      try {
        const service = await device.services(ESP32_SERVICE_UUID);
        const characteristic = await service[0].characteristics(
          MOUSE_CHAR_UUID
        );
        await characteristic[0].writeWithResponse(command);
      } catch (error) {
        console.error("Send mouse command error:", error);
      }
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.statusContainer}>
        <Text style={styles.statusText}>
          Status: {isConnected ? "Connected" : "Disconnected"}
        </Text>
      </View>

      {/* Trackpad Area */}
      <View {...panResponder.panHandlers} style={styles.trackpadArea}>
        <Text style={styles.trackpadText}>Trackpad Area</Text>
      </View>

      {/* Simple Keyboard */}
      <View style={styles.keyboardContainer}>
        {["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"].map((key) => (
          <TouchableOpacity
            key={key}
            style={styles.key}
            onPress={() => sendKeyboardCommand(key)}
          >
            <Text style={styles.keyText}>{key}</Text>
          </TouchableOpacity>
        ))}
      </View>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: "#F5F5F5",
  },
  statusContainer: {
    padding: 10,
    backgroundColor: "#E0E0E0",
  },
  statusText: {
    fontSize: 16,
    textAlign: "center",
  },
  trackpadArea: {
    flex: 1,
    margin: 20,
    backgroundColor: "#FFFFFF",
    borderRadius: 10,
    justifyContent: "center",
    alignItems: "center",
    elevation: 3,
    shadowColor: "#000",
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.25,
    shadowRadius: 3.84,
  },
  trackpadText: {
    color: "#888888",
  },
  keyboardContainer: {
    flexDirection: "row",
    flexWrap: "wrap",
    padding: 10,
    justifyContent: "center",
  },
  key: {
    width: 40,
    height: 40,
    margin: 2,
    backgroundColor: "#FFFFFF",
    justifyContent: "center",
    alignItems: "center",
    borderRadius: 5,
    elevation: 2,
    shadowColor: "#000",
    shadowOffset: { width: 0, height: 1 },
    shadowOpacity: 0.2,
    shadowRadius: 1.41,
  },
  keyText: {
    fontSize: 18,
  },
});

export default App;
