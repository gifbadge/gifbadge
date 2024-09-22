#include "drivers/battery_dummy.h"
battery_dummy::battery_dummy() = default;
void battery_dummy::poll() {

}
double battery_dummy::BatteryVoltage() {
  return 4.2;
}
int battery_dummy::getSoc() {
  return 100;
}
double battery_dummy::getRate() {
  return 0;
}
void battery_dummy::removed() {

}
void battery_dummy::inserted() {

}
Battery::State battery_dummy::status() {
  return Battery::State::OK;
}
