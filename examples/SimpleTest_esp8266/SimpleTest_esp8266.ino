#include <QuickChargeForArduino.h>

/*
 * Hardware requirments:
 * 1. any esp8266 board
 * 2. two 2.2 kOhm resistors (or series-parallel assembly of resistors with same resulting value)
 * 3. two 10 kOhm resistors (or series-parallel assembly of resistors with same resulting value)
 * 4. voltage divider for your esp8266 board, that convert A0 pin range up
 *    to 15V for Class A (5V, 9V, 12V) and 25V for Class B (5V, 9V, 12V, 20V)
 * 5. Any USB connector for connection to QuickCharge power source
 */

#define DP_2K2_PIN  12
#define DP_10K_PIN  14
#define DM_2K2_PIN  13
#define DM_10K_PIN  5

/*
 * Uncoment this for setup adcZeroOffset and KU
 */
//#define ADC_SETUP   1

/*
 * Short A0 to GND. Write ADC value to adcZeroOffset.
 * Reprogram MCU with sketch with new adcZeroOffset value.
 * Unshort A0. Connect some DC power source to voltage divider on A0.
 * Devide actual power source voltage by ADC value. Write result to the KU value.
 */
const int adcZeroOffset = 12;
const float KU = 0.022003745;

float adc2voltage(int adcValue) {
  return (adcValue - adcZeroOffset)*KU;
};

#if !defined(ADC_SETUP)
QCClient3v3GPIOClass QCPowerSupply(
  DP_2K2_PIN,
  DP_10K_PIN,
  DM_2K2_PIN,
  DM_10K_PIN
);

void setup() {
  Serial.begin(115200);
  Serial.println(F(""));

  Serial.println(F("Waiting A0"));
  while (analogRead(A0) < 230) {
    ESP.wdtFeed();
  };

  delay(1000);
  char powerSupplyVersion = QCPowerSupply.begin();
  Serial.print(F("Power supply QC version: "));
  switch (powerSupplyVersion) {
    case NonQCVersion:
      Serial.println(F("non QC"));
      break;
    case Qc10Version:
      Serial.println(F("1.0"));
      break;
    case Qc20Version:
      Serial.println(F("2.0"));
      break;
    case Qc30Version:
      Serial.println(F("3.0"));
      break;
    case Qc20orQc30Version:
      Serial.println(F("2.0 or 3.0"));
      break;
  }
};

void printVoltages() {
  int adcValue = analogRead(A0);
  Serial.print(F("Target voltage: "));
  Serial.print(QCPowerSupply.getTargetVoltage());
  Serial.println(F("mV"));

  adcValue = (adcValue + analogRead(A0) + analogRead(A0) + analogRead(A0)) / 4;
  Serial.print(F("ADC voltage: "));
  Serial.println(adc2voltage(adcValue));
  
};

void loop() {
  byte powerSupplyVersion = QCPowerSupply.getVersion();

  if ((powerSupplyVersion == NonQCVersion) || (powerSupplyVersion == Qc10Version)) {
    Serial.println(F("nothing"));
    return;
  }

  byte presentMode = QCPowerSupply.getMode();
  uint16_t presentTargetVolatage = QCPowerSupply.getTargetVoltage();

  Serial.print(F("Present Target voltage: "));
  Serial.println(presentTargetVolatage);

  switch (presentMode) {
    case NonQCMode:
      return;
      break;
    case Qc5V:
      QCPowerSupply.setMode(Qc9V);
      break;
    case Qc9V:
      QCPowerSupply.setMode(Qc12V);
      break;
    case Qc12V:
    case QcContinuous:
      if (presentTargetVolatage > 8000) {
        QCPowerSupply.continuousModeSetTargetVoltage(presentTargetVolatage-1000);
      } else if (presentTargetVolatage > 7000) {
        QCPowerSupply.continuousModeSetTargetVoltage(presentTargetVolatage-200);
      } else {
        QCPowerSupply.setMode(Qc5V);
      }
      break;
  }
  
  printVoltages();
  delay(1000);
};
#else 
void setup() {
  Serial.begin(115200);
}

void printADC() {
  int adcValue = analogRead(A0);
  adcValue = (adcValue + analogRead(A0) + analogRead(A0) + analogRead(A0)) / 4;
  Serial.print(F("ADC: "));
  Serial.print(adcValue);
  Serial.print(F(" voltage: "));
  Serial.println(adc2voltage(adcValue));
};

void loop() {
  printADC();
  delay(500);
}
#endif
