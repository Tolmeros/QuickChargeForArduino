#pragma once

#include <Arduino.h>

#define INITIAL_USB_POWERSUPLY_VOLTAGE_MV   5000
#define QC_MIN_TARGET_VOLTAGE_MV            3600
#define QC_CLASS_A_MAX_TARGET_VOLTAGE_MV    12000
#define QC_CLASS_B_MAX_TARGET_VOLTAGE_MV    20000


enum QuickChargeMode {
  NonQCMode = 0,
  Qc1versionMode,
  Qc5V,
  Qc9V,
  Qc12V,
  Qc20V,
  QcContinuous,
};

enum QuickChargeVersion {
  NonQCVersion = 0,
  Qc10Version,
  Qc20Version,
  Qc30Version,
  Qc20orQc30Version,
};

enum QuickChargeClass {
  QcClassA = 0,
  QcClassB,
};

enum QuickChargeSignals {
  QcSignal0v0 = 0,
  QcSignal0v6,
  QcSignal3v3,
  QcSignalHiZ,
};

class QCClient3v3GPIOClass {
  public:
    QCClient3v3GPIOClass(
      byte dp2k2Pin,
      byte dp10kPin,
      byte dm2k2Pin,      
      byte dm10kPin,
      byte QcClass = QcClassA
    );
    int8_t begin();
    bool setMode(byte mode);
    uint8_t getMode();
    uint8_t getVersion();
    int8_t continuousModeSetTargetVoltage(uint16_t targetVoltage);
    int8_t continuousModeStepUp(byte steps=1);
    int8_t continuousModeStepDown(byte steps=1);
    int8_t continuousModeStep(int8_t steps=1);
    uint16_t getTargetVoltage();
    //float getRealVoltage();


  private:
    uint16_t _target_voltage_mv = INITIAL_USB_POWERSUPLY_VOLTAGE_MV;
    uint16_t _min_target_voltage_mv = QC_MIN_TARGET_VOLTAGE_MV;
    uint16_t _max_target_voltage_mv = QC_CLASS_A_MAX_TARGET_VOLTAGE_MV;

    byte _dp_2k2_pin, _dp_10k_pin;
    byte _dm_2k2_pin, _dm_10k_pin;
    uint8_t _quick_charge_version = NonQCVersion;
    uint8_t _quick_charge_mode = NonQCMode;
    uint8_t _quick_charge_class = QcClassA;

    void _setDPQcSignal(byte qcSignal);
    void _setDMQcSignal(byte qcSignal);
    void _setModeFast(byte mode);
    int8_t _continuousModeStep(int8_t steps=1, bool checkRange=true);
};
