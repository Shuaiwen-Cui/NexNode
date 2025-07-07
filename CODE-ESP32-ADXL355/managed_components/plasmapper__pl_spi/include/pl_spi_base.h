#pragma once
#include "pl_common.h"
#include "driver/spi_master.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief SPI class
class Spi : public Lockable {
  friend class SpiController;

public:
  /// @brief Default max transaction size in bytes
  static const int defaultMaxTransactionSize = 32;

  /// @brief Creates an SPI
  /// @param host SPI host (HSPI_HOST or VSPI_HOST)
  /// @param mosiPin MOSI pin
  /// @param misoPin MISO pin
  /// @param sclkPin SCLK pin
  Spi(spi_host_device_t host, int mosiPin, int misoPin, int sclkPin);
  ~Spi();
  Spi(const Spi&) = delete;
  Spi& operator=(const Spi&) = delete;
  
  esp_err_t Lock(TickType_t timeout = portMAX_DELAY) override;
  esp_err_t Unlock() override;

  /// @brief Initializes the SPI
  /// @return error code
  esp_err_t Initialize();

  /// @brief Sets the max transaction size (may be called only before Initialize)
  /// @param maxTransactionSize max transaction size in bytes
  /// @return error code
  esp_err_t SetMaxTransactionSize(int maxTransactionSize);

private:
  Mutex mutex;
  spi_host_device_t host;
  int mosiPin, misoPin, sclkPin;
  int maxTransactionSize;
  bool initialized = false;
};

//==============================================================================

}