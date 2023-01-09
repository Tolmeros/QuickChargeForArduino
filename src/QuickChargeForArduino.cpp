#include "QuickChargeForArduino.h"

#define TEST_BC_IMPULSE_US      10
#define INIT_HVDCP_IMPULSE_MS   1500
#define TEST_HVDCP_IMPULSE_US   10
#define MODE_CHANGE_IMPULSE_MS  80

#define CONTINOUS_MODE_STEP_MV          200
#define CONTINOUS_MODE_STEP_IMPULSE_MS  5
#define CONTINOUS_MODE_STEP_PAUSE_MS    5

int16_t continousModeStep_mv = CONTINOUS_MODE_STEP_MV;

QCClient3v3GPIOClass::QCClient3v3GPIOClass(
                                            byte dp2k2Pin,
                                            byte dp10kPin,
                                            byte dm2k2Pin,
                                            byte dm10kPin,
                                            byte QcClass
                                          ) {
  _dp_2k2_pin = dp2k2Pin;
  _dp_10k_pin = dp10kPin;
  _dm_2k2_pin = dm2k2Pin;
  _dm_10k_pin = dm10kPin;
  _quick_charge_class = QcClass;

  pinMode(_dp_2k2_pin, INPUT);
  pinMode(_dp_10k_pin, INPUT);
  digitalWrite(_dp_2k2_pin, LOW);
  digitalWrite(_dp_10k_pin, LOW);

  pinMode(_dm_2k2_pin, INPUT);
  pinMode(_dm_10k_pin, INPUT);
  digitalWrite(_dm_2k2_pin, LOW);
  digitalWrite(_dm_10k_pin, LOW);
};

int8_t QCClient3v3GPIOClass::begin() {
  _setDPQcSignal(QcSignalHiZ);
  _setDMQcSignal(QcSignalHiZ);

  pinMode(_dp_2k2_pin, OUTPUT);
  pinMode(_dp_10k_pin, OUTPUT);
  
  _setDPQcSignal(QcSignal0v0);
  delayMicroseconds(TEST_BC_IMPULSE_US);

  pinMode(_dm_10k_pin, OUTPUT);
  digitalWrite(_dm_10k_pin, HIGH);
  if (digitalRead(_dm_2k2_pin) != LOW) {
    _setDPQcSignal(QcSignalHiZ);
    _setDMQcSignal(QcSignalHiZ);
    _quick_charge_version = NonQCVersion;
    _quick_charge_mode = NonQCMode;
    return _quick_charge_version;
  }
  digitalWrite(_dm_10k_pin, LOW);
  pinMode(_dm_10k_pin, INPUT);

  _setDPQcSignal(QcSignal0v6);
  delay(INIT_HVDCP_IMPULSE_MS);

  _setDPQcSignal(QcSignal3v3);
  delayMicroseconds(TEST_HVDCP_IMPULSE_US);
  
  if (digitalRead(_dm_2k2_pin) == HIGH) {
    _setDPQcSignal(QcSignalHiZ);
    _setDMQcSignal(QcSignalHiZ);
    _quick_charge_version = Qc10Version;
    _quick_charge_mode = Qc1versionMode;
    return _quick_charge_version;
  }

  pinMode(_dm_2k2_pin, OUTPUT);
  pinMode(_dm_10k_pin, OUTPUT);

  _setModeFast(Qc5V);

  _quick_charge_version = Qc20orQc30Version;
  _max_target_voltage_mv = (_quick_charge_class == QcClassA)
    ? QC_CLASS_A_MAX_TARGET_VOLTAGE_MV
    : QC_CLASS_B_MAX_TARGET_VOLTAGE_MV;

  return _quick_charge_version; // maybe error code ?

};

void QCClient3v3GPIOClass::_setModeFast(byte mode) {
  switch (mode) {
    case Qc5V:
      _setDPQcSignal(QcSignal0v6);
      _setDMQcSignal(QcSignal0v0);
      _target_voltage_mv = 5000;
      break;
    case Qc9V:
      _setDPQcSignal(QcSignal3v3);
      _setDMQcSignal(QcSignal0v6);
      _target_voltage_mv = 9000;
      break;
    case Qc12V:
      _setDPQcSignal(QcSignal0v6);
      _setDMQcSignal(QcSignal0v6);
      _target_voltage_mv = 12000;
      break;
    case Qc20V:
      _setDPQcSignal(QcSignal3v3);
      _setDMQcSignal(QcSignal3v3);
      _target_voltage_mv = 20000;
      break;
    case QcContinuous:
      _setDPQcSignal(QcSignal0v6);
      _setDMQcSignal(QcSignal3v3);
      break;
  };
  _quick_charge_mode = mode;
  delay(MODE_CHANGE_IMPULSE_MS);
};

