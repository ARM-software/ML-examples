//
// Copyright © 2019 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include "utils.hpp"

std::vector<uint8_t> PrepareImageTensor(const std::string& imagePath,
                                        unsigned int newWidth,
                                        unsigned int newHeight,
                                        const NormalizationParameters& normParams)
{
    // Get float32 image tensor
    InferenceTestImage testImage(imagePath.c_str());
    if (newWidth == 0)
    {
        newWidth = testImage.GetWidth();
    }
    if (newHeight == 0)
    {
        newHeight = testImage.GetHeight();
    }

    // Resize the image to new width and height or keep at original dimensions if the new width and height are specified
    // as 0 Centre/Normalise the image.
    std::vector<float> imageDataFloat = testImage.Resize(newWidth,
                                                         newHeight,
                                                         normParams.mean,
                                                         normParams.stddev,
                                                         normParams.scale);

    // Convert to uint8 image tensor with static cast
    std::vector<uint8_t> imageDataQasymm8;
    imageDataQasymm8.reserve(imageDataFloat.size());
    std::transform(imageDataFloat.begin(), imageDataFloat.end(), std::back_inserter(imageDataQasymm8),
                   [](float val) { return static_cast<uint8_t>(val); });

    return imageDataQasymm8;
}
