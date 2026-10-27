#include "CHyprTitleBarDecoration.hpp"

#include "Motif.hpp"
#include "TitleBarLayout.hpp"

#include "../../Compositor.hpp"
#include "../../config/ConfigValue.hpp"
#include "../../desktop/state/FocusState.hpp"
#include "../../desktop/state/WindowState.hpp"
#include "../../desktop/view/window/Window.hpp"
#include "../../desktop/view/window/WindowPresentation.hpp"
#include "../../helpers/time/Time.hpp"
#include "../../managers/fullscreen/FullscreenController.hpp"
#include "../../managers/input/InputManager.hpp"
#include "../Renderer.hpp"
#include "../WindowRenderPresentation.hpp"
#include "../pass/BorderPassElement.hpp"
#include "../pass/RectPassElement.hpp"
#include "../pass/TexPassElement.hpp"

#include <algorithm>
#include <cairo/cairo.h>
#include <cmath>

using namespace Render;

static double luminance(const CHyprColor& color) {
    return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

static int scaledThickness(int value, float scale) {
    return std::max(1, sc<int>(std::round(value * scale)));
}

static TitleBar::SLayoutConfig logicalLayoutConfig(bool closeButton) {
    TitleBar::SLayoutConfig config;
    config.separator     = TitleBar::SEPARATOR;
    config.padding       = TitleBar::TEXT_PADDING;
    config.minButtonSize = TitleBar::MIN_BUTTON_SIZE;
    config.closeButton   = closeButton;
    return config;
}

CHyprTitleBarDecoration::CHyprTitleBarDecoration(PHLWINDOW pWindow) : IHyprWindowDecoration(pWindow), m_window(pWindow) {
    ;
}

int CHyprTitleBarDecoration::height() const {
    static auto PHEIGHT = CConfigValue<Config::INTEGER>("decoration:titlebar:height");

    return std::max(0, sc<int>(*PHEIGHT));
}

int CHyprTitleBarDecoration::currentHeight() const {
    return visible() ? height() : 0;
}

bool CHyprTitleBarDecoration::visible() const {
    static auto PENABLED = CConfigValue<Config::BOOL>("decoration:titlebar:enabled");

    const auto  PWINDOW = m_window.lock();
    if (!PWINDOW || !*PENABLED)
        return false;

    if (PWINDOW->backend().traits().suggestsNoBorder)
        return false;

    if (!PWINDOW->m_ruleApplicator || !PWINDOW->m_ruleApplicator->decorate().valueOrDefault())
        return false;

    if (Fullscreen::controller()->getFullscreenModes(PWINDOW).internal == Fullscreen::FSMODE_FULLSCREEN)
        return false;

    return height() > 0;
}

SDecorationPositioningInfo CHyprTitleBarDecoration::getPositioningInfo() {
    SDecorationPositioningInfo info;
    info.policy   = DECORATION_POSITION_STICKY;
    info.edges    = DECORATION_EDGE_TOP;
    info.priority = 10001; // above the border (10000), so the border frame wraps the title bar
    info.reserved = true;

    if (!visible()) {
        info.desiredExtents = {{}, {}};
        return info;
    }

    info.desiredExtents = {Vector2D{0.0, sc<double>(height())}, Vector2D{0.0, 0.0}};
    return info;
}

void CHyprTitleBarDecoration::onPositioningReply(const SDecorationPositioningReply& reply) {
    m_assignedGeometry = reply.assignedGeometry;
}

CHyprColor CHyprTitleBarDecoration::faceColor(bool active) const {
    static auto PACTIVE   = CConfigValue<Config::INTEGER>("decoration:titlebar:col.active");
    static auto PINACTIVE = CConfigValue<Config::INTEGER>("decoration:titlebar:col.inactive");

    const auto  PWINDOW = m_window.lock();
    if (!PWINDOW)
        return CHyprColor(0xffffffff);

    const auto BORDERCOLOR = PWINDOW->presentation().borderColor();

    if (active)
        return *PACTIVE == -1 ? BORDERCOLOR : CHyprColor(sc<uint64_t>(*PACTIVE));

    return *PINACTIVE == -1 ? BORDERCOLOR : CHyprColor(sc<uint64_t>(*PINACTIVE));
}

CHyprColor CHyprTitleBarDecoration::textColor(bool active) const {
    static auto      PACTIVE   = CConfigValue<Config::INTEGER>("decoration:titlebar:text_color");
    static auto      PINACTIVE = CConfigValue<Config::INTEGER>("decoration:titlebar:text_color_inactive");

    const CHyprColor ACTIVE = CHyprColor(*PACTIVE == -1 ? 0xffffffff : sc<uint64_t>(*PACTIVE));

    if (active)
        return ACTIVE;

    return *PINACTIVE == -1 ? ACTIVE : CHyprColor(sc<uint64_t>(*PINACTIVE));
}

CHyprColor CHyprTitleBarDecoration::shadowColor(const CHyprColor& face, const CHyprColor& text) const {
    static auto PENABLED = CConfigValue<Config::BOOL>("decoration:titlebar:text_shadow");
    static auto PCOLOR   = CConfigValue<Config::INTEGER>("decoration:titlebar:text_shadow_color");

    if (!*PENABLED)
        return CHyprColor(0, 0, 0, 0);

    if (*PCOLOR != -1)
        return CHyprColor(sc<uint64_t>(*PCOLOR)).modifyA(sc<float>(text.a));

    // Light text gets a dark shadow and vice versa.
    return Motif::shade(face, luminance(text) < 0.5).modifyA(sc<float>(text.a));
}

CBox CHyprTitleBarDecoration::assignedBoxGlobal(const Render::SWindowRenderPresentation& presentation) const {
    CBox box = m_assignedGeometry;
    box.translate(g_pDecorationPositioner->getEdgeDefinedPoint(DECORATION_EDGE_TOP, m_window));
    box.translate(presentation.workspaceOffset);

    return box.round();
}

CBox CHyprTitleBarDecoration::inputBox() const {
    const auto PWINDOW = m_window.lock();
    if (!PWINDOW)
        return {};

    const auto PRESENTATION = PWINDOW->presentation().renderPresentation();
    CBox       box          = assignedBoxGlobal(PRESENTATION);
    box.translate(PRESENTATION.floatingOffset);

    return box.round();
}

void CHyprTitleBarDecoration::invalidateTextures() {
    m_titleTextures = {};
}

void CHyprTitleBarDecoration::invalidateCloseGlyph() {
    m_closeGlyph = {};
}

void CHyprTitleBarDecoration::ensureTitleTextures(const std::string& title, bool active, float scale, int maxWidth, int fontSize, const std::string& fontFamily,
                                                  const CHyprColor& text, const CHyprColor& shadow) {
    auto& CACHE = m_titleTextures;

    if (CACHE.main && CACHE.main->ok() && CACHE.content == title && CACHE.fontFamily == fontFamily && CACHE.active == active && CACHE.scale == scale &&
        CACHE.maxWidth == maxWidth && CACHE.fontSize == fontSize && CACHE.textColor == text && CACHE.shadowColor == shadow)
        return;

    CACHE             = {};
    CACHE.content     = title;
    CACHE.fontFamily  = fontFamily;
    CACHE.active      = active;
    CACHE.scale       = scale;
    CACHE.maxWidth    = maxWidth;
    CACHE.fontSize    = fontSize;
    CACHE.textColor   = text;
    CACHE.shadowColor = shadow;

    CACHE.main = g_pHyprRenderer->renderText(title, text, fontSize, false, fontFamily, maxWidth);

    if (shadow.a > 0.F)
        CACHE.shadowTex = g_pHyprRenderer->renderText(title, shadow, fontSize, false, fontFamily, maxWidth);
}

SP<ITexture> CHyprTitleBarDecoration::ensureCloseGlyph(int size, float scale, const CHyprColor& glyph, const CHyprColor& glyphShadow) {
    auto& CACHE = m_closeGlyph;

    if (size < 1)
        return nullptr;

    if (CACHE.tex && CACHE.tex->ok() && CACHE.size == size && CACHE.scale == scale && CACHE.glyph == glyph && CACHE.glyphShadow == glyphShadow)
        return CACHE.tex;

    CACHE             = {};
    CACHE.size        = size;
    CACHE.scale       = scale;
    CACHE.glyph       = glyph;
    CACHE.glyphShadow = glyphShadow;

    auto SURFACE = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    if (cairo_surface_status(SURFACE) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(SURFACE);
        return nullptr;
    }

    auto* CAIRO = cairo_create(SURFACE);
    if (cairo_status(CAIRO) == CAIRO_STATUS_SUCCESS) {
        // Only the glyph is drawn here. The face and bevel are rendered by the decoration itself.
        cairo_save(CAIRO);
        cairo_set_operator(CAIRO, CAIRO_OPERATOR_CLEAR);
        cairo_paint(CAIRO);
        cairo_restore(CAIRO);

        const double LINEWIDTH = std::max(1.0, sc<double>(std::round(scale)));
        const double INSET     = std::max(2.0, std::round(size * 0.28));
        const double SHADOWOFF = std::max(1.0, sc<double>(std::round(scale)));

        auto         drawX = [&](double off, const CHyprColor& color) {
            cairo_set_source_rgba(CAIRO, color.r, color.g, color.b, color.a);
            cairo_set_line_width(CAIRO, LINEWIDTH);
            cairo_set_line_cap(CAIRO, CAIRO_LINE_CAP_SQUARE);

            cairo_move_to(CAIRO, INSET + off, INSET + off);
            cairo_line_to(CAIRO, size - INSET + off, size - INSET + off);
            cairo_move_to(CAIRO, size - INSET + off, INSET + off);
            cairo_line_to(CAIRO, INSET + off, size - INSET + off);
            cairo_stroke(CAIRO);
        };

        if (glyphShadow.a > 0.F)
            drawX(SHADOWOFF, glyphShadow);

        drawX(0.0, glyph);
    }
    cairo_destroy(CAIRO);

    cairo_surface_flush(SURFACE);
    CACHE.tex = g_pHyprRenderer->createTexture(SURFACE);
    cairo_surface_destroy(SURFACE);

    return CACHE.tex;
}

void CHyprTitleBarDecoration::draw(Render::CRenderContext& ctx, PHLMONITOR pMonitor, float const& a, const Render::SWindowRenderPresentation& presentation) {
    const auto PWINDOW = m_window.lock();
    if (!PWINDOW || !visible() || m_assignedGeometry.width < 1 || m_assignedGeometry.height < 1)
        return;

    static auto PCLOSEBUTTON  = CConfigValue<Config::BOOL>("decoration:titlebar:close_button");
    static auto PFONTFALLBACK = CConfigValue<Config::STRING>("misc:font_family");
    static auto PFONTFAMILY   = CConfigValue<Config::STRING>("decoration:titlebar:font_family");
    static auto PFONTSIZE     = CConfigValue<Config::INTEGER>("decoration:titlebar:font_size");

    if (m_pressedButton != PRESSED_BUTTON_NONE && !g_pInputManager->hasHeldButtons())
        m_pressedButton = PRESSED_BUTTON_NONE;

    const bool ACTIVE = Desktop::focusState()->window() == PWINDOW;
    const auto FACE   = faceColor(ACTIVE);
    const auto TEXT   = textColor(ACTIVE);
    const auto SHADOW = shadowColor(FACE, TEXT);

    const auto SCALE    = pMonitor->m_scale;
    const auto BOX      = assignedBoxGlobal(presentation).translate(-pMonitor->m_position + presentation.floatingOffset);
    CBox       physical = BOX.copy().scale(SCALE).round();

    if (physical.w < 1 || physical.h < 1)
        return;

    const int               SEP     = std::max(1, sc<int>(std::round(SCALE)));
    const int               SEPSIZE = SEP * 2;
    const int               PADDING = scaledThickness(TitleBar::TEXT_PADDING, SCALE);
    const int               MINBTN  = std::max(3, sc<int>(std::round(TitleBar::MIN_BUTTON_SIZE * SCALE)));

    TitleBar::SLayoutConfig layoutConfig;
    layoutConfig.separator     = SEPSIZE;
    layoutConfig.padding       = PADDING;
    layoutConfig.minButtonSize = MINBTN;
    layoutConfig.closeButton   = *PCLOSEBUTTON;

    const auto LAYOUT = TitleBar::layout(physical, layoutConfig);

    auto       faceFill = FACE;
    faceFill.a *= a;

    CRectPassElement::SRectData faceData;
    faceData.color                 = faceFill;
    faceData.box                   = LAYOUT.titleBar;
    faceData.workspacePresentation = presentation;
    g_pHyprRenderer->addPassElement(ctx, makeUnique<CRectPassElement>(faceData));

    // The title bar gets its own embossed edge, inside the frame's inner bevel.
    const int BEVELPX = std::max(1, sc<int>(std::round(SCALE)));
    if (LAYOUT.titleBar.w > 2 * BEVELPX && LAYOUT.titleBar.h > 2 * BEVELPX) {
        CBorderPassElement::SBorderData bevelData;
        bevelData.box        = LAYOUT.titleBar.copy().expand(-BEVELPX);
        bevelData.grad1      = Config::CGradientValueData{FACE};
        bevelData.borderSize = 1;
        bevelData.round      = 0;
        bevelData.outerRound = 0;
        bevelData.a          = a;
        bevelData.window     = PWINDOW;
        g_pHyprRenderer->addPassElement(ctx, makeUnique<CBorderPassElement>(bevelData));
    }

    if (LAYOUT.titleBar.h > SEPSIZE) {
        auto dark  = Motif::shade(FACE, false);
        auto light = Motif::shade(FACE, true);
        dark.a *= a;
        light.a *= a;

        CRectPassElement::SRectData darkData;
        darkData.color                 = dark;
        darkData.box                   = {LAYOUT.titleBar.x, LAYOUT.titleBar.y + LAYOUT.titleBar.h - SEPSIZE, LAYOUT.titleBar.w, sc<double>(SEP)};
        darkData.workspacePresentation = presentation;
        g_pHyprRenderer->addPassElement(ctx, makeUnique<CRectPassElement>(darkData));

        CRectPassElement::SRectData lightData;
        lightData.color                 = light;
        lightData.box                   = {LAYOUT.titleBar.x, LAYOUT.titleBar.y + LAYOUT.titleBar.h - SEP, LAYOUT.titleBar.w, sc<double>(SEP)};
        lightData.workspacePresentation = presentation;
        g_pHyprRenderer->addPassElement(ctx, makeUnique<CRectPassElement>(lightData));
    }

    const auto TITLE = PWINDOW->metadata().title();
    if (!TITLE.empty() && LAYOUT.textArea.w >= 1 && LAYOUT.textArea.h >= 1) {
        const int  FONTSIZE   = std::clamp(sc<int>(std::round(*PFONTSIZE * SCALE)), 1, std::max(1, sc<int>(LAYOUT.textArea.h)));
        const auto CONFIGFONT = *PFONTFAMILY;
        const auto FONT       = !CONFIGFONT.empty() && CONFIGFONT != STRVAL_EMPTY ? CONFIGFONT : *PFONTFALLBACK;

        ensureTitleTextures(TITLE, ACTIVE, SCALE, std::max(1, sc<int>(LAYOUT.textArea.w)), FONTSIZE, FONT, TEXT, SHADOW);

        if (m_titleTextures.main && m_titleTextures.main->ok()) {
            const auto X = sc<int>(std::round(LAYOUT.textArea.x + (LAYOUT.textArea.w - m_titleTextures.main->m_size.x) / 2.0));
            const auto Y = sc<int>(std::round(LAYOUT.textArea.y + (LAYOUT.textArea.h - m_titleTextures.main->m_size.y) / 2.0));

            if (m_titleTextures.shadowTex && m_titleTextures.shadowTex->ok()) {
                const int                    OFFSET = std::max(1, sc<int>(std::round(SCALE)));

                CTexPassElement::SRenderData shadowData;
                shadowData.tex                   = m_titleTextures.shadowTex;
                shadowData.box                   = {sc<double>(X + OFFSET), sc<double>(Y + OFFSET), m_titleTextures.shadowTex->m_size.x, m_titleTextures.shadowTex->m_size.y};
                shadowData.a                     = a;
                shadowData.clipRegion            = CRegion{LAYOUT.textArea};
                shadowData.workspacePresentation = presentation;
                g_pHyprRenderer->addPassElement(ctx, makeUnique<CTexPassElement>(shadowData));
            }

            CTexPassElement::SRenderData titleData;
            titleData.tex                   = m_titleTextures.main;
            titleData.box                   = {sc<double>(X), sc<double>(Y), m_titleTextures.main->m_size.x, m_titleTextures.main->m_size.y};
            titleData.a                     = a;
            titleData.clipRegion            = CRegion{LAYOUT.textArea};
            titleData.workspacePresentation = presentation;
            g_pHyprRenderer->addPassElement(ctx, makeUnique<CTexPassElement>(titleData));
        }
    }

    if (LAYOUT.closeButton) {
        const bool PRESSED     = m_pressedButton == PRESSED_BUTTON_CLOSE;
        const auto buttonColor = PRESSED ? Motif::shade(FACE, false) : FACE;

        auto       buttonFill = buttonColor;
        buttonFill.a *= a;

        CRectPassElement::SRectData buttonData;
        buttonData.color                 = buttonFill;
        buttonData.box                   = *LAYOUT.closeButton;
        buttonData.workspacePresentation = presentation;
        g_pHyprRenderer->addPassElement(ctx, makeUnique<CRectPassElement>(buttonData));

        if (LAYOUT.closeButton->w > 2 * BEVELPX && LAYOUT.closeButton->h > 2 * BEVELPX) {
            CBorderPassElement::SBorderData buttonBevel;
            buttonBevel.box        = LAYOUT.closeButton->copy().expand(-BEVELPX);
            buttonBevel.grad1      = Config::CGradientValueData{buttonColor};
            buttonBevel.borderSize = 1;
            buttonBevel.round      = 0;
            buttonBevel.outerRound = 0;
            buttonBevel.a          = a;
            buttonBevel.window     = PWINDOW;
            g_pHyprRenderer->addPassElement(ctx, makeUnique<CBorderPassElement>(buttonBevel));
        }

        const auto TEX = ensureCloseGlyph(sc<int>(LAYOUT.closeButton->w), SCALE, TEXT, SHADOW);
        if (TEX && TEX->ok()) {
            CTexPassElement::SRenderData closeData;
            closeData.tex                   = TEX;
            closeData.box                   = *LAYOUT.closeButton;
            closeData.a                     = a;
            closeData.workspacePresentation = presentation;
            g_pHyprRenderer->addPassElement(ctx, makeUnique<CTexPassElement>(closeData));
        }
    }
}

bool CHyprTitleBarDecoration::onMouseButton(const Vector2D& coords, const IPointer::SButtonEvent& e) {
    static auto PFOLLOWMOUSE = CConfigValue<Config::INTEGER>("input:follow_mouse");

    const auto  PWINDOW = m_window.lock();
    if (!PWINDOW)
        return false;

    static auto PCLOSEBUTTON = CConfigValue<Config::BOOL>("decoration:titlebar:close_button");

    const auto  LAYOUT = TitleBar::layout(inputBox(), logicalLayoutConfig(*PCLOSEBUTTON));

    if (e.state == WL_POINTER_BUTTON_STATE_PRESSED) {
        if (e.button == 272 && LAYOUT.closeButton && LAYOUT.closeButton->containsPoint(coords)) {
            m_pressedButton = PRESSED_BUTTON_CLOSE;
            damageEntire();
        }

        if (!Desktop::focusState()->isWindowActive(PWINDOW) && *PFOLLOWMOUSE != 3)
            Desktop::focusState()->rawWindowFocus(PWINDOW, Desktop::FOCUS_REASON_CLICK_DOWN);

        if (PWINDOW->isFloating())
            Desktop::windowState()->raise(PWINDOW);

        return true;
    }

    if (e.state == WL_POINTER_BUTTON_STATE_RELEASED) {
        if (m_pressedButton == PRESSED_BUTTON_CLOSE) {
            const bool STILLON = LAYOUT.closeButton && LAYOUT.closeButton->containsPoint(coords);

            m_pressedButton = PRESSED_BUTTON_NONE;
            damageEntire();

            if (STILLON && PWINDOW->m_closeableSince <= Time::steadyNow())
                PWINDOW->sendClose();
        }

        return true;
    }

    return true;
}

bool CHyprTitleBarDecoration::onInputOnDeco(const eInputType type, const Vector2D& mouseCoords, std::any data) {
    if (!visible())
        return false;

    switch (type) {
        case INPUT_TYPE_BUTTON: return onMouseButton(mouseCoords, std::any_cast<const IPointer::SButtonEvent&>(data));
        case INPUT_TYPE_DRAG_START: {
            static auto PCLOSEBUTTON = CConfigValue<Config::BOOL>("decoration:titlebar:close_button");

            const auto  LAYOUT = TitleBar::layout(inputBox(), logicalLayoutConfig(*PCLOSEBUTTON));
            return LAYOUT.closeButton && LAYOUT.closeButton->containsPoint(mouseCoords);
        }
        default: return false;
    }
}

void CHyprTitleBarDecoration::updateWindow(PHLWINDOW) {
    const auto PWINDOW = m_window.lock();
    if (!PWINDOW)
        return;

    const auto HEIGHT = visible() ? height() : 0;

    if (HEIGHT != m_lastHeight) {
        damageEntire();
        m_lastHeight = HEIGHT;
        g_pDecorationPositioner->repositionDeco(this);
    }

    const auto& TITLE = PWINDOW->metadata().title();
    if (TITLE != m_lastTitle) {
        m_lastTitle = TITLE;
        invalidateTextures();
        damageEntire();
    }
}

void CHyprTitleBarDecoration::updateState() {
    const auto PWINDOW = m_window.lock();
    if (!PWINDOW)
        return;

    const bool ACTIVE = Desktop::focusState()->window() == PWINDOW;
    const auto FACE   = faceColor(ACTIVE);

    if (!m_lastActive.has_value() || *m_lastActive != ACTIVE || !m_lastFaceColor.has_value() || *m_lastFaceColor != FACE) {
        m_lastActive    = ACTIVE;
        m_lastFaceColor = FACE;

        invalidateTextures();
        invalidateCloseGlyph();
        damageEntire();
    }
}

void CHyprTitleBarDecoration::damageEntire() {
    const auto PWINDOW = m_window.lock();
    if (!PWINDOW)
        return;

    CBox box = assignedBoxGlobal(PWINDOW->presentation().renderPresentation());
    if (box.w <= 0 || box.h <= 0)
        return;

    box.translate(PWINDOW->presentation().floatingOffset());
    g_pHyprRenderer->damageBox(box.expand(2));
}

eDecorationType CHyprTitleBarDecoration::getDecorationType() {
    return DECORATION_TITLEBAR;
}

eDecorationLayer CHyprTitleBarDecoration::getDecorationLayer() {
    return DECORATION_LAYER_OVER;
}

uint64_t CHyprTitleBarDecoration::getDecorationFlags() {
    return DECORATION_ALLOWS_MOUSE_INPUT | DECORATION_PART_OF_MAIN_WINDOW;
}

std::string CHyprTitleBarDecoration::getDisplayName() {
    return "TitleBar";
}
