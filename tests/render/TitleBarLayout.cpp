#include <render/decorations/TitleBarLayout.hpp>

#include <gtest/gtest.h>

TEST(TitleBarLayout, closeButtonIsSeparateAndRightAligned) {
    const auto LAYOUT = TitleBar::layout(CBox{0, 0, 200, 20}, TitleBar::SLayoutConfig{});

    ASSERT_TRUE(LAYOUT.closeButton.has_value());
    EXPECT_EQ(LAYOUT.closeButton->x, 180);
    EXPECT_EQ(LAYOUT.closeButton->y, 0);
    EXPECT_EQ(LAYOUT.closeButton->w, 20);
    EXPECT_EQ(LAYOUT.closeButton->h, 20);

    EXPECT_EQ(LAYOUT.titleBar.x, 0);
    EXPECT_EQ(LAYOUT.titleBar.y, 0);
    EXPECT_EQ(LAYOUT.titleBar.w, 180);
    EXPECT_EQ(LAYOUT.titleBar.h, 20);

    EXPECT_EQ(LAYOUT.textArea.x, 2);
    EXPECT_EQ(LAYOUT.textArea.y, 0);
    EXPECT_EQ(LAYOUT.textArea.w, 176);
    EXPECT_EQ(LAYOUT.textArea.h, 18);
}

TEST(TitleBarLayout, closeButtonRespectsBarOffset) {
    const auto LAYOUT = TitleBar::layout(CBox{10, 20, 100, 20}, TitleBar::SLayoutConfig{});

    ASSERT_TRUE(LAYOUT.closeButton.has_value());
    EXPECT_EQ(LAYOUT.closeButton->x, 90);
    EXPECT_EQ(LAYOUT.closeButton->y, 20);
    EXPECT_EQ(LAYOUT.titleBar.w, 80);
    EXPECT_EQ(LAYOUT.textArea.x, 12);
    EXPECT_EQ(LAYOUT.textArea.w, 76);
}

TEST(TitleBarLayout, narrowBarDropsButton) {
    const auto LAYOUT = TitleBar::layout(CBox{0, 0, 20, 20}, TitleBar::SLayoutConfig{});

    EXPECT_FALSE(LAYOUT.closeButton.has_value());
    EXPECT_EQ(LAYOUT.titleBar.w, 20);
    EXPECT_EQ(LAYOUT.textArea.x, 2);
    EXPECT_EQ(LAYOUT.textArea.w, 16);
}

TEST(TitleBarLayout, shortBarDropsButton) {
    const auto LAYOUT = TitleBar::layout(CBox{0, 0, 200, 8}, TitleBar::SLayoutConfig{});

    EXPECT_FALSE(LAYOUT.closeButton.has_value());
    EXPECT_EQ(LAYOUT.titleBar.w, 200);
    EXPECT_EQ(LAYOUT.textArea.w, 196);
    EXPECT_EQ(LAYOUT.textArea.h, 6);
}

TEST(TitleBarLayout, minimumButtonSizeShowsButton) {
    const auto LAYOUT = TitleBar::layout(CBox{0, 0, 200, 10}, TitleBar::SLayoutConfig{});

    ASSERT_TRUE(LAYOUT.closeButton.has_value());
    EXPECT_EQ(LAYOUT.closeButton->w, 10);
    EXPECT_EQ(LAYOUT.closeButton->h, 10);
    EXPECT_EQ(LAYOUT.titleBar.w, 190);
}

TEST(TitleBarLayout, disabledButtonLeavesFullTextArea) {
    TitleBar::SLayoutConfig config;
    config.closeButton = false;

    const auto LAYOUT = TitleBar::layout(CBox{0, 0, 200, 20}, config);

    EXPECT_FALSE(LAYOUT.closeButton.has_value());
    EXPECT_EQ(LAYOUT.titleBar.w, 200);
    EXPECT_EQ(LAYOUT.textArea.w, 196);
}

TEST(TitleBarLayout, tinyBarDoesNotProduceNegativeTextArea) {
    const auto LAYOUT = TitleBar::layout(CBox{0, 0, 1, 1}, TitleBar::SLayoutConfig{});

    EXPECT_FALSE(LAYOUT.closeButton.has_value());
    EXPECT_GE(LAYOUT.textArea.w, 0);
    EXPECT_GE(LAYOUT.textArea.h, 0);
}