bool QCClient3v3GPIOClass::setMode(byte mode) {
  if ((_quick_charge_version == NonQCVersion) ||
      (_quick_charge_version == Qc10Version)) {
    return false;
  }

  if (_quick_charge_mode == mode) {
    return false;
  }

  switch (mode) {
    case QcContinuous:
      if ((_quick_charge_version == Qc20Version)) {
        return false;
      }
      break;
    case Qc20V:
      if (_quick_charge_class != QcClassB) {
        return false;
      };
    case Qc12V:
    case Qc9V:
      if (_quick_charge_mode == QcContinuous) {
        setMode(Qc5V);
      };
      break;
  }
  _setModeFast(mode);
  return true;
};

uint8_t QCClient3v3GPIOClass::getMode() {
  return _quick_charge_mode;
};

int8_t QCClient3v3GPIOClass::continuousModeStep(int8_t steps) {
  return _continuousModeStep(steps, true);
}

int8_t QCClient3v3GPIOClass::_continuousModeStep(int8_t steps, bool checkRange) {
  if (_quick_charge_mode != QcContinuous) {
    if(!setMode(QcContinuous)) {
      return -1;
    }
  }

  if (steps == 0) {
    return 0;
  }

  uint16_t newVoltage_mv = _target_voltage_mv + (continousModeStep_mv * steps);

  if (checkRange && ((newVoltage_mv < _min_target_voltage_mv) ||
      (newVoltage_mv > _max_target_voltage_mv))) {
    return -2;
  }

  if (steps > 0) {
    for (byte step = 0; step < steps; step++) {
      _setDPQcSignal(QcSignal3v3);
      delay(CONTINOUS_MODE_STEP_IMPULSE_MS);
      _setDPQcSignal(QcSignal0v6);
      delay(CONTINOUS_MODE_STEP_PAUSE_MS);
    }
  } else {
    for (byte step = 0; step < -steps; step++) {
      _setDMQcSignal(QcSignal0v6);
      delay(CONTINOUS_MODE_STEP_IMPULSE_MS);
      _setDMQcSignal(QcSignal3v3);
      delay(CONTINOUS_MODE_STEP_PAUSE_MS);
    }
  }

  _target_voltage_mv = newVoltage_mv;

  // можливо повертати _target_voltage_mv?
  return 1;
}

int8_t QCClient3v3GPIOClass::continuousModeStepUp(byte steps) {
  return continuousModeStep(steps);
};

int8_t QCClient3v3GPIOClass::continuousModeStepDown(byte steps) {
  return continuousModeStep(-steps);
};

int8_t QCClient3v3GPIOClass::continuousModeSetTargetVoltage(uint16_t targetVoltage) {
  if ((targetVoltage < _min_target_voltage_mv) ||
      ( targetVoltage > _max_target_voltage_mv)) {
    return -2;
  }

  int8_t steps = (targetVoltage - _target_voltage_mv) / continousModeStep_mv;
  return _continuousModeStep(steps, false);
};

void QCClient3v3GPIOClass::_setDPQcSignal(byte qcSignal) {
  switch (qcSignal) {
      case QcSignal0v0:
        digitalWrite(_dp_2k2_pin, LOW);
        digitalWrite(_dp_10k_pin, LOW);
        break;
      case QcSignal0v6:
        digitalWrite(_dp_2k2_pin, LOW);
        digitalWrite(_dp_10k_pin, HIGH);
        break;
      case QcSignal3v3:
        digitalWrite(_dp_2k2_pin, HIGH);
        digitalWrite(_dp_10k_pin, HIGH);
        break;
      case QcSignalHiZ:
        pinMode(_dp_2k2_pin, INPUT);
        pinMode(_dp_10k_pin, INPUT);
        digitalWrite(_dp_2k2_pin, LOW);
        digitalWrite(_dp_10k_pin, LOW);
        break;
  }
};

void QCClient3v3GPIOClass::_setDMQcSignal(byte qcSignal) {
  switch (qcSignal) {
      case QcSignal0v0:
        digitalWrite(_dm_2k2_pin, LOW);
        digitalWrite(_dm_10k_pin, LOW);
        break;
      case QcSignal0v6:
        digitalWrite(_dm_2k2_pin, LOW);
        digitalWrite(_dm_10k_pin, HIGH);
        break;
      case QcSignal3v3:
        digitalWrite(_dm_2k2_pin, HIGH);
        digitalWrite(_dm_10k_pin, HIGH);
        break;
      case QcSignalHiZ:
        pinMode(_dm_2k2_pin, INPUT);
        pinMode(_dm_10k_pin, INPUT);
        digitalWrite(_dm_2k2_pin, LOW);
        digitalWrite(_dm_10k_pin, LOW);
        break;
  }
};

uint16_t QCClient3v3GPIOClass::getTargetVoltage() {
  return _target_voltage_mv;
};

uint8_t QCClient3v3GPIOClass::getVersion() {
  return _quick_charge_version;
};
