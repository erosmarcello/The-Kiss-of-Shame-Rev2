#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ShameResources.h"

//==============================================================================
KissOfShameAudioProcessorEditor::KissOfShameAudioProcessorEditor(KissOfShameAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    using namespace ShameResources;

    faceWithReels    = getImage(BinaryData::fond_alpha_png, BinaryData::fond_alpha_pngSize);
    faceWithoutReels = getImage(BinaryData::fond_alone2_png, BinaryData::fond_alone2_pngSize);

    backlight.setTopLeftPosition(0, 703 - backlight.getHeight());
    content.addAndMakeVisible(backlight);

    faceImage.setNumFrames(1);
    faceImage.setDimensions(0, 0, 960, 703);
    faceImage.setAnimationImage(faceWithReels);
    faceImage.setInterceptsMouseClicks(false, false);
    content.addAndMakeVisible(faceImage);

    /////////// COMPONENTS /////////////////

    environmentsComponent.setAnimationImage(getImage(BinaryData::_00_png, BinaryData::_00_pngSize));
    environmentsComponent.setTopLeftPosition(388, 654);
    content.addAndMakeVisible(environmentsComponent);

    ////////// KNOBS ////////////////

    inputSaturationKnob.setKnobImage(getImage(BinaryData::_06_alpha_png, BinaryData::_06_alpha_pngSize));
    inputSaturationKnob.setNumFrames(65);
    inputSaturationKnob.setKnobDimensions(104, 521, 116, 116);
    content.addAndMakeVisible(inputSaturationKnob);

    shameKnobImage.setNumFrames(65);
    shameKnobImage.setDimensions(401, 491, 174, 163);
    shameKnobImage.setAnimationImage(getImage(BinaryData::_09_alpha_png, BinaryData::_09_alpha_pngSize));
    content.addAndMakeVisible(shameKnobImage);

    shameKnob.setKnobImage(getImage(BinaryData::_09_v2_png, BinaryData::_09_v2_pngSize));
    shameKnob.setNumFrames(65);
    shameKnob.setKnobDimensions(401, 491, 174, 163);
    shameKnob.onValueChange = [this] { shameKnobImage.updateImageWithValue((float) shameKnob.getValue()); };
    shameKnob.onDoubleClick = [this] { toggleExtreme(); };
    content.addAndMakeVisible(shameKnob);

    hissKnob.setKnobImage(getImage(BinaryData::_04_alpha_png, BinaryData::_04_alpha_pngSize));
    hissKnob.setNumFrames(65);
    hissKnob.setKnobDimensions(547, 455, 78, 72);
    content.addAndMakeVisible(hissKnob);

    blendKnob.setKnobImage(getImage(BinaryData::_05_alpha_png, BinaryData::_05_alpha_pngSize));
    blendKnob.setNumFrames(65);
    blendKnob.setKnobDimensions(705, 455, 78, 72);
    content.addAndMakeVisible(blendKnob);

    outputKnob.setKnobImage(getImage(BinaryData::_12_alpha_png, BinaryData::_12_alpha_pngSize));
    outputKnob.setNumFrames(65);
    outputKnob.setKnobDimensions(757, 521, 122, 116);
    content.addAndMakeVisible(outputKnob);

    ageKnob.setKnobImage(getImage(BinaryData::_03_alpha_png, BinaryData::_03_alpha_pngSize));
    ageKnob.setNumFrames(65);
    ageKnob.setKnobDimensions(350, 455, 74, 72);
    content.addAndMakeVisible(ageKnob);

    // The input/output link: invert-couples the two gain knobs.
    inputSaturationKnob.onValueChange = [this]
    {
        if (linkIOMode)
            outputKnob.setValue(1.0 - inputSaturationKnob.getValue());
    };
    outputKnob.onValueChange = [this]
    {
        if (linkIOMode)
            inputSaturationKnob.setValue(1.0 - outputKnob.getValue());
    };

    ///////////// BUTTONS /////////////////

    const auto bypassImage = getImage(BinaryData::_01_png, BinaryData::_01_pngSize);
    bypassButton.setTopLeftPosition(202, 469);
    bypassButton.setClippedCustomOnImage(bypassImage, 0, 68, 34, 34);
    bypassButton.setClippedCustomOffImage(bypassImage, 0, 0, 34, 34);
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this]
    {
        const bool bypassed = bypassButton.getToggleState();
        backlight.setAlpha(bypassed ? 0.25f : 1.0f);
        shameKnob.setAlpha(bypassed ? 0.25f : 1.0f);
    };
    content.addAndMakeVisible(bypassButton);

    // The lost lever: 02.png, specified at (232, 502) in positions.rtf but
    // never wired into Rev 1. Two frames — lever up (signal flowing) and
    // lever thrown (bypassed). The lamp shows the state; this throws it.
    const auto leverImage = getImage(BinaryData::_02_png, BinaryData::_02_pngSize);
    bypassLever.setTopLeftPosition(232, 502);
    bypassLever.setClippedCustomOnImage(leverImage, 0, 0, 44, 37);
    bypassLever.setClippedCustomOffImage(leverImage, 0, 37, 44, 37);
    bypassLever.setClickingTogglesState(true);
    content.addAndMakeVisible(bypassLever);

    const auto tapeTypeImage = getImage(BinaryData::_07_png, BinaryData::_07_pngSize);
    tapeTypeButton.setTopLeftPosition(233, 610);
    tapeTypeButton.setClippedCustomOnImage(tapeTypeImage, 0, 0, 42, 39);
    tapeTypeButton.setClippedCustomOffImage(tapeTypeImage, 0, 39, 42, 39);
    tapeTypeButton.setClickingTogglesState(true);
    content.addAndMakeVisible(tapeTypeButton);

    const auto printThroughImage = getImage(BinaryData::_11_png, BinaryData::_11_pngSize);
    printThroughButton.setTopLeftPosition(698, 609);
    printThroughButton.setClippedCustomOnImage(printThroughImage, 0, 41, 47, 41);
    printThroughButton.setClippedCustomOffImage(printThroughImage, 0, 0, 47, 41);
    printThroughButton.setClickingTogglesState(true);
    content.addAndMakeVisible(printThroughButton);

    const auto linkImage = getImage(BinaryData::link_png, BinaryData::link_pngSize);
    linkIOButtonL.setTopLeftPosition(137, 605);
    linkIOButtonL.setClippedCustomOnImage(linkImage, 0, 0, 50, 50);
    linkIOButtonL.setClippedCustomOffImage(linkImage, 0, 0, 50, 50);
    linkIOButtonL.resizeButton(0.6f);
    linkIOButtonL.setClickingTogglesState(true);
    content.addAndMakeVisible(linkIOButtonL);

    linkIOButtonR.setTopLeftPosition(792, 605);
    linkIOButtonR.setClippedCustomOnImage(linkImage, 0, 0, 50, 50);
    linkIOButtonR.setClippedCustomOffImage(linkImage, 0, 0, 50, 50);
    linkIOButtonR.resizeButton(0.6f);
    linkIOButtonR.setClickingTogglesState(true);
    content.addAndMakeVisible(linkIOButtonR);

    auto handleLinkClick = [this](CustomButton& clicked)
    {
        const bool linked = clicked.getToggleState();
        linkIOButtonL.setToggleState(linked, dontSendNotification);
        linkIOButtonR.setToggleState(linked, dontSendNotification);
        linkIOMode = linked;

        linkIOButtonL.setAlpha(linked ? 1.0f : 0.25f);
        linkIOButtonR.setAlpha(linked ? 1.0f : 0.25f);

        if (linked)
            outputKnob.setValue(1.0 - inputSaturationKnob.getValue());
    };
    linkIOButtonL.onClick = [this, handleLinkClick] { handleLinkClick(linkIOButtonL); };
    linkIOButtonR.onClick = [this, handleLinkClick] { handleLinkClick(linkIOButtonR); };

    linkIOButtonL.setAlpha(0.25f);
    linkIOButtonR.setAlpha(0.25f);

    ///////////////// Animation //////////////////

    reelAnimation = std::make_unique<ImageAnimationComponent>(
        getImage(BinaryData::wheels_png, BinaryData::wheels_pngSize), 31, 50);
    reelAnimation->setFrameDimensions(0, 0, 960, 322);
    content.addAndMakeVisible(*reelAnimation);

    vuMeterL.setNumFrames(65);
    vuMeterL.setDimensions(251, 518, 108, 108);
    vuMeterL.setAnimationImage(getImage(BinaryData::_08_png, BinaryData::_08_pngSize));
    content.addAndMakeVisible(vuMeterL);

    vuMeterR.setNumFrames(65);
    vuMeterR.setDimensions(605, 518, 110, 108);
    vuMeterR.setAnimationImage(getImage(BinaryData::_10_png, BinaryData::_10_pngSize));
    content.addAndMakeVisible(vuMeterR);

    ///////////////// Parameter bindings //////////////////

    auto& apvts = processor.apvts;

    inputAttachment  = std::make_unique<SliderAttachment>(apvts, ParamIDs::input, inputSaturationKnob);
    shameAttachment  = std::make_unique<SliderAttachment>(apvts, ParamIDs::shame, shameKnob);
    hissAttachment   = std::make_unique<SliderAttachment>(apvts, ParamIDs::hiss, hissKnob);
    blendAttachment  = std::make_unique<SliderAttachment>(apvts, ParamIDs::blend, blendKnob);
    outputAttachment = std::make_unique<SliderAttachment>(apvts, ParamIDs::output, outputKnob);
    ageAttachment    = std::make_unique<SliderAttachment>(apvts, ParamIDs::age, ageKnob);

    bypassAttachment       = std::make_unique<ButtonAttachment>(apvts, ParamIDs::bypass, bypassButton);
    bypassLeverAttachment  = std::make_unique<ButtonAttachment>(apvts, ParamIDs::bypass, bypassLever);
    tapeTypeAttachment     = std::make_unique<ButtonAttachment>(apvts, ParamIDs::tapeType, tapeTypeButton);
    printThroughAttachment = std::make_unique<ButtonAttachment>(apvts, ParamIDs::printThrough, printThroughButton);

    if (auto* flangeParam = apvts.getParameter(ParamIDs::flange))
    {
        flangeAttachment = std::make_unique<ParameterAttachment>(*flangeParam, [](float) {});

        reelAnimation->onFlangeGestureStart = [this] { flangeAttachment->beginGesture(); };
        reelAnimation->onFlangeDepthChanged = [this](float depth) { flangeAttachment->setValueAsPartOfGesture(depth); };
        reelAnimation->onFlangeGestureEnd   = [this] { flangeAttachment->endGesture(); };
    }

    if (auto* envParam = apvts.getParameter(ParamIDs::environment))
    {
        environmentAttachment = std::make_unique<ParameterAttachment>(*envParam,
            [this](float newValue) { environmentsComponent.setDisplayedEnvironment((int) std::lround(newValue)); });
        environmentAttachment->sendInitialUpdate();

        environmentsComponent.onEnvironmentChanged = [this](int index)
        { environmentAttachment->setValueAsCompleteGesture((float) index); };
    }

    // Keep the strip's intensity underline (and picker footer) honest about
    // what AGE is doing to the chosen environment.
    ageKnob.onValueChange = [this]
    { environmentsComponent.setAgeAmount((float) ageKnob.getValue()); };
    environmentsComponent.setAgeAmount((float) ageKnob.getValue());

    // The era experiment is retired: the original 2014 faceplate is the
    // interface. (The switch component stays in the codebase, un-added.)
    eraSwitch.onEraToggled = nullptr;

    shameKnob.setModernCross(true);
    bypassButton.setModernStyle(CustomButton::ModernStyle::lampKnob);
    tapeTypeButton.setModernStyle(CustomButton::ModernStyle::crossLamp);
    printThroughButton.setModernStyle(CustomButton::ModernStyle::pill);
    vuMeterL.setModernStyle(ImageInteractor::ModernStyle::vuMeter);
    vuMeterR.setModernStyle(ImageInteractor::ModernStyle::vuMeter);

    ///////////////// Scaled canvas + initial state //////////////////

    content.onPaint = [this](Graphics& g)
    {
        if (era == UIEra::modern)
            paintModernPanel(g);
        else
            g.fillAll(Colours::black);
    };
    content.onPaintOver = [this](Graphics& g) { paintContentOverChildren(g); };
    content.onDoubleClick = [this] { toggleReels(); };
    addAndMakeVisible(content);

    shameKnobImage.updateImageWithValue((float) shameKnob.getValue());
    backlight.setExtreme(processor.isShameExtreme());
    shameKnob.setExtremeVisual(processor.isShameExtreme());

    showReels = processor.getShowReels();
    content.setBounds(0, 0, 960, 703);
    if (! showReels)
    {
        setReelMode(false);
        applyReelVisibility();
    }

    setResizable(true, true);
    applyEra(UIEra::heritage, false); // the original UI, always

    constructionComplete = true;
    startTimer(25);
}

