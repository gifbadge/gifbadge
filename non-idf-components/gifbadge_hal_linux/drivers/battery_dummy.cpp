#include "drivers/battery_dummy.h"
battery_dummy::battery_dummy() = default;
void battery_dummy::poll() {

}
double battery_dummy::BatteryVoltage() {
  return 4.2;
}
int battery_dummy::BatterySoc() {
  return 100;
}
void battery_dummy::BatteryRemoved() {

}
void battery_dummy::BatteryInserted() {

}
Battery::State battery_dummy::BatteryStatus() {
  return Battery::State::OK;
}
