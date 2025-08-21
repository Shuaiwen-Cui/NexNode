#include "pl_spi.h"

//==============================================================================

const spi_host_device_t host = HSPI_HOST;
const int mosiPin = 5;
const int misoPin = 18;
const int sclkPin = 19;
const uint8_t spiMode = 0;
const int sclkFrequency = 10000000;
const int csPin = 21;
const int numberOfAddressBits = 8;
const int maxSclkMisoDelay = 30;

//==============================================================================

extern "C" void app_main(void) {
  auto spi = std::make_shared<PL::Spi>(host, mosiPin, misoPin, sclkPin);
  PL::SpiController controller(spi, spiMode, sclkFrequency, csPin);
  controller.SetNumberOfAddressBits(numberOfAddressBits);
  controller.SetMaxSclkMisoDelay(maxSclkMisoDelay);

  controller.Initialize();

  uint8_t writeData[2] = {10, 20};
  printf("Writing data:");
  for (int i = 0; i < sizeof(writeData) / sizeof(writeData[0]); i++)
    printf(" %d", writeData[i]);
  printf("\n");
  controller.Transaction(0, (0x20 << 1) | 0, writeData, NULL, sizeof(writeData) * 8);

  uint8_t readData[2] = {};
  printf("Reading data:");
  controller.Transaction(0, (0x20 << 1) | 1, NULL, readData, sizeof(readData) * 8);
  for (int i = 0; i < sizeof(readData) / sizeof(readData[0]); i++)
    printf(" %d", readData[i]);
  printf("\n");
}