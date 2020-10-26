//
// Copyright © 2019 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#pragma once

#include <array>
#include <vector>
#include <cstdint>

// Parameters used in normalizing images
struct NormalizationParameters
{
    float scale{ 1.0 };
    std::array<float, 3> mean{ { 0.0, 0.0, 0.0 } };
    std::array<float, 3> stddev{ { 1.0, 1.0, 1.0 } };
};

// Image wrapper used for inferences
class InferenceTestImage
{
public:
    // Common names used to identify a channel in a pixel.
    enum class ResizingMethods
    {
        STB,
        BilinearAndNormalized,
    };

    explicit InferenceTestImage(char const* filePath);

    InferenceTestImage(InferenceTestImage&&) = delete;
    InferenceTestImage(const InferenceTestImage&) = delete;
    InferenceTestImage& operator=(const InferenceTestImage&) = delete;
    InferenceTestImage& operator=(InferenceTestImage&&) = delete;

    unsigned int GetWidth()       const { return m_Width; }
    unsigned int GetHeight()      const { return m_Height; }
    unsigned int GetNumChannels() const { return m_NumChannels; }
    unsigned int GetNumElements() const { return GetWidth() * GetHeight() * GetNumChannels(); }
    unsigned int GetSizeInBytes() const { return GetNumElements() * GetSingleElementSizeInBytes(); }

    // Returns the pixel identified by the given coordinates as a 3-channel value.
    // Channels beyond the third are dropped. If the image provides less than 3 channels, the non-existent
    // channels of the pixel will be filled with 0. Channels are returned in RGB order (that is, the first element
    // of the tuple corresponds to the Red channel, whereas the last element is the Blue channel).
    std::tuple<uint8_t, uint8_t, uint8_t> GetPixelAs3Channels(unsigned int x, unsigned int y) const;

    std::vector<float> Resize(unsigned int newWidth,
                              unsigned int newHeight,
                              const std::array<float, 3>& mean = {{0.0, 0.0, 0.0}},
                              const std::array<float, 3>& stddev = {{1.0, 1.0, 1.0}},
                              float scale = 255.0f);

private:
    static unsigned int GetSingleElementSizeInBytes()
    {
        return sizeof(decltype(std::declval<InferenceTestImage>().m_Data[0]));
    }

    std::vector<uint8_t> m_Data;
    unsigned int m_Width;
    unsigned int m_Height;
    unsigned int m_NumChannels;
};
