/*
  ==============================================================================

    HSBFaders.h
    Created: 17 Mar 2026
    Author:  BlinderKitten

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class BKEngine;

class HSBFadersUI : public ShapeShifterContent
{
public:
    HSBFadersUI(const String& contentName);
    ~HSBFadersUI();

    static HSBFadersUI* create(const String& name) { return new HSBFadersUI(name); }
};

class HSBFaders :
    public juce::Component,
    public juce::Slider::Listener,
    public juce::Timer
{
public:
    juce_DeclareSingleton(HSBFaders, true);
    HSBFaders();
    ~HSBFaders() override;

    BKEngine* engine = nullptr;

    Slider hueSlider;
    Slider satSlider;
    Slider brightSlider;

    float currentHue = 0.0f;
    float currentSat = 1.0f;
    float currentBright = 1.0f;

    void paint(juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(Slider* slider) override;
    void timerCallback() override;
    void updateFromCommand();
    void sendColorToChannels();

private:
  class NoThumbSliderLookAndFeel : public LookAndFeel_V4
  {
  public:
    void drawLinearSliderThumb(Graphics&, int, int, int, int, float, float, float, const Slider::SliderStyle, Slider&) override {}
  } noThumbLookAndFeel;

    bool updatingFromCommand = false;
    int64 lastUserInteractionTime = 0;
    static constexpr int64 kInteractionCooldownMs = 350;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HSBFaders)
};
