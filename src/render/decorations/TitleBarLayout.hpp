#pragma once

#include "../../helpers/math/Math.hpp"

#include <optional>

namespace TitleBar {
    constexpr int SEPARATOR       = 1;
    constexpr int TEXT_PADDING    = 2;
    constexpr int MIN_BUTTON_SIZE = 10;

    struct SLayoutConfig {
        int  separator     = SEPARATOR;
        int  padding       = TEXT_PADDING;
        int  minButtonSize = MIN_BUTTON_SIZE;
        bool closeButton   = true;
    };

    struct SLayout {
        CBox                titleBar;
        CBox                textArea;
        CBox                separator;
        std::optional<CBox> closeButton;
    };

    SLayout layout(const CBox& bar, const SLayoutConfig& config);
}
