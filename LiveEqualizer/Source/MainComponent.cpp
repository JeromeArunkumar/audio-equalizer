/*
  ==============================================================================
    MainComponent.cpp  —  System-Wide Audio Equalizer Implementation

    Core IPC mechanism:
      writeEqualizerConfig() opens the Equalizer APO config file in truncate
      mode and writes a complete, valid filter chain. The std::ofstream is
      RAII-managed — no explicit close() call needed; the destructor flushes
      and releases the file handle when the ofstream goes out of scope.

    Every slider movement triggers one writeEqualizerConfig() call via a
    C++ lambda stored in juce::Slider::onValueChange.  The function also
    runs once at construction to establish a flat 0 dB baseline.
  ==============================================================================
*/

#include "MainComponent.h"

// ==============================================================================
// Constructor
// ==============================================================================

MainComponent::MainComponent()
{
    // ----- Bass — Low Shelf filter @ 200 Hz ----------------------------------
    setupSlider(bassSlider);
    styleSlider(bassSlider, juce::Colour(0xFF4FC3F7)); // Cyan-blue
    setupLabel(bassLabel, "Bass (200 Hz)");
    bassSlider.onValueChange = [this]() { writeEqualizerConfig(); };
    addAndMakeVisible(bassSlider);
    addAndMakeVisible(bassLabel);

    // ----- Mid — Peaking filter @ 1 kHz -------------------------------------
    setupSlider(midSlider);
    styleSlider(midSlider, juce::Colour(0xFF81C784)); // Soft green
    setupLabel(midLabel, "Mid (1 kHz)");
    midSlider.onValueChange = [this]() { writeEqualizerConfig(); };
    addAndMakeVisible(midSlider);
    addAndMakeVisible(midLabel);

    // ----- Treble — High Shelf filter @ 4 kHz --------------------------------
    setupSlider(trebleSlider);
    styleSlider(trebleSlider, juce::Colour(0xFFFFB74D)); // Amber
    setupLabel(trebleLabel, "Treble (4 kHz)");
    trebleSlider.onValueChange = [this]() { writeEqualizerConfig(); };
    addAndMakeVisible(trebleSlider);
    addAndMakeVisible(trebleLabel);

    // Write the flat baseline immediately so Equalizer APO starts at 0 dB
    // regardless of what any previous session may have left in the file.
    writeEqualizerConfig();

    // Fixed window size matching the three-column layout in resized()
    setSize(600, 250);
}

// ==============================================================================
// juce::Component overrides
// ==============================================================================

void MainComponent::paint(juce::Graphics& g)
{
    // Main background — dark slate
    g.fillAll(juce::Colour(0xFF1E1E1E));

    // Slightly lighter top strip (label area)
    g.setColour(juce::Colour(0xFF252525));
    g.fillRect(0, 0, getWidth(), 40);

    // Hairline column separators
    g.setColour(juce::Colour(0xFF2E2E2E));
    g.drawLine(200.0f, 0.0f, 200.0f, static_cast<float>(getHeight()), 1.0f);
    g.drawLine(400.0f, 0.0f, 400.0f, static_cast<float>(getHeight()), 1.0f);

    // Subtle branding — attribution line flush with the bottom of the top strip
    g.setColour(juce::Colour(0xFF484848));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    g.drawText("System EQ  —  Powered by Equalizer APO",
               0, 0, getWidth(), 38, juce::Justification::centredBottom, false);
}

void MainComponent::resized()
{
    // -------------------------------------------------------------------------
    // Layout constants
    //   Window:    600 × 250
    //   Top strip: 40 px  (labels)
    //   Knob area: 210 px (y = 40 … 250)
    //   Columns:   200 px each
    // -------------------------------------------------------------------------
    constexpr int colW       = 200;
    constexpr int labelY     = 7;
    constexpr int labelH     = 26;
    constexpr int sliderY    = 42;
    constexpr int sliderW    = 160;
    constexpr int xPad       = (colW - sliderW) / 2;  // 20 px margin per side

    const int sliderH = getHeight() - sliderY - 6;    // ~202 px

    // Column 1 — Bass
    bassLabel .setBounds(0,         labelY, colW,    labelH);
    bassSlider.setBounds(xPad,      sliderY, sliderW, sliderH);

    // Column 2 — Mid
    midLabel .setBounds(colW,           labelY, colW,    labelH);
    midSlider.setBounds(colW + xPad,    sliderY, sliderW, sliderH);

    // Column 3 — Treble
    trebleLabel .setBounds(2 * colW,         labelY, colW,    labelH);
    trebleSlider.setBounds(2 * colW + xPad,  sliderY, sliderW, sliderH);
}

