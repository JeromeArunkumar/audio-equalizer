/*
  ==============================================================================
    MainComponent.h  —  System-Wide Audio Equalizer (JUCE + Equalizer APO)

    This component is the "Frontend" in a Hybrid IPC Architecture:
      [JUCE GUI]  --(file write)-->  [juce_eq.txt]  --(Include:)-->  [Equalizer APO]

    The application does NOT process audio buffers. It acts as a pure GUI
    controller that translates rotary-knob positions into Equalizer APO
    configuration syntax and persists them to disk whenever any slider moves.
    Equalizer APO — a Windows kernel-level audio pipeline service — monitors
    that file and recalculates its Biquad IIR coefficients in real-time.

    IPC mechanism: shared text file (std::ofstream, full overwrite on each change)
    DSP work:      delegated entirely to Equalizer APO
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Standard library headers used by writeEqualizerConfig()
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

// ==============================================================================
/**
 * @class  MainComponent
 * @brief  Primary GUI component — three EQ band knobs + file-based IPC writer.
 *
 * Presents Bass (200 Hz), Mid (1 kHz), and Treble (4 kHz) rotary sliders in a
 * fixed 600 × 250 dark-themed window. Every value change immediately rewrites
 * the Equalizer APO config file so that the gain adjustment is audible within
 * one APO processing block (~10 ms at 48 kHz / 512 samples).
 */
class MainComponent final : public juce::Component
{
public:
    // =========================================================================
    // Construction / Destruction
    // =========================================================================

    /**
     * Initialises sliders and labels, attaches onValueChange lambdas, then
     * calls writeEqualizerConfig() once to establish a flat 0 dB baseline on
     * startup — preventing stale settings from a prior session.
     */
    MainComponent();

    /** Default destructor. JUCE RAII handles child-component teardown. */
    ~MainComponent() override = default;

    // =========================================================================
    // juce::Component interface
    // =========================================================================

    /** Paints the dark-slate background and subtle column separators. */
    void paint(juce::Graphics& g) override;

    /** Lays out three label/slider pairs across the 600 × 250 window. */
    void resized() override;

private:
    // =========================================================================
    // IPC Core — Equalizer APO Config Writer
    // =========================================================================

    /**
     * @brief Atomically rewrites the Equalizer APO config file with current dB values.
     *
     * Opens kEqualizerApoConfigPath in truncate mode (full overwrite) and writes
     * a valid Equalizer APO filter chain using the three slider states.
     * Falls back to a local "juce_eq.txt" in the working directory when the
     * APO installation path is unavailable (useful for development).
     *
     * Output format:
     *   Preamp: 0 dB
     *   Filter 1: ON LSC  Fc  200 Hz Gain {bass}   dB Q 0.707
     *   Filter 2: ON PK   Fc 1000 Hz Gain {mid}    dB Q 1.0
     *   Filter 3: ON HSC  Fc 4000 Hz Gain {treble} dB Q 0.707
     */
    void writeEqualizerConfig();

    // =========================================================================
    // UI Helpers
    // =========================================================================

    /** Applies the shared range, step, text-box, and double-click-reset settings
     *  to a slider. All three EQ bands share identical control characteristics. */
    void setupSlider(juce::Slider& slider);

    /** Colours a slider with the given accent colour on the dark-theme palette. */
    void styleSlider(juce::Slider& slider, juce::Colour accent);

    /** Sets a label's font, colour, alignment, and display text. */
    void setupLabel(juce::Label& label, const juce::String& text);

    // =========================================================================
    // Constants
    // =========================================================================

    /** Production path: the file Equalizer APO reads via "Include: juce_eq.txt"
     *  in its master config.txt.  Ensure setup_link.ps1 has been run once. */
    static constexpr const char* kEqualizerApoConfigPath =
        "C:\\Program Files\\EqualizerAPO\\config\\juce_eq.txt";

    /** Development fallback written when the APO path cannot be opened. */
    static constexpr const char* kFallbackConfigPath = "juce_eq.txt";

    // =========================================================================
    // UI Components
    // =========================================================================

    juce::Slider bassSlider;    ///< Low Shelf  @ 200 Hz  (cyan accent)
    juce::Slider midSlider;     ///< Peaking EQ @ 1 kHz   (green accent)
    juce::Slider trebleSlider;  ///< High Shelf @ 4 kHz   (amber accent)

    juce::Label bassLabel;
    juce::Label midLabel;
    juce::Label trebleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
