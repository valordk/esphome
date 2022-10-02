#include "mitutoyo.h"
#include "esphome/core/log.h"

namespace esphome {
namespace mitutoyo {

static const char *const TAG = "mitutoyo";

bool IRAM_ATTR DigimaticProcessor::decode(uint32_t ms, bool data) {
  /// Check if a new measurement has started, based on time since previous bit
  if ((ms - this->prev_ms_) > DIGIMATIC_MAX_BIT_WAIT_MS) {
    this->num_bits_ = 0;
  }
  this->prev_ms_ = ms;

  /// Read each four-bit nibbles
  if (this->num_bits_ < DIGIMATIC_FRAME_LEN) {
    int idx = this->num_bits_ / DIGIMATIC_DIGIT_BITS_LEN;
    this->buffer_[idx] = (this->buffer_[idx] >> 1) | (data ? 8 : 0);
    this->num_bits_++;

    /// Parse measurement value if a full frame is read
    if (this->num_bits_ == DIGIMATIC_FRAME_LEN) {
      uint16_t temp_value = (
        ((int)this->buffer_[5] * 100000
        + (int)this->buffer_[6] * 10000
        + (int)this->buffer_[7] * 1000
        + (int)this->buffer_[8] * 100
        + (int)this->buffer_[9] * 10
        + (int)this->buffer_[10])
      );

      this->measurement->decimal_point = this->buffer_[11];
      this->measurement->value = temp_value;
      this->measurement->is_positive = (buffer_[4] == 0);

      /// Cleanup for the next reading cycle
      memset(this->buffer_, 0, sizeof this->buffer_);
      this->num_bits_ = 0;

      return true;
    }
  }

  return false;
}

void MitutoyoStore::setup(InternalGPIOPin *pin_clock, InternalGPIOPin *pin_data, InternalGPIOPin *pin_trigger) {
  pin_clock->setup();
  pin_data->setup();
  this->pin_clock_ = pin_clock->to_isr();
  this->pin_data_ = pin_data->to_isr();
  pin_clock->attach_interrupt(MitutoyoStore::interrupt, this, gpio::INTERRUPT_FALLING_EDGE);

}

void IRAM_ATTR MitutoyoStore::interrupt(MitutoyoStore *arg) {
  bool data_bit = arg->pin_data_.digital_read();
  bool clock_bit = arg->pin_clock_.digital_read();
  if (arg->is_reading && !clock_bit){
    uint32_t now = millis();    
    if (arg->processor_.decode(now, data_bit)) {
      arg->set_data_(arg->processor_.measurement);
      arg->clear_is_reading_();
    }
  }
  arg->pin_clock_.clear_interrupt();

}

void IRAM_ATTR MitutoyoStore::set_data_(MitutoyoMeasurement *measurement) {
  this->last_reading = measurement->value;
  this->last_reading_decimal_point = measurement->decimal_point;
  this->last_reading_is_positive = measurement->is_positive;
}

void IRAM_ATTR MitutoyoStore::mark_is_reading_() {
  this->is_reading = true;
}

void IRAM_ATTR MitutoyoStore::clear_is_reading_() {
  this->is_reading = false;
}


void MitutoyoInstrument::dump_config() {
  ESP_LOGCONFIG(TAG, "MitutoyoInstrument:");
  LOG_PIN("  Pin Clock: ", this->pin_clock_);
  LOG_PIN("  Pin Data: ", this->pin_data_);
  LOG_PIN("  Pin Req: ", this->pin_trigger_);
  LOG_UPDATE_INTERVAL(this);
}

void MitutoyoInstrument::update() {
  ESP_LOGD(TAG, "Initiating measurement");
  store_.mark_is_reading_();
  this->pin_trigger_->digital_write(true);
  delayMicroseconds(10000);
  for (uint8_t i = 0; i < 28; i++) {
    /// Avoid unnessarily long waiting and disable the trigger once the data is read
    if (!store_.is_reading) {
      break;
    }
    delayMicroseconds(5000);
  }

  this->pin_trigger_->digital_write(false);  
  if (!store_.is_reading) { 
    /// Reading stopped successfully    
    this->publish_state(
      (store_.last_reading / (store_.last_reading_is_positive ? 1.0f:-1.0f)) / pow10[store_.last_reading_decimal_point]
    );
  } else {
    ESP_LOGW(TAG, "Timeout reading measurement.");
  }

}

}  // namespace mitutoyo
}  // namespace esphome
