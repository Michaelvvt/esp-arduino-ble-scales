#include "espressiscale.h"

/**
 * Protocol description for EspressiScale:
 * 
 * The EspressiScale uses a simple BLE communication protocol:
 * 1. Weight Characteristic (19B10001-E8F2-537E-4F6C-D104768A1214):
 *    - Provides notifications with current weight as a float (4 bytes)
 * 
 * 2. Timer Characteristic (19B10002-E8F2-537E-4F6C-D104768A1214):
 *    - Provides notifications with current timer value as a float (4 bytes)
 * 
 * 3. Command Characteristic (19B10003-E8F2-537E-4F6C-D104768A1214):
 *    - Accepts single-byte commands to control scale functions:
 *      - 0x01: Tare (zero the scale)
 *      - 0x02: Start/Resume Timer
 *      - 0x03: Stop Timer
 *      - 0x04: Reset Timer
 */

//-----------------------------------------------------------------------------------/
//---------------------------        PUBLIC       -----------------------------------/
//-----------------------------------------------------------------------------------/

/**
 * Constructor for EspressiScales
 * @param device The discovered BLE device
 */
EspressiScales::EspressiScales(const DiscoveredDevice& device) : RemoteScales(device) {
  // Initialize with default values
}

/**
 * Establish connection to the EspressiScale
 * 
 * This method:
 * 1. Initiates BLE connection
 * 2. Discovers services and characteristics
 * 3. Sets up notifications
 * 
 * @return true if connection successful, false otherwise
 */
bool EspressiScales::connect() {
  log("Connecting to EspressiScale at %s", getDeviceAddress().c_str());
  
  if (!clientConnect()) {
    log("Failed to connect to EspressiScale");
    return false;
  }

  log("Connected to EspressiScale");
  
  // Get service
  service = clientGetService(NimBLEUUID(ESPRESSISCALE_SERVICE_UUID));
  if (service == nullptr) {
    log("Failed to find EspressiScale service");
    disconnect();
    return false;
  }

  // Get characteristics
  weightCharacteristic = service->getCharacteristic(NimBLEUUID(ESPRESSISCALE_WEIGHT_CHAR_UUID));
  if (weightCharacteristic == nullptr) {
    log("Failed to find weight characteristic");
    disconnect();
    return false;
  }

  timerCharacteristic = service->getCharacteristic(NimBLEUUID(ESPRESSISCALE_TIMER_CHAR_UUID));
  if (timerCharacteristic == nullptr) {
    log("Failed to find timer characteristic");
    disconnect();
    return false;
  }

  commandCharacteristic = service->getCharacteristic(NimBLEUUID(ESPRESSISCALE_COMMAND_CHAR_UUID));
  if (commandCharacteristic == nullptr) {
    log("Failed to find command characteristic");
    disconnect();
    return false;
  }

  subscribeToNotifications();
  log("Successfully connected to EspressiScale");
  return true;
}

/**
 * Subscribe to weight and timer notifications
 * 
 * Sets up callbacks for notifications from the weight and timer characteristics
 */
void EspressiScales::subscribeToNotifications() {
  if (weightCharacteristic != nullptr && weightCharacteristic->canNotify()) {
    if (!weightCharacteristic->subscribe(true, [this](NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
      this->notifyWeightCallback(pBLERemoteCharacteristic, pData, length, isNotify);
    })) {
      log("Failed to subscribe to weight notifications");
    }
  }

  if (timerCharacteristic != nullptr && timerCharacteristic->canNotify()) {
    if (!timerCharacteristic->subscribe(true, [this](NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
      this->notifyTimerCallback(pBLERemoteCharacteristic, pData, length, isNotify);
    })) {
      log("Failed to subscribe to timer notifications");
    }
  }
}

/**
 * Process weight notifications
 * 
 * Called when weight characteristic sends a notification
 * Parses the notification data and updates the current weight value
 * 
 * @param pBLERemoteCharacteristic The characteristic that sent the notification
 * @param pData Notification data
 * @param length Length of data
 * @param isNotify True if this is a notification (vs read response)
 */
void EspressiScales::notifyWeightCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (length >= 4) {
    // Decode the weight from the notification
    // The weight is sent as a float (4 bytes)
    float newWeight;
    memcpy(&newWeight, pData, sizeof(float));
    setWeight(newWeight);
  }
}

/**
 * Process timer notifications
 * 
 * Called when timer characteristic sends a notification
 * Parses the notification data and updates the current timer value
 * 
 * @param pBLERemoteCharacteristic The characteristic that sent the notification
 * @param pData Notification data
 * @param length Length of data
 * @param isNotify True if this is a notification (vs read response)
 */
void EspressiScales::notifyTimerCallback(NimBLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (length >= 4) {
    // Decode the timer from the notification
    // The timer is sent as a float (4 bytes)
    memcpy(&timer, pData, sizeof(float));
  }
}

/**
 * Send a command to the scale
 * 
 * @param command Command code to send
 * @return true if command was sent successfully
 */
bool EspressiScales::sendCommand(EspressiScaleCommand command) {
  if (!isConnected() || commandCharacteristic == nullptr) {
    return false;
  }

  uint8_t cmd = static_cast<uint8_t>(command);
  return commandCharacteristic->writeValue(&cmd, 1, true);
}

/**
 * Tare the scale (set to zero)
 * 
 * @return true if command sent successfully
 */
bool EspressiScales::tare() {
  log("Sending tare command to EspressiScale");
  return sendCommand(EspressiScaleCommand::TARE);
}

/**
 * Start or resume the timer
 * 
 * @return true if command sent successfully
 */
bool EspressiScales::startTimer() {
  log("Sending start timer command to EspressiScale");
  return sendCommand(EspressiScaleCommand::START_TIMER);
}

/**
 * Stop/pause the timer
 * 
 * @return true if command sent successfully
 */
bool EspressiScales::stopTimer() {
  log("Sending stop timer command to EspressiScale");
  return sendCommand(EspressiScaleCommand::STOP_TIMER);
}

/**
 * Reset the timer to zero
 * 
 * @return true if command sent successfully
 */
bool EspressiScales::resetTimer() {
  log("Sending reset timer command to EspressiScale");
  return sendCommand(EspressiScaleCommand::RESET_TIMER);
}

/**
 * Disconnect from the scale
 * 
 * Cleans up BLE client and resets all characteristic pointers
 */
void EspressiScales::disconnect() {
  log("Disconnecting from EspressiScale");
  clientCleanup();
  service = nullptr;
  weightCharacteristic = nullptr;
  timerCharacteristic = nullptr;
  commandCharacteristic = nullptr;
}

/**
 * Check if scale is connected
 * 
 * @return true if connected to the scale
 */
bool EspressiScales::isConnected() {
  return clientIsConnected();
}

/**
 * Update method called periodically
 * 
 * EspressiScale implementation relies on notifications for updates,
 * so this method is currently empty.
 */
void EspressiScales::update() {
  // Empty as we rely on notifications for updates
} 