int KissOfShameAudioProcessorEditor::logicalHeight() const
{
    return showReels ? 703 : (era == UIEra::modern ? 381 : 266);
}

void KissOfShameAudioProcessorEditor::refreshCanvasSize()
{
    // The scale must be read BEFORE the resize limits are applied:
    // installing them can resize a 0x0 editor to the minimum, and resized()
    // must never persist that transient (constructionComplete guards it).
    const float scale = getWidth() > 0 ? (float) getWidth() / 960.0f : processor.getUIScale();
    content.setSize(960, logicalHeight());
    updateSizeConstraints();
    setSize((int) std::lround(960.0f * scale), (int) std::lround((float) logicalHeight() * scale));
}

//==============================================================================
void KissOfShameAudioProcessorEditor::resized()
{
    const float scale = (float) getWidth() / 960.0f;
    content.setTransform(AffineTransform::scale(scale));

    if (constructionComplete)
        processor.setUIScale(scale);
}

void KissOfShameAudioProcessorEditor::updateSizeConstraints()
{
    const int logicalH = logicalHeight();
    setResizeLimits((int) (960 * 0.6), (int) (logicalH * 0.6),
                    (int) (960 * 2.0), (int) (logicalH * 2.0));
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio(960.0 / (double) logicalH);
}

