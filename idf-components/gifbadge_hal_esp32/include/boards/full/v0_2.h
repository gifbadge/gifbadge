/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include "v0_2_v0_4_common.h"

namespace Boards::esp32::s3::full {

class v0_2 : public Boards::esp32::s3::full::v0_2v0_4 {
  public:
    void LateInit() override;

    v0_2() = default;
  ~v0_2() override = default;

  const char * Name() override;
};
}