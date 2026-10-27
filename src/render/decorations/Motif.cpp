#include "Motif.hpp"

#include <algorithm>
#include <cmath>

static double toByte(double value) {
    return std::floor(std::clamp(value, 0.0, 1.0) * 255.0 + 0.5);
}

static double shadeByte(double byte, bool lightSide) {
    return lightSide ? byte + std::floor((255.0 - byte) * 0.5) : std::floor(byte * 0.5);
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