//==============================================================================
void KissOfShameAudioProcessorEditor::applyEra(UIEra newEra, bool animate)
{
    if (animate)
    {
        eraTransition = std::make_unique<EraTransitionOverlay>(createComponentSnapshot(getLocalBounds()));
        eraTransition->setBounds(getLocalBounds());
        eraTransition->onFinished = [this]
        {
            MessageManager::callAsync([safeThis = Component::SafePointer<KissOfShameAudioProcessorEditor>(this)]
            {
                if (safeThis != nullptr)
                    safeThis->eraTransition.reset();
            });
        };
    }

    era = newEra;
    processor.setUIEra(era == UIEra::modern ? "modern" : "heritage");

    backlight.setEra(era);
    faceImage.setEra(era);
    environmentsComponent.setEra(era);

    inputSaturationKnob.setEra(era);
    shameKnob.setEra(era);
    hissKnob.setEra(era);
    blendKnob.setEra(era);
    outputKnob.setEra(era);
    ageKnob.setEra(era);

    bypassButton.setEra(era);
    tapeTypeButton.setEra(era);
    printThroughButton.setEra(era);
    linkIOButtonL.setEra(era);
    linkIOButtonR.setEra(era);

    reelAnimation->setEra(era);
    vuMeterL.setEra(era);
    vuMeterR.setEra(era);
    shameKnobImage.setEra(era);

    eraSwitch.setEra(era);

    positionEraDependentControls();
    refreshCanvasSize();

    if (animate && eraTransition != nullptr)
        addAndMakeVisible(*eraTransition);

    repaint();
}

