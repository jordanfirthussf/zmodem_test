//
// Created by jordan on 2026-06-05.
//

#include "zmodem.h"

#include "zmodem_config.h"

Stream* ZModem::_serial = nullptr;

void ZModem::begin(Stream &serial) {
  _serial = &serial;
}

