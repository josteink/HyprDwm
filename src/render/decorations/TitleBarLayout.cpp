#include "TitleBarLayout.hpp"

#include <algorithm>

TitleBar::SLayout TitleBar::layout(const CBox& bar, const SLayoutConfig& config) {
    SLayout layout;
    if (bar.w <= 0 || bar.h <= 0)
        return layout;

    const int  SEP      = std::clamp(config.separator, 0, std::max(0, static_cast<int>(bar.h) - 1));
    const int  PADDING  = std::max(0, config.padding);
    const int  BUTTONSZ = static_cast<int>(bar.h);
    const bool SHOWBTN  = config.closeButton && BUTTONSZ >= config.minButtonSize && bar.w > BUTTONSZ + 2 * PADDING;

    layout.titleBar = {bar.x, bar.y, bar.w - (SHOWBTN ? BUTTONSZ : 0.0), bar.h};

    if (SHOWBTN)
        layout.closeButton = CBox{bar.x + bar.w - BUTTONSZ, bar.y, static_cast<double>(BUTTONSZ), static_cast<double>(BUTTONSZ)};

    layout.textArea = {layout.titleBar.x + PADDING, bar.y, std::max(0.0, layout.titleBar.w - 2 * PADDING), static_cast<double>(std::max(0, static_cast<int>(bar.h) - SEP))};

    return layout;
}
