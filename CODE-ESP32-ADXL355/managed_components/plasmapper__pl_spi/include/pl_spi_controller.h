#pragma once
#include "pl_spi_base.h"

//==============================================================================

namespace PL {

//==============================================================================

/// @brief SPI controller class
class SpiController : public Lockable {
public:
  /// @brief Creates an SPI controller
  /// @param spi SPI interface
  /// @param mode SPI mode (0, 1, 2 or 3)
  /// @param sclkFrequency SCLK frequency in Hz
  /// @param csPin CS pin
  SpiController(std::shared_ptr<Spi> spi, uint8_t mode, int sclkFrequency, int csPin);
  ~SpiController();
  SpiController(const SpiController&) = delete;
  SpiController& operator=(const SpiController&) = delete;
  
  esp_err_t Lock(TickType_t timeout = portMAX_DELAY) override;
  esp_err_t Unlock() override;

  /// @brief Initializes the controller
  /// @return error code
  esp_err_t Initialize();

  /// @brief Executes SPI controller transaction
  /// @param command command bits
  /// @param address address bits
  /// @param writeData data bits to write to the target (can be NULL)
  /// @param readData data bits read from the target (can be NULL)
  /// @param dataSize data size in bits
  /// @return error code
  esp_err_t Transaction(uint16_t command, uint64_t address, const void* writeData, void* readData, size_t dataSize);

  /// @brief Sets the number of command bits (first written bits before read, may be called only before Initialize)
  /// @param numberOfCommandBits number of command bits (0..16)
  esp_err_t SetNumberOfCommandBits(int numberOfCommandBits);

  /// @brief Sets the number of address bits (bits written after command bits before read, may be called only before Initialize)
  /// @param numberOfAddressBits number of address bits (0..64)
  esp_err_t SetNumberOfAddressBits(int numberOfAddressBits);

  /// @brief Sets the max delay between the SCLK edge and the valid MISO level (may be called only before Initialize)
  /// @param delay delay in ns
  esp_err_t SetMaxSclkMisoDelay(int delay);

private:
  Mutex mutex;
  spi_device_handle_t deviceHandle = NULL;
  std::shared_ptr<Spi> spi;
  uint8_t mode;
  int sclkFrequency;
  int csPin;
  uint8_t numberOfCommandBits = 0, numberOfAddressBits = 0;
  int maxSclkMisoDelay = 0;
};

//==============================================================================

}