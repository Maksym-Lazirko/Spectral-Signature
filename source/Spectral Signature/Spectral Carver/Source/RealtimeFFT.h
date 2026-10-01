#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

// An instance belongs exclusively to one audio engine. Tables are allocated only
// at construction; transforms use the caller's 2*N float buffer and never lock.
class RealtimeFFT
{
public:
    explicit RealtimeFFT (int order)
        : size (1 << order), reversed ((size_t) size), cosine ((size_t) size / 2), sine ((size_t) size / 2)
    {
        for (int index = 0; index < size; ++index)
        {
            uint32_t value = (uint32_t) index, reverse = 0;
            for (int bit = 0; bit < order; ++bit)
            {
                reverse = (reverse << 1) | (value & 1u);
                value >>= 1;
            }
            reversed[(size_t) index] = reverse;
        }
        for (int index = 0; index < size / 2; ++index)
        {
            const auto angle = -6.2831853071795864769 * (double) index / (double) size;
            cosine[(size_t) index] = (float) std::cos (angle);
            sine[(size_t) index] = (float) std::sin (angle);
        }
    }

    void performRealOnlyForwardTransform (float* data) const noexcept
    {
        for (int index = size - 1; index >= 0; --index)
        {
            data[2 * index] = data[index];
            data[2 * index + 1] = 0.0f;
        }
        transform (data, false);
    }

    void performRealOnlyInverseTransform (float* data) const noexcept
    {
        data[1] = data[size + 1] = 0.0f;
        for (int index = size / 2 + 1; index < size; ++index)
        {
            data[2 * index] = data[2 * (size - index)];
            data[2 * index + 1] = -data[2 * (size - index) + 1];
        }
        transform (data, true);
        const auto scale = 1.0f / (float) size;
        for (int index = 0; index < size; ++index)
            data[index] = data[2 * index] * scale;
    }

private:
    void transform (float* data, bool inverse) const noexcept
    {
        for (int index = 0; index < size; ++index)
        {
            const auto other = (int) reversed[(size_t) index];
            if (other > index)
            {
                std::swap (data[2 * index], data[2 * other]);
                std::swap (data[2 * index + 1], data[2 * other + 1]);
            }
        }
        for (int length = 2; length <= size; length *= 2)
        {
            const auto half = length / 2, step = size / length;
            for (int base = 0; base < size; base += length)
                for (int offset = 0; offset < half; ++offset)
                {
                    const auto twiddle = (size_t) (offset * step);
                    const auto wr = cosine[twiddle], wi = inverse ? -sine[twiddle] : sine[twiddle];
                    const auto even = 2 * (base + offset), odd = 2 * (base + offset + half);
                    const auto real = wr * data[odd] - wi * data[odd + 1];
                    const auto imaginary = wr * data[odd + 1] + wi * data[odd];
                    data[odd] = data[even] - real;
                    data[odd + 1] = data[even + 1] - imaginary;
                    data[even] += real;
                    data[even + 1] += imaginary;
                }
        }
    }

    int size;
    std::vector<uint32_t> reversed;
    std::vector<float> cosine, sine;
};
