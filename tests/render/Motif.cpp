#include <render/decorations/Motif.hpp>

#include <gtest/gtest.h>

TEST(Motif, raisedLightMatchesOpenMotifMiters) {
    EXPECT_TRUE(Motif::raisedLight(0, 0, 10, 10, 2));
    EXPECT_TRUE(Motif::raisedLight(9, 0, 10, 10, 2)); // outer top-right corner
    EXPECT_FALSE(Motif::raisedLight(9, 1, 10, 10, 2));
    EXPECT_TRUE(Motif::raisedLight(0, 9, 10, 10, 2)); // outer bottom-left corner
    EXPECT_FALSE(Motif::raisedLight(1, 9, 10, 10, 2));
    EXPECT_FALSE(Motif::raisedLight(5, 5, 10, 10, 2));
}

TEST(Motif, shadeMatchesShaderRecipe) {
    const CHyprColor FACE = CHyprColor(0.5F, 0.5F, 0.5F, 0.5F);

    const auto       DARK  = Motif::shade(FACE, false);
    const auto       LIGHT = Motif::shade(FACE, true);

    EXPECT_FLOAT_EQ(DARK.r, 64.0F / 255.0F);
    EXPECT_FLOAT_EQ(DARK.g, 64.0F / 255.0F);
    EXPECT_FLOAT_EQ(DARK.b, 64.0F / 255.0F);
    EXPECT_FLOAT_EQ(DARK.a, 0.5F);

    EXPECT_FLOAT_EQ(LIGHT.r, 191.0F / 255.0F);
    EXPECT_FLOAT_EQ(LIGHT.g, 191.0F / 255.0F);
    EXPECT_FLOAT_EQ(LIGHT.b, 191.0F / 255.0F);
    EXPECT_FLOAT_EQ(LIGHT.a, 0.5F);
}

TEST(Motif, shadeExtremes) {
    const auto WHITE = CHyprColor(1.F, 1.F, 1.F, 1.F);
    const auto BLACK = CHyprColor(0.F, 0.F, 0.F, 1.F);

    EXPECT_FLOAT_EQ(Motif::shade(WHITE, false).r, 127.0F / 255.0F);
    EXPECT_FLOAT_EQ(Motif::shade(WHITE, true).r, 1.F);
    EXPECT_FLOAT_EQ(Motif::shade(BLACK, false).r, 0.F);
    EXPECT_FLOAT_EQ(Motif::shade(BLACK, true).r, 127.0F / 255.0F);
}

TEST(Motif, rampMatchesShaderRecipe) {
    const CHyprColor FACE = CHyprColor(0.5F, 0.5F, 0.5F, 0.5F);

    EXPECT_FLOAT_EQ(Motif::ramp(FACE, 0).r, 197.0F / 255.0F);
    EXPECT_FLOAT_EQ(Motif::ramp(FACE, 1).r, 163.0F / 255.0F);
    EXPECT_FLOAT_EQ(Motif::ramp(FACE, 2).r, 128.0F / 255.0F);
    EXPECT_FLOAT_EQ(Motif::ramp(FACE, 3).r, 128.0F / 255.0F);
    EXPECT_FLOAT_EQ(Motif::ramp(FACE, 4).r, 102.0F / 255.0F);
    EXPECT_FLOAT_EQ(Motif::ramp(FACE, 5).r, 76.0F / 255.0F);

    EXPECT_FLOAT_EQ(Motif::ramp(FACE, 0).a, 0.5F);
    EXPECT_FLOAT_EQ(Motif::ramp(FACE, 5).a, 0.5F);
}
