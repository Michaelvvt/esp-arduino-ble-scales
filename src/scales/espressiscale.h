#pragma once
#include "remote_scales.h"
#include "remote_scales_plugin_registry.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEUtils.h>
#include <NimBLEScan.h>
#include <vector>
#include <memory>

/**
 * Custom BLE service and characteristic UUIDs for EspressiScale
 * These UUIDs are unique to the EspressiScale implementation and follow
 * the standard format for custom BLE services and characteristics.
 */
#define ESPRESSISCALE_SERVICE_UUID         "19B10000-E8F2-537E-4F6C-D104768A1214"
#define ESPRESSISCALE_WEIGHT_CHAR_UUID     "19B10001-E8F2-537E-4F6C-D104768A1214"
#define ESPRESSISCALE_TIMER_CHAR_UUID      "19B10002-E8F2-537E-4F6C-D104768A1214"
#define ESPRESSISCALE_COMMAND_CHAR_UUID    "19B10003-E8F2-537E-4F6C-D104768A1214"

/**
 * Command codes for controlling the EspressiScale
 * These commands are sent over BLE to control scale functionality
 */
enum class EspressiScaleCommand : uint8_t {
  TARE = 0x01,        // Zero the scale
  START_TIMER = 0x02, // Start or resume the timer
  STOP_TIMER = 0x03,  // Pause the timer
  RESET_TIMER = 0x04  // Reset the timer to zero
};

/**
 * EspressiScales class implements RemoteScales interface for the EspressiScale device
 * Handles BLE communication, notifications, and scale-specific functionality
 */
class EspressiScales : public RemoteScales {
public:
  /**
   * Constructor for EspressiScales
   * @param device The discovered BLE device representing an EspressiScale
   */
  EspressiScales(const DiscoveredDevice& device);
  
  /**
   * Standard RemoteScales interface methods
   */
  void update() override;          // Called periodically to update state
  bool connect() override;         // Establish connection to the scale
  void disconnect() override;      // Disconnect from the scale
  bool isConnected() override;     // Check if scale is connected
  bool tare() override;            // Tare the scale (set to zero)
  
  /**
   * Additional EspressiScale-specific functions for timer control
   */
  bool startTimer();              // Start or resume the timer
  bool stopTimer();               // Pause the timer
  bool resetTimer();              // Reset the timer to zero
  float getTimer() const { return timer; } // Get current timer value

private:
  float timer = 0;                // Current timer value in seconds
  uint8_t battery = 0;            // Battery level (not currently used)

  // BLE service and characteristic pointers
  NimBLERemoteService* service;
  NimBLERemoteCharacteristic* weightCharacteristic;
  NimBLERemoteCharacteristic* timerCharacteristic;
  NimBLERemoteCharacteristic* commandCharacteristic;

  /**
   * Subscribe to weight and timer notifications from the scale
   */
  void subscribeToNotifications();
  
  /**
   * Callback for weight characteristic notifications
   * @param pBLERemoteCharacteristic The characteristic that triggered the notification
   * @param pData Data received from the notification
   * @param length Length of received data
   * @param isNotify True if this is a notification (as opposed to a read response)
   */
  void notifyWeightCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
  
  /**
   * Callback for timer characteristic notifications
   * @param pBLERemoteCharacteristic The characteristic that triggered the notification
   * @param pData Data received from the notification
   * @param length Length of received data
   * @param isNotify True if this is a notification (as opposed to a read response)
   */
  void notifyTimerCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
  
  /**
   * Send a command to the scale
   * @param command The command to send
   * @return true if command was sent successfully
   */
  bool sendCommand(EspressiScaleCommand command);
};

/**
 * Plugin for registering EspressiScales with the RemoteScalesPluginRegistry
 * Enables the library to automatically detect and use EspressiScale devices
 */
class EspressiScalesPlugin {
public:
  /**
   * Register the EspressiScales plugin with the registry
   */
  static void apply() {
    RemoteScalesPlugin plugin = RemoteScalesPlugin{
      .id = "plugin-espressiscale",
      .handles = [](const DiscoveredDevice& device) { return EspressiScalesPlugin::handles(device); },
      .initialise = [](const DiscoveredDevice& device) -> std::unique_ptr<RemoteScales> { return std::make_unique<EspressiScales>(device); },
    };
    RemoteScalesPluginRegistry::getInstance()->registerPlugin(plugin);
  }
private:
  /**
   * Check if a discovered device is an EspressiScale
   * @param device The discovered device to check
   * @return true if the device is an EspressiScale
   */
  static bool handles(const DiscoveredDevice& device) {
    const std::string& deviceName = device.getName();
    return !deviceName.empty() && deviceName.find("EspressiScale") == 0;
  }
}; 