void KissOfShameAudioProcessorEditor::positionEraDependentControls()
{
    if (era == UIEra::modern)
    {
        // The shelved-design layout: deck spans y 322..703 under the reels.
        const int dy = showReels ? 0 : -322;

        bypassButton.setBounds(44, 348 + dy, 48, 58);
        bypassLever.setVisible(false);
        ageKnob.setBounds(280, 348 + dy, 66, 66);
        hissKnob.setBounds(606, 348 + dy, 66, 66);
        blendKnob.setBounds(712, 348 + dy, 66, 66);
        inputSaturationKnob.setBounds(22, 416 + dy, 144, 144);
        outputKnob.setBounds(794, 416 + dy, 144, 144);
        vuMeterL.setBounds(176, 440 + dy, 156, 110);
        vuMeterR.setBounds(628, 440 + dy, 156, 110);
        shameKnob.setBounds(355, 394 + dy, 250, 250);
        environmentsComponent.setBounds(358, 656 + dy, 244, 28);
        printThroughButton.setBounds(692, 574 + dy, 66, 26);
        tapeTypeButton.setBounds(36, 628 + dy, 38, 56);
        eraSwitch.setBounds(866, 664 + dy, 80, 26);
        backlight.setBounds(0, showReels ? 322 : 0, 960, 381);

        faceImage.setVisible(false);
        shameKnobImage.setVisible(false);
        linkIOButtonL.setVisible(false);
        linkIOButtonR.setVisible(false);
    }
    else
    {
        // Heritage positions are engraved into the faceplate artwork.
        const int dy = showReels ? 0 : -437;

        bypassButton.setBounds(202, 469 + dy, 34, 34);
        bypassLever.setBounds(232, 502 + dy, 44, 37);
        bypassLever.setVisible(true);
        ageKnob.setBounds(350, 455 + dy, 74, 72);
        hissKnob.setBounds(547, 455 + dy, 78, 72);
        blendKnob.setBounds(705, 455 + dy, 78, 72);
        inputSaturationKnob.setBounds(104, 521 + dy, 116, 116);
        outputKnob.setBounds(757, 521 + dy, 122, 116);
        vuMeterL.setBounds(251, 518 + dy, 108, 108);
        vuMeterR.setBounds(605, 518 + dy, 110, 108);
        shameKnob.setBounds(401, 491 + dy, 174, 163);
        shameKnobImage.setBounds(401, 491 + dy, 174, 163);
        environmentsComponent.setBounds(388, 654 + dy, 183, 32);
        printThroughButton.setBounds(698, 609 + dy, 47, 41);
        tapeTypeButton.setBounds(233, 610 + dy, 42, 39);
        linkIOButtonL.setBounds(137, 605 + dy, 30, 30);
        linkIOButtonR.setBounds(792, 605 + dy, 30, 30);
        eraSwitch.setBounds(862, 656 + dy, 84, 30);
        backlight.setBounds(0, showReels ? 437 : 0, 960, 266);

        faceImage.setVisible(true);
        shameKnobImage.setVisible(true);
        linkIOButtonL.setVisible(true);
        linkIOButtonR.setVisible(true);
    }

    statusBar.setVisible(false);
}

