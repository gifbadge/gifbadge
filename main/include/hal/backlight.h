#pragma once

class Backlight {
 public:
  Backlight() = default;
  virtual ~Backlight() = default;
  virtual void state(bool) = 0;
  virtual void setLevel(int) = 0;
  virtual int getLevel() = 0;
};