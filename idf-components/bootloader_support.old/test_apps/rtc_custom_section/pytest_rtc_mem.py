#  Copyright (c) 2026 GifBadge
#
#  SPDX-License-Identifier:   GPL-3.0-or-later
#
#  SPDX-License-Identifier:   GPL-3.0-or-later
#
#  SPDX-License-Identifier:   GPL-3.0-or-later
import pytest
from pytest_embedded import Dut
from pytest_embedded_idf.utils import idf_parametrize


@pytest.mark.generic
@idf_parametrize(
    'target',
    ['esp32', 'esp32c3', 'esp32c5', 'esp32c6', 'esp32h2', 'esp32s2', 'esp32s3', 'esp32p4'],
    indirect=['target'],
)
def test_rtc_reserved_memory(dut: Dut) -> None:
    dut.expect_exact('SUCCESS: data were saved across reboot', timeout=10)
