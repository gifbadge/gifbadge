/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "soc/rtc.h"

#ifdef BOOTLOADER_BUILD

int esp_clk_apb_freq(void)
{
    return rtc_clk_apb_freq_get();
}

#endif // BOOTLOADER_BUILD
