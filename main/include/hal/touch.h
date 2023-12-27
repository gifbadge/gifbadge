#pragma once

#include <cstdint>
#include <utility>

class Touch {
public:
    Touch() = default;

    ~Touch() = default;

    virtual std::pair<uint16_t, uint16_t> read() = 0;
};