//==============================================================================
void KissOfShameAudioProcessorEditor::toggleExtreme()
{
    const bool extreme = ! processor.isShameExtreme();
    processor.setShameExtreme(extreme);
    backlight.setExtreme(extreme);
    shameKnob.setExtremeVisual(extreme);
    repaint();
}

//==============================================================================
void KissOfShameAudioProcessorEditor::setReelMode(bool shouldShowReels)
{
    const int adjustment = shouldShowReels ? 437 : -437; // height difference: 703 - 266

    auto shift = [adjustment](Component& c)
    { c.setTopLeftPosition(c.getX(), c.getY() + adjustment); };

    shift(backlight);

    shift(inputSaturationKnob);
    shift(shameKnob);
    shift(hissKnob);
    shift(blendKnob);
    shift(outputKnob);
    shift(ageKnob);

    shift(bypassButton);
    shift(tapeTypeButton);
    shift(printThroughButton);
    shift(linkIOButtonL);
    shift(linkIOButtonR);

    shift(environmentsComponent);
    shift(eraSwitch);

    shift(vuMeterL);
    shift(vuMeterR);
    shift(shameKnobImage);

    if (shouldShowReels)
    {
        faceImage.setAnimationImage(faceWithReels);
        faceImage.setDimensions(0, 0, 960, 703);
    }
    else
    {
        faceImage.setAnimationImage(faceWithoutReels);
        faceImage.setDimensions(0, 0, 960, 266);
    }
}

