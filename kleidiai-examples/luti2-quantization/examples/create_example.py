# SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

"""Create the example tensor used by the lut2quant quick start."""

import numpy as np


weights = np.random.default_rng(7).normal(size=(64, 256))
np.save("layer.npy", weights)
print("wrote layer.npy")
