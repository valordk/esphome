#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace mitutoyo {

static const uint8_t DIGIMATIC_MAX_BIT_WAIT_MS = 30;
static const uint8_t DIGIMATIC_DIGIT_BITS_LEN = 4; /// bits in a single Mitutoyo Digimatic nibble
static const uint8_t DIGIMATIC_FRAME_LEN = 52; /// bits in a Mitutoyo Digimatic response
static const int pow10[] = {1, 10, 100, 1000, 10000, 100000}; /// fast lookup table for 10's powers

struct MitutoyoMeasurement {
  uint16_t value = 0;
  int decimal_point = 0;
  bool is_positive = true;
};

class DigimaticProcessor {
 public:
  bool decode(uint32_t ms, bool data);
  MitutoyoMeasurement *measurement = new MitutoyoMeasurement;

 protected:
  uint8_t buffer_[DIGIMATIC_FRAME_LEN / DIGIMATIC_DIGIT_BITS_LEN];
  uint8_t num_bits_ = 0;
  uint32_t prev_ms_;
};

class MitutoyoStore {
 public:
  uint16_t last_reading;
  int last_reading_decimal_point;
  bool last_reading_is_positive;
  bool is_reading;
  bool is_first_bit_received;

  void setup(InternalGPIOPin *pin_clock, InternalGPIOPin *pin_data, InternalGPIOPin *pin_trigger);
  static void interrupt(MitutoyoStore *arg);
  void mark_is_reading_();
  void clear_is_reading_();

 protected:
  ISRInternalGPIOPin pin_clock_;
  ISRInternalGPIOPin pin_data_;
  ISRInternalGPIOPin pin_trigger_;
  DigimaticProcessor processor_;
  void set_data_(MitutoyoMeasurement *measurement);

};

class MitutoyoInstrument : public PollingComponent, public sensor::Sensor {
 public:
  void set_pin_clock(InternalGPIOPin *pin) { pin_clock_ = pin; }
  void set_pin_data(InternalGPIOPin *pin) { pin_data_ = pin; }
  void set_pin_trigger(InternalGPIOPin *pin) { pin_trigger_ = pin; }
  void enable_polling() { polling_enabled_ = true; }
  void disable_polling() { polling_enabled_ = false; }
  bool is_polling() { return polling_enabled_; }
  void set_reversed(bool reversed) { is_reversed_ = reversed; }
  bool is_reversed() { return is_reversed_; }

  void setup() override {
    this->store_.setup(this->pin_clock_, this->pin_data_, this->pin_trigger_);
    this->pin_trigger_->pin_mode(gpio::FLAG_OUTPUT);
  }
  void dump_config() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  MitutoyoStore store_;
  InternalGPIOPin *pin_clock_;
  InternalGPIOPin *pin_data_;
  InternalGPIOPin *pin_trigger_;  
  bool polling_enabled_ = true;
  bool is_reversed_ = false;
};

}  // namespace mitutoyo
}  // namespace esphome
