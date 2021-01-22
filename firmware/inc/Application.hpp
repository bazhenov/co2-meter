#pragma once

#include "HAL/Atmel/Device.hpp"
#include "Time/RealTimer.hpp"
#include "HAL/Atmel/Power.hpp"
#include "HAL/Atmel/ADConverter.hpp"
#include "auto_field.hpp"
#include "HAL/Atmel/InterruptHandlers.hpp"
#include "Tasks/TaskState.hpp"
#include "Tasks/loop.hpp"
#include "Tasks/Task.hpp"
#include <avr/sfr_defs.h>

using namespace HAL::Atmel;
using namespace Streams;

struct Application: public Task {
  typedef Application This;
  typedef Logging::Log<Loggers::Main> log;

  Usart0 usart0 = { 9600 };
  auto_var(usartTX, PinPD1(usart0));
  auto_var(usartRX, PinPD0());
  auto_var(led, PinPB5());
  auto_var(co2_pin, PinPC1());
  auto_var(timer0, Timer0::withPrescaler<64>::inNormalMode());
  auto_var(rt, realTimer(timer0));
  auto_var(power, Power(rt));
  auto_var(adc, ADConverter<uint16_t>());

  auto_var(twiceASecond, periodic(rt, 500_ms));

  typedef Delegate<This, decltype(rt), &This::rt,
    Delegate<This, decltype(power), &This::power,
    Delegate<This, decltype(adc), &This::adc,
    Delegate<This, decltype(usartTX), &This::usartTX>>>> Handlers;

  TaskState getTaskState() {
    // Need to maintain IDLE power-down state for USART to work correctly
    return TaskState::busy(SleepMode::IDLE);
  }

  void reportTask() {
    adc.measure(co2_pin);
    auto value = adc.awaitValue();
    usartTX.writeIfSpace(FB(0x45, 0x21), value);
    led.setHigh(!led.isHigh());
  }

public:
  void main() {
    led.configureAsOutput();
    usartRX.configureAsInputWithoutPullup();
    co2_pin.configureAsInputWithoutPullup();
    
    auto reportTask = twiceASecond.invoking<This, &This::reportTask>(*this);
    while(true) {
      loopTasks(power, reportTask, *this);
    }
  }
};