// ==============================================================================
// Private: IPC file writer
// ==============================================================================

void MainComponent::writeEqualizerConfig()
{
    // Attempt the production Equalizer APO path first.
    // Open in truncate mode: always a full overwrite — never append.
    std::ofstream cfg(kEqualizerApoConfigPath, std::ios::out | std::ios::trunc);

    if (!cfg.is_open())
    {
        // Fallback: write a local file for development / testing without APO.
        cfg.open(kFallbackConfigPath, std::ios::out | std::ios::trunc);

        if (!cfg.is_open())
        {
            DBG("[EQ] writeEqualizerConfig: could not open any output path");
            return;
        }

        DBG("[EQ] APO path unavailable — writing to local fallback: juce_eq.txt");
    }

    // Format a double to one decimal place (e.g. -6.0, +3.5, 0.0).
    // std::fixed prevents scientific notation; setprecision(1) gives one decimal.
    auto fmtGain = [](double v) -> std::string
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << v;
        return oss.str();
    };

    // Write the Equalizer APO filter chain.
    // Each "Filter N: ON <TYPE> Fc <FREQ> Hz Gain <dB> dB Q <Q>" line maps to
    // one second-order IIR (Biquad) section that Equalizer APO computes and
    // applies to every audio sample in the Windows audio pipeline.
    cfg << "Preamp: 0 dB\n"

        // Filter 1 — Low Shelf (LSC): attenuates/boosts everything below 200 Hz.
        // Q = 0.707 → Butterworth maximally-flat response at the shelf transition.
        << "Filter 1: ON LSC Fc 200 Hz Gain "
        << fmtGain(bassSlider.getValue()) << " dB Q 0.707\n"

        // Filter 2 — Peaking (PK): symmetric bell curve centred at 1 kHz.
        // Q = 1.0 → bandwidth of approximately one octave (500 Hz … 2 kHz).
        << "Filter 2: ON PK Fc 1000 Hz Gain "
        << fmtGain(midSlider.getValue()) << " dB Q 1.0\n"

        // Filter 3 — High Shelf (HSC): attenuates/boosts everything above 4 kHz.
        // Q = 0.707 → Butterworth shelf (matches symmetry of Filter 1).
        << "Filter 3: ON HSC Fc 4000 Hz Gain "
        << fmtGain(trebleSlider.getValue()) << " dB Q 0.707\n";

    // cfg destructor runs here → stream is flushed and the OS file handle
    // is closed automatically.  No explicit cfg.close() needed (RAII).
}

// ==============================================================================
// Private: UI helpers
// ==============================================================================

void MainComponent::setupSlider(juce::Slider& slider)
{
    // Rotary knob with circular drag — mouse tracks the angle around the knob centre.
    slider.setSliderStyle(juce::Slider::Rotary);

    // Text box below the knob: 70 × 20 px, editable so users can type exact values.
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);

    // EQ range: ±30 dB in 0.1 dB steps, initialised flat (0 dB).
    slider.setRange(-30.0, +30.0, 0.1);
    slider.setValue(0.0, juce::dontSendNotification);
    slider.setTextValueSuffix(" dB");

    // Double-click resets the knob to 0 dB (muscle-memory-friendly UX).
    slider.setDoubleClickReturnValue(true, 0.0);
}

void MainComponent::styleSlider(juce::Slider& slider, juce::Colour accent)
{
    // Coloured arc filling the "active" portion of the knob ring
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);

    // Unlit background arc
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF3A3A3A));

    // Indicator line/thumb
    slider.setColour(juce::Slider::thumbColourId, accent.brighter(0.25f));

    // Text box dark-theme colours
    slider.setColour(juce::Slider::textBoxTextColourId,       juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF2A2A2A));
    slider.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colour(0xFF484848));
    slider.setColour(juce::Slider::textBoxHighlightColourId,  accent.withAlpha(0.45f));
}

void MainComponent::setupLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);

    // JUCE 8 FontOptions API: explicit builder replaces deprecated Font(height, flags).
    label.setFont(juce::Font(juce::FontOptions{}.withHeight(13.5f).withStyle("Bold")));

    label.setColour(juce::Label::textColourId, juce::Colour(0xFFDDDDDD));
    label.setJustificationType(juce::Justification::centred);
}
