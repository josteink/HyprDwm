#pragma once

#include "IHyprWindowDecoration.hpp"
#include "../Texture.hpp"
#include "../../devices/IPointer.hpp"
#include "../../helpers/Color.hpp"

#include <optional>
#include <string>

class CHyprTitleBarDecoration : public IHyprWindowDecoration {
  public:
    CHyprTitleBarDecoration(PHLWINDOW);
    virtual ~CHyprTitleBarDecoration() = default;

    virtual SDecorationPositioningInfo getPositioningInfo() override;

    virtual void                       onPositioningReply(const SDecorationPositioningReply& reply) override;

    virtual void                       draw(Render::CRenderContext& ctx, PHLMONITOR, float const& a, const Render::SWindowRenderPresentation& presentation) override;

    virtual eDecorationType            getDecorationType() override;

    virtual void                       updateWindow(PHLWINDOW) override;

    virtual void                       damageEntire() override;

    virtual bool                       onInputOnDeco(const eInputType, const Vector2D&, std::any = {}) override;

    virtual eDecorationLayer           getDecorationLayer() override;

    virtual uint64_t                   getDecorationFlags() override;

    virtual std::string                getDisplayName() override;

    virtual void                       updateState() override;

    int                                currentHeight() const;

  private:
    struct STitleTextures {
        std::string          content;
        std::string          fontFamily;
        bool                 active   = false;
        float                scale    = 0.F;
        int                  maxWidth = 0;
        int                  fontSize = 0;
        CHyprColor           textColor;
        CHyprColor           shadowColor;
        SP<Render::ITexture> main;
        SP<Render::ITexture> shadowTex;
    };

    struct SCloseGlyph {
        int                  size  = 0;
        float                scale = 0.F;
        CHyprColor           glyph;
        CHyprColor           glyphShadow;
        SP<Render::ITexture> tex;
    };

    enum ePressedButton : uint8_t {
        PRESSED_BUTTON_NONE = 0,
        PRESSED_BUTTON_CLOSE,
    };

    PHLWINDOWREF              m_window;
    CBox                      m_assignedGeometry = {0};

    int                       m_lastHeight = -1;
    std::string               m_lastTitle;
    std::optional<bool>       m_lastActive;
    std::optional<CHyprColor> m_lastFaceColor;
    ePressedButton            m_pressedButton = PRESSED_BUTTON_NONE;

    STitleTextures            m_titleTextures;
    SCloseGlyph               m_closeGlyph;

    int                       height() const;
    bool                      visible() const;
    CHyprColor                faceColor(bool active) const;
    CHyprColor                textColor(bool active) const;
    CHyprColor                shadowColor(const CHyprColor& face, const CHyprColor& text) const;
    CBox                      assignedBoxGlobal(const Render::SWindowRenderPresentation& presentation) const;
    CBox                      inputBox() const;

    void                      invalidateTextures();
    void                      invalidateCloseGlyph();
    void                 ensureTitleTextures(const std::string& title, bool active, float scale, int maxWidth, int fontSize, const std::string& fontFamily, const CHyprColor& text,
                                             const CHyprColor& shadow);
    SP<Render::ITexture> ensureCloseGlyph(int size, float scale, const CHyprColor& glyph, const CHyprColor& glyphShadow);
    bool                 onMouseButton(const Vector2D& coords, const IPointer::SButtonEvent& e);
};
