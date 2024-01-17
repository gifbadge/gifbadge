#pragma once

#include <cstdint>
#include <utility>

class Touch {
public:
    Touch() = default;

    virtual ~Touch() = default;

    virtual std::pair<int16_t, int16_t> read() = 0;
};