void KissOfShameAudioProcessorEditor::applyReelVisibility()
{
    if (showReels)
        content.addAndMakeVisible(*reelAnimation);
    else
        content.removeChildComponent(reelAnimation.get());
}

void KissOfShameAudioProcessorEditor::toggleReels()
{
    showReels = ! showReels;
    processor.setShowReels(showReels);

    if (era == UIEra::heritage)
        setReelMode(showReels);

    applyReelVisibility();
    positionEraDependentControls();
    refreshCanvasSize();
    repaint();
}

//==============================================================================
void KissOfShameAudioProcessorEditor::timerCallback()
{
    // VU meters track the processed level unless bypassed.
    const float vuLevelL = bypassButton.getToggleState() ? 0.0f : processor.getCurrentRMSL() * 3.0f;
    const float vuLevelR = bypassButton.getToggleState() ? 0.0f : processor.getCurrentRMSR() * 3.0f;
    vuMeterL.updateImageWithValue(vuLevelL);
    vuMeterR.updateImageWithValue(vuLevelR);

    // The light follows the program: backlight ember and the cross's halo.
    const float programGlow = jlimit(0.0f, 1.0f, 0.5f * (vuLevelL + vuLevelR));
    backlight.setAudioLevel(programGlow);
    shameKnob.setGlowLevel(programGlow);

    // Reels spin only while the host transport runs and audio is flowing.
    const int blockCount = processor.getBlockCounter();
    if (processor.isTransportPlaying() && blockCount != priorBlockCount && ! processor.isGraphBypassed())
    {
        priorBlockCount = blockCount;
        if (! reelAnimation->isAnimating)
        {
            reelAnimation->setFramesPerSecond(50);
            reelAnimation->isAnimating = true;
        }
    }
    else if (reelAnimation->isAnimating)
    {
        reelAnimation->setFramesPerSecond(0);
        reelAnimation->isAnimating = false;
    }
}

//==============================================================================
void KissOfShameAudioProcessorEditor::paint(Graphics& g)
{
    if (era == UIEra::modern)
    {
        paintModernPanel(g);
        return;
    }

    g.fillAll(Colours::black);
}

