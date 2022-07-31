//
//  ControllerDevice.cpp
//  ControllerSupport360
//
//  Created by Skyler Calaman on 7/20/22.
//


#include <os/log.h>
#include <chrono>

#include <DriverKit/IOUserServer.h>
#include <DriverKit/IOLib.h>
#include <DriverKit/OSCollections.h>
#include <HIDDriverKit/HIDDriverKit.h>

#include "ControllerDevice.h"

#define LOG_PREFIX "hi"

struct ControllerDevice_IVars {};

bool ControllerDevice::init() {
  os_log(OS_LOG_DEFAULT, "ControllerDevice init");

  if (!super::init()) {
    return false;
  }

  ivars = IONewZero(ControllerDevice_IVars, 1);
  if (ivars == nullptr) {
    return false;
  }

  return true;
}

void ControllerDevice::free() {
  os_log(OS_LOG_DEFAULT, LOG_PREFIX " free");

  IOSafeDeleteNULL(ivars, ControllerDevice_IVars, 1);

  super::free();
}

kern_return_t IMPL(ControllerDevice, Start) {

  auto kr = Start(provider, SUPERDISPATCH);
  if (kr != kIOReturnSuccess) {
    os_log(OS_LOG_DEFAULT, LOG_PREFIX " super::Start failed: 0x%x", kr);
    Stop(provider, SUPERDISPATCH);
    return kr;
  }

  RegisterService();

  return kIOReturnSuccess;
}

kern_return_t IMPL(ControllerDevice, Stop) {
  os_log(OS_LOG_DEFAULT, LOG_PREFIX " Stop");

  return Stop(provider, SUPERDISPATCH);
}

