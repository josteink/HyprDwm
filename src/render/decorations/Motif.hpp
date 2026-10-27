#pragma once

#include "../../helpers/Color.hpp"

namespace Motif {
    // OpenMotif XmeDrawShadows, raised: top and left are light, including the outer
    // top-right and bottom-left corners. The other two corners are 45-degree miters.
    bool raisedLight(float px, float py, float w, float h, float b);

    // Same 8-bit recipe as the CDE panel CSS: light is halfway to white, dark is half the face.
    CHyprColor shade(const CHyprColor& face, bool lightSide);
}
