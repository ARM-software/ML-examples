//
// Copyright © 2019 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include "inference_test_image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../third-party/stb/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../third-party/stb/stb_image_resize.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third-party/stb/stb_image_write.h"

#include <armnn/Exceptions.hpp>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/format.hpp>

#include <memory>
#include <algorithm>

inline float Lerp(float a, float b, float w)
{
    return w * b + (1.f - w) * a;
}

inline void PutData(std::vector<float>& data,
                    unsigned int width,
                    unsigned int x,
                    unsigned int y,
                    unsigned int c,
                    float value)
{
    data[(3 * ((y * width) + x)) + c] = value;
}

// Resize and normalize the given image
std::vector<float> ResizeBilinearAndNormalize(const InferenceTestImage& image,
                                              unsigned int outputWidth,
                                              unsigned int outputHeight,
                                              float scale,
                                              const std::array<float, 3>& mean,
                                              const std::array<float, 3>& stddev)
{
    std::vector<float> out(outputWidth * outputHeight * 3, 0.f);

    // We follow the definition of TensorFlow and AndroidNN: the top-left corner of a texel in the output
    // image is projected into the input image to figure out the interpolants and weights. Note that this
    // will yield different results than if projecting the centre of output texels.

    const unsigned int inputWidth = image.GetWidth();
    const unsigned int inputHeight = image.GetHeight();

    // How much to scale pixel coordinates in the output image to get the corresponding pixel coordinates
    // in the input image.
    const float scaleY = boost::numeric_cast<float>(inputHeight) / boost::numeric_cast<float>(outputHeight);
    const float scaleX = boost::numeric_cast<float>(inputWidth) / boost::numeric_cast<float>(outputWidth);

    uint8_t rgb_x0y0[3] = {};
    uint8_t rgb_x1y0[3] = {};
    uint8_t rgb_x0y1[3] = {};
    uint8_t rgb_x1y1[3] = {};

    for (unsigned int y = 0; y < outputHeight; ++y)
    {
        // Corresponding real-valued height coordinate in input image.
        const float iy = boost::numeric_cast<float>(y) * scaleY;

        // Discrete height coordinate of top-left texel (in the 2x2 texel area used for interpolation).
        const float fiy = floorf(iy);
        const unsigned int y0 = boost::numeric_cast<unsigned int>(fiy);

        // Interpolation weight (range [0,1])
        const float yw = iy - fiy;

        for (unsigned int x = 0; x < outputWidth; ++x)
        {
            // Real-valued and discrete width coordinates in input image.
            const float ix = boost::numeric_cast<float>(x) * scaleX;
            const float fix = floorf(ix);
            const unsigned int x0 = boost::numeric_cast<unsigned int>(fix);

            // Interpolation weight (range [0,1]).
            const float xw = ix - fix;

            // Discrete width/height coordinates of texels below and to the right of (x0, y0).
            const unsigned int x1 = std::min(x0 + 1, inputWidth - 1u);
            const unsigned int y1 = std::min(y0 + 1, inputHeight - 1u);

            std::tie(rgb_x0y0[0], rgb_x0y0[1], rgb_x0y0[2]) = image.GetPixelAs3Channels(x0, y0);
            std::tie(rgb_x1y0[0], rgb_x1y0[1], rgb_x1y0[2]) = image.GetPixelAs3Channels(x1, y0);
            std::tie(rgb_x0y1[0], rgb_x0y1[1], rgb_x0y1[2]) = image.GetPixelAs3Channels(x0, y1);
            std::tie(rgb_x1y1[0], rgb_x1y1[1], rgb_x1y1[2]) = image.GetPixelAs3Channels(x1, y1);

            for (unsigned c = 0; c < 3; ++c)
            {
                const float ly0 = Lerp(float(rgb_x0y0[c]), float(rgb_x1y0[c]), xw);
                const float ly1 = Lerp(float(rgb_x0y1[c]), float(rgb_x1y1[c]), xw);
                const float l   = Lerp(ly0, ly1, yw);
                PutData(out, outputWidth, x, y, c, ((l / scale) - mean[c]) / stddev[c]);
            }
        }
    }
    return out;
}

InferenceTestImage::InferenceTestImage(char const* filePath)
    : m_Width(0u)
    , m_Height(0u)
    , m_NumChannels(0u)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    using StbImageDataPtr = std::unique_ptr<unsigned char, decltype(&stbi_image_free)>;
    StbImageDataPtr stbData(stbi_load(filePath, &width, &height, &channels, 0), &stbi_image_free);

    if (stbData == nullptr)
    {
        throw armnn::Exception(boost::str(boost::format("Could not load the image at %1%") % filePath));
    }

    if (width == 0 || height == 0)
    {
        throw armnn::Exception(boost::str(boost::format("Could not load empty image at %1%") % filePath));
    }

    m_Width = boost::numeric_cast<unsigned int>(width);
    m_Height = boost::numeric_cast<unsigned int>(height);
    m_NumChannels = boost::numeric_cast<unsigned int>(channels);

    const unsigned int sizeInBytes = GetSizeInBytes();
    m_Data.resize(sizeInBytes);
    memcpy(m_Data.data(), stbData.get(), sizeInBytes);
}

std::tuple<uint8_t, uint8_t, uint8_t> InferenceTestImage::GetPixelAs3Channels(unsigned int x, unsigned int y) const
{
    if (x >= m_Width || y >= m_Height)
    {
        throw armnn::InvalidArgumentException(
                    boost::str(boost::format("Attempted out of bounds image access. "
                                             "Requested (%1%, %2%). Maximum valid coordinates (%3%, %4%).")
                               % x
                               % y
                               % (m_Width - 1)
                               % (m_Height - 1)));
    }

    const unsigned int pixelOffset = x * GetNumChannels() + y * GetWidth() * GetNumChannels();
    const uint8_t* const pixelData = m_Data.data() + pixelOffset;
    BOOST_ASSERT(pixelData <= (m_Data.data() + GetSizeInBytes()));

    std::array<uint8_t, 3> outPixelData;
    outPixelData.fill(0);

    const unsigned int maxChannelsInPixel = std::min(GetNumChannels(), static_cast<unsigned int>(outPixelData.size()));
    for (unsigned int c = 0; c < maxChannelsInPixel; ++c)
    {
        outPixelData[c] = pixelData[c];
    }

    return std::make_tuple(outPixelData[0], outPixelData[1], outPixelData[2]);
}

std::vector<float> InferenceTestImage::Resize(unsigned int newWidth,
                                              unsigned int newHeight,
                                              const std::array<float, 3>& mean,
                                              const std::array<float, 3>& stddev,
                                              float scale)
{
    if (newWidth == 0 || newHeight == 0)
    {
        throw armnn::InvalidArgumentException(
                    boost::str(boost::format("None of the dimensions passed to a resize operation can be zero. "
                                             "Requested width: %1%. Requested height: %2%.")
                               % newWidth
                               % newHeight));
    }

    return ResizeBilinearAndNormalize(*this, newWidth, newHeight, scale, mean, stddev);
}
