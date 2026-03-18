/*
  ==============================================================================

    HSBFaders.cpp
    Created: 17 Mar 2026
    Author:  BlinderKitten

  ==============================================================================
*/

#include <JuceHeader.h>
#include "HSBFaders.h"
#include "UserInputManager.h"
#include "BKEngine.h"
#include "CIEColorPicker.h"
#include "Definitions/Command/Command.h"
#include "Definitions/ChannelFamily/ChannelType/ChannelType.h"

//==============================================================================
HSBFadersUI::HSBFadersUI(const String& contentName) :
    ShapeShifterContent(HSBFaders::getInstance(), contentName)
{
}

HSBFadersUI::~HSBFadersUI()
{
}

juce_ImplementSingleton(HSBFaders);

HSBFaders::HSBFaders()
{
    engine = dynamic_cast<BKEngine*>(Engine::mainEngine);

    for (auto* s : { &hueSlider, &satSlider, &brightSlider }) {
        s->setSliderStyle(Slider::LinearVertical);
        s->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
        s->setColour(Slider::backgroundColourId, Colours::transparentBlack);
        s->setColour(Slider::trackColourId, Colours::transparentBlack);
        s->setColour(Slider::thumbColourId, Colours::transparentBlack);
        s->setLookAndFeel(&noThumbLookAndFeel);
        s->addListener(this);
        s->setOpaque(false);
        addAndMakeVisible(s);
    }

    hueSlider.setRange(0.0, 360.0, 0.1);
    hueSlider.setValue(0.0, juce::dontSendNotification);
    satSlider.setRange(0.0, 100.0, 0.1);
    satSlider.setValue(100.0, juce::dontSendNotification);
    brightSlider.setRange(0.0, 100.0, 0.1);
    brightSlider.setValue(100.0, juce::dontSendNotification);

    startTimer(66);
}

HSBFaders::~HSBFaders()
{
    stopTimer();

    for (auto* s : { &hueSlider, &satSlider, &brightSlider }) {
        s->setLookAndFeel(nullptr);
    }
}

void HSBFaders::resized()
{
    auto area = getLocalBounds();
    int w = area.getWidth() / 3;

    hueSlider.setBounds(area.getX(), area.getY(), w, area.getHeight());
    satSlider.setBounds(area.getX() + w, area.getY(), w, area.getHeight());
    brightSlider.setBounds(area.getX() + 2 * w, area.getY(), area.getWidth() - 2 * w, area.getHeight());
}

void HSBFaders::paint(juce::Graphics& g)
{
    g.fillAll(Colour(40, 40, 40));

    // --- Hue fader background: full rainbow spectrum ---
    Rectangle<int> hBounds = hueSlider.getBounds();
    for (int y = 0; y < hBounds.getHeight(); y++) {
        float hue = 1.0f - (float)y / (float)hBounds.getHeight();
        g.setColour(Colour::fromHSV(hue, 1.0f, 1.0f, 1.0f));
        g.fillRect(hBounds.getX(), hBounds.getY() + y, hBounds.getWidth(), 1);
    }

    // --- Saturation fader background: desaturated at bottom → full hue at top ---
    Rectangle<int> sBounds = satSlider.getBounds();
    for (int y = 0; y < sBounds.getHeight(); y++) {
        float sat = 1.0f - (float)y / (float)sBounds.getHeight();
        g.setColour(Colour::fromHSV(currentHue, sat, 1.0f, 1.0f));
        g.fillRect(sBounds.getX(), sBounds.getY() + y, sBounds.getWidth(), 1);
    }

    // --- Brightness fader background: white at top → black at bottom ---
    Rectangle<int> bBounds = brightSlider.getBounds();
    for (int y = 0; y < bBounds.getHeight(); y++) {
        float bri = 1.0f - (float)y / (float)bBounds.getHeight();
        uint8 grey = (uint8)(bri * 255.0f);
        g.setColour(Colour(grey, grey, grey));
        g.fillRect(bBounds.getX(), bBounds.getY() + y, bBounds.getWidth(), 1);
    }

    // Draw value text at thumb position for each fader
    g.setFont(14.0f);
    auto drawValueAtThumb = [&](Slider& slider, const String& text)
    {
        Rectangle<int> bounds = slider.getBounds();
        double proportion = (slider.getValue() - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum());
        int thumbY = bounds.getY() + (int)((1.0 - proportion) * bounds.getHeight());
        int textH = 20;
        int textY = thumbY - textH / 2;
        textY = jmax(bounds.getY(), jmin(textY, bounds.getBottom() - textH));

        g.setColour(Colour(0, 0, 0).withAlpha(0.45f));
        g.fillRect(bounds.getX() + 2, textY, bounds.getWidth() - 4, textH);
        g.setColour(Colours::white);
        g.drawText(text, bounds.getX(), textY, bounds.getWidth(), textH, Justification::centred, false);
    };

    drawValueAtThumb(hueSlider, String(hueSlider.getValue(), 1));
    drawValueAtThumb(satSlider, String(satSlider.getValue(), 1));
    drawValueAtThumb(brightSlider, String(brightSlider.getValue(), 1));
}

void HSBFaders::sliderValueChanged(Slider* slider)
{
    if (updatingFromCommand) return;

    lastUserInteractionTime = Time::currentTimeMillis();

    currentHue = (float)hueSlider.getValue() / 360.0f;
    currentSat = (float)satSlider.getValue() / 100.0f;
    currentBright = (float)brightSlider.getValue() / 100.0f;

    sendColorToChannels();
    repaint();
}

