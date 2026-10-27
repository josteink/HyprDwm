#include "Motif.hpp"

#include <algorithm>
#include <cmath>

static double toByte(double value) {
    return std::floor(std::clamp(value, 0.0, 1.0) * 255.0 + 0.5);
}

static double shadeByte(double byte, bool lightSide) {
    return lightSide ? byte + std::floor((255.0 - byte) * 0.5) : std::floor(byte * 0.5);
}

static double rampByte(double byte, int step) {
    if (step <= 0)
        return byte + std::floor((255.0 - byte) * 0.55);
    if (step == 1)
        return byte + std::floor((255.0 - byte) * 0.28);
    if (step == 4)
        return std::floor(byte * 0.80);
    if (step == 5)
        return std::floor(byte * 0.60);
    return byte;
}

bool Motif::raisedLight(float px, float py, float w, float h, float b) {
    return (py < b && px + py < w) || (px < b && px + py < h);
}

CHyprColor Motif::shade(const CHyprColor& face, bool lightSide) {
    const auto R = shadeByte(toByte(face.r), lightSide);
    const auto G = shadeByte(toByte(face.g), lightSide);
    const auto B = shadeByte(toByte(face.b), lightSide);

    return CHyprColor(sc<float>(R / 255.0), sc<float>(G / 255.0), sc<float>(B / 255.0), sc<float>(face.a));
}

CHyprColor Motif::ramp(const CHyprColor& face, int step) {
    const auto R = rampByte(toByte(face.r), step);
    const auto G = rampByte(toByte(face.g), step);
    const auto B = rampByte(toByte(face.b), step);

    return CHyprColor(sc<float>(R / 255.0), sc<float>(G / 255.0), sc<float>(B / 255.0), sc<float>(face.a));
}
