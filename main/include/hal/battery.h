#pragma once

struct BatteryStatus {
    double voltage;
    int soc;
    float rate;
};

class Battery{
public:
    Battery() = default;

    virtual ~Battery() = default;

    virtual BatteryStatus read() = 0;

    virtual int pollInterval() = 0;
};