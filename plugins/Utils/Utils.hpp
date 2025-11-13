#pragma once
#include "SC_PlugIn.hpp"
#include <array>
#include <cmath>  
#include <algorithm>

namespace Utils {

// ===== CONSTANTS =====

inline constexpr float SAFE_DENOM_EPSILON = 1e-10f;
inline constexpr float PI = 3.14159265358979323846f;
inline constexpr float TWO_PI = 6.28318530717958647692f;
inline constexpr float TWO_PI_INV = 1.0f / 6.28318530717958647692f;

// ===== BASIC MATH UTILITIES =====

inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

inline float cosineInterp(float a, float b, float t) {
    float mu2 = (1.0f - std::cos(t * PI)) / 2.0f;
    return a * (1.0f - mu2) + b * mu2;
}

// ===== HIGH-PERFORMANCE BUFFER ACCESS UTILITIES =====

// Fast no-interpolation peek with bitwise wrapping and optional offset - (for power-of-2 sizes)
inline float peekNoInterp(const float* buffer, int index, int startPos, int mask) {
    const int wrappedIndex = startPos + (index & mask);
    return buffer[wrappedIndex];
}

// Fast linear interpolation peek with bitwise wrapping - (for power-of-2 sizes)
inline float peekLinearInterp(const float* buffer, float phase, int mask) {

    const int intPart = static_cast<int>(phase);
    const float fracPart = phase - static_cast<float>(intPart);
    
    const int idx1 = intPart & mask;
    const int idx2 = (intPart + 1) & mask;
    
    const float a = buffer[idx1];
    const float b = buffer[idx2];
    
    return lerp(a, b, fracPart);
}

// Fast cubic interpolation peek with bitwise wrapping - (for power-of-2 sizes)
inline float peekCubicInterp(const float* buffer, float phase, int mask) {
    
    const int intPart = static_cast<int>(phase);
    const float fracPart = phase - static_cast<float>(intPart);
    
    const int idx0 = (intPart - 1) & mask;
    const int idx1 = intPart & mask;
    const int idx2 = (intPart + 1) & mask;
    const int idx3 = (intPart + 2) & mask;
    
    const float a = buffer[idx0];
    const float b = buffer[idx1];
    const float c = buffer[idx2];
    const float d = buffer[idx3];
    
    return cubicinterp(fracPart, a, b, c, d);
}

// ===== BIT MANIPULATION UTILITIES =====

inline int rotateBits(int value, int rotation, int length) {
    // Use wrap instead of % to handle negative rotation amount
    int normalizedRotation = sc_wrap(rotation, 0, length - 1);
    int complementRotation = length - normalizedRotation;
        
    // Calculate bit ranges for the given length
    int maxValueForLength = static_cast<int>(std::pow(2, length));
    double leftShiftMultiplier = std::pow(2, normalizedRotation);
    double rightShiftDivisor = std::pow(2, -complementRotation);
        
    // Perform the bit shifts
    double leftShifted = value * leftShiftMultiplier;
    double rightShifted = value * rightShiftDivisor;
        
    // Extract the relevant parts
    int leftPart = static_cast<int>(leftShifted) % maxValueForLength;
    int rightPart = static_cast<int>(std::floor(rightShifted));
        
    // Combine both parts to get the rotated result
    return leftPart + rightPart;
}

// Extract top numBits and apply LSB weighting (bit5*1 + bit6*2 + bit7*4)
inline float getMSBBits(int value, int numBits, int totalBits) {
    int startBit = totalBits - numBits;  // Calculate start bit for MSB
    int result = 0;
        
    for (int i = 0; i < numBits; i++) {
        int bitIndex = startBit + i;
            
        // Extract the bit using power/modulus
        int divisor = static_cast<int>(std::pow(2, bitIndex));
        int bit = (value / divisor) % 2;
            
        // Apply LSB-first weighting
        int weight = static_cast<int>(std::pow(2, i));
        result += bit * weight;
    }
        
    // Normalize to 0-1 range
    int maxValue = static_cast<int>(std::pow(2, numBits)) - 1;
    return static_cast<float>(result) / static_cast<float>(maxValue);
}

// Extract bottom numBits and apply MSB weighting (bit0*128 + bit1*64 + ... + bit7*1)
inline float getLSBBits(int value, int numBits, int totalBits) {
    int result = 0;
        
    for (int i = 0; i < numBits; i++) {
        int bitIndex = i;  // Start from bit 0
            
        // Extract the bit using power/modulus
        int divisor = static_cast<int>(std::pow(2, bitIndex));
        int bit = (value / divisor) % 2;
            
        // Apply MSB-first weighting
        int weight = static_cast<int>(std::pow(2, numBits - 1 - i));
        result += bit * weight;
    }
        
    // Normalize to 0-1 range
    int maxValue = static_cast<int>(std::pow(2, numBits)) - 1;
    return static_cast<float>(result) / static_cast<float>(maxValue);
}

} // namespace Utils