void KissOfShameAudioProcessorEditor::paintModernPanel(Graphics& g)
{
    using namespace ModernTheme;

    auto full = content.getLocalBounds().toFloat();
    const float deckTop = showReels ? 322.0f : 0.0f;
    auto deck = full.withTop(deckTop);

    // weathered gunmetal faceplate
    ColourGradient steel(Colour(0xff2f2f34), deck.getCentreX(), deck.getY(),
                         Colour(0xff121215), deck.getCentreX(), deck.getBottom(), false);
    g.setGradientFill(steel);
    g.fillRect(deck);
    fillGrunge(g, deck, 1.0f);
    fillGrain(g, deck, 0.35f);

    // vignette: the plate falls away at the edges
    ColourGradient vig(Colours::transparentBlack, deck.getCentreX(), deck.getCentreY(),
                       Colours::black.withAlpha(0.45f), deck.getX(), deck.getY(), true);
    vig.addColour(0.66, Colours::transparentBlack);
    g.setGradientFill(vig);
    g.fillRect(deck);

    // machined seam under the reel bay
    g.setColour(Colours::black.withAlpha(0.7f));
    g.fillRect(deck.getX(), deckTop, deck.getWidth(), 2.0f);
    g.setColour(Colours::white.withAlpha(0.10f));
    g.fillRect(deck.getX(), deckTop + 2.0f, deck.getWidth(), 1.0f);

    // distressed chrome wordmark
    drawWordmark(g, { 372.0f, deckTop + 16.0f, 216.0f, 58.0f });

    // corner screws
    drawScrew(g, { deck.getX() + 13.0f, deckTop + 14.0f }, 3.2f, 0.7f);
    drawScrew(g, { deck.getRight() - 13.0f, deckTop + 14.0f }, 3.2f, 2.1f);
    drawScrew(g, { deck.getX() + 13.0f, deck.getBottom() - 13.0f }, 3.2f, 1.4f);
    drawScrew(g, { deck.getRight() - 13.0f, deck.getBottom() - 13.0f }, 3.2f, 2.8f);

    // stenciled white captions, exactly where the shelved design put them
    auto stencil = [&g](juce::Rectangle<int> area, const String& text, float size,
                        Justification just = Justification::centred)
    {
        g.setFont(ModernTheme::labelFont(size));
        g.setColour(Colours::black.withAlpha(0.65f));
        g.drawText(text, area.translated(0, 1), just, false);
        g.setColour(Colours::white.withAlpha(0.86f));
        g.drawText(text, area, just, false);
    };

    auto under = [](const Component& c, int dy, int h) {
        return juce::Rectangle<int>(c.getX() - 20, c.getBottom() + dy, c.getWidth() + 40, h);
    };

    stencil(under(bypassButton, 0, 13), "BYPASS", 10.0f);
    stencil(under(ageKnob, 1, 13), "AGE", 10.5f);
    stencil(under(hissKnob, 1, 13), "HISS", 10.5f);
    stencil(under(blendKnob, 1, 13), "BLEND", 10.5f);
    stencil(under(inputSaturationKnob, -6, 15), "INPUT", 12.0f);
    stencil(under(outputKnob, -6, 15), "OUTPUT", 12.0f);

    // meter legends sit low-left of the glass, like the print on the plate
    stencil({ vuMeterL.getX() + 4, vuMeterL.getBottom() + 2, 40, 13 }, "IN", 10.5f, Justification::centredLeft);
    stencil({ vuMeterR.getX() + 4, vuMeterR.getBottom() + 2, 44, 13 }, "OUT", 10.5f, Justification::centredLeft);

    stencil({ printThroughButton.getX() - 20, printThroughButton.getBottom() + 2,
              printThroughButton.getWidth() + 40, 12 }, "PRINT", 9.0f);
    stencil({ printThroughButton.getX() - 20, printThroughButton.getBottom() + 13,
              printThroughButton.getWidth() + 40, 12 }, "THROUGH", 9.0f);
}

void KissOfShameAudioProcessorEditor::paintContentOverChildren(Graphics& g)
{
    // Extreme mode in the heritage era: the Shame knob runs hot. (The modern
    // knob paints its own heat.)
    if (era == UIEra::heritage && processor.isShameExtreme())
    {
        const auto knobBounds = shameKnob.getBounds().toFloat().expanded(10.0f);
        const auto centre = knobBounds.getCentre();

        ColourGradient glow(Colour::fromFloatRGBA(1.0f, 0.1f, 0.15f, 0.45f), centre.x, centre.y,
                            Colour::fromFloatRGBA(1.0f, 0.1f, 0.15f, 0.0f), centre.x, knobBounds.getY(), true);
        g.setGradientFill(glow);
        g.fillEllipse(knobBounds);
    }
}