void HSBFaders::sendColorToChannels()
{
    if (engine == nullptr)
        engine = dynamic_cast<BKEngine*>(Engine::mainEngine);
    if (engine == nullptr) return;

    if (CIEColorPicker::getInstanceWithoutCreating() != nullptr) {
        CIEColorPicker::getInstance()->clearCursorOverrideFromExternal();
    }

    Colour c = Colour::fromHSV(currentHue, currentSat, currentBright, 1.0f);
    float r = c.getFloatRed();
    float gr = c.getFloatGreen();
    float b = c.getFloatBlue();

    if (dynamic_cast<ChannelType*>(engine->CPRedChannel->targetContainer.get()) == nullptr) {
        engine->autoFillDefaultChannels();
    }

    if (engine->CPRedChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPRedChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, r);
    }
    if (engine->CPGreenChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPGreenChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, gr);
    }
    if (engine->CPBlueChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPBlueChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, b);
    }
    if (engine->CPCyanChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPCyanChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, 1.0f - r);
    }
    if (engine->CPMagentaChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPMagentaChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, 1.0f - gr);
    }
    if (engine->CPYellowChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPYellowChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, 1.0f - b);
    }
    if (engine->CPHueChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPHueChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, currentHue);
    }
    if (engine->CPSaturationChannel->stringValue() != "") {
        ChannelType* ch = dynamic_cast<ChannelType*>(engine->CPSaturationChannel->targetContainer.get());
        if (ch) UserInputManager::getInstance()->changeChannelValue(ch, currentSat);
    }
}

void HSBFaders::updateFromCommand()
{
    // Block timer updates while the user is actively interacting to prevent wiggling
    if (Time::currentTimeMillis() - lastUserInteractionTime < kInteractionCooldownMs)
        return;

    if (engine == nullptr)
        engine = dynamic_cast<BKEngine*>(Engine::mainEngine);
    if (engine == nullptr) return;

    Command* cmd = UserInputManager::getInstance()->targetCommand;
    if (cmd == nullptr) return;

    float hue = -1, sat = -1;
    float red = -1, green = -1, blue = -1;
    float cyan = -1, magenta = -1, yellow = -1;

    for (int i = 0; i < cmd->values.items.size(); i++) {
        CommandValue* cv = cmd->values.items[i];
        if (cv->channelType->getValue() == engine->CPHueChannel->getValue()) hue = cv->valueFrom->getValue();
        if (cv->channelType->getValue() == engine->CPSaturationChannel->getValue()) sat = cv->valueFrom->getValue();
        if (cv->channelType->getValue() == engine->CPRedChannel->getValue()) red = cv->valueFrom->getValue();
        if (cv->channelType->getValue() == engine->CPGreenChannel->getValue()) green = cv->valueFrom->getValue();
        if (cv->channelType->getValue() == engine->CPBlueChannel->getValue()) blue = cv->valueFrom->getValue();
        if (cv->channelType->getValue() == engine->CPCyanChannel->getValue()) cyan = cv->valueFrom->getValue();
        if (cv->channelType->getValue() == engine->CPMagentaChannel->getValue()) magenta = cv->valueFrom->getValue();
        if (cv->channelType->getValue() == engine->CPYellowChannel->getValue()) yellow = cv->valueFrom->getValue();
    }

    updatingFromCommand = true;

    if (hue >= 0 && sat >= 0) {
        // Native H/S channels: hue is explicit, safe to use directly
        currentHue = hue;
        currentSat = sat;
        hueSlider.setValue(hue * 360.0, dontSendNotification);
        satSlider.setValue(sat * 100.0, dontSendNotification);
    }
    else {
        float r = -1, gr = -1, b = -1;
        if (red >= 0 && green >= 0 && blue >= 0) {
            r = red; gr = green; b = blue;
        } else if (cyan >= 0 && magenta >= 0 && yellow >= 0) {
            r = 1.0f - cyan; gr = 1.0f - magenta; b = 1.0f - yellow;
        }

        if (r >= 0 && gr >= 0 && b >= 0) {
            Colour c((uint8)(r * 255), (uint8)(gr * 255), (uint8)(b * 255));
            float newSat    = c.getSaturation();
            float newBright = c.getBrightness();

            // Only update hue if saturation is meaningful — otherwise the conversion
            // is ill-defined (gray has no hue) and we preserve the last known value.
            if (newSat > 0.01f)
            {
                currentHue = c.getHue();
                hueSlider.setValue(currentHue * 360.0, dontSendNotification);
            }

            // Only update saturation if brightness is meaningful — a black colour
            // (brightness=0) also has undefined saturation, so preserve it.
            if (newBright > 0.002f)
            {
                currentSat = newSat;
                satSlider.setValue(currentSat * 100.0, dontSendNotification);
            }

            currentBright = newBright;
            brightSlider.setValue(currentBright * 100.0, dontSendNotification);
        }
    }

    updatingFromCommand = false;
    repaint();
}

void HSBFaders::timerCallback()
{
    if (engine == nullptr) engine = dynamic_cast<BKEngine*>(Engine::mainEngine);
    updateFromCommand();
}
