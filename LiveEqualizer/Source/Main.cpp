/*
  ==============================================================================
    Main.cpp  —  Application entry point

    Defines the JUCEApplication subclass that:
      1. Creates a fixed-size (600 × 250) DocumentWindow on startup.
      2. Embeds MainComponent as the sole piece of UI content.
      3. Enforces single-instance behaviour — a second launch brings the
         existing window to the front rather than opening a duplicate.

    The START_JUCE_APPLICATION macro at the bottom expands to the platform-
    appropriate entry point (main() on Windows, WinMain(), etc.) and hands
    control to JUCE's event loop.
  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"

// ==============================================================================
/**
 * @class  AudioEqualizerApp
 * @brief  Root JUCE application object — owns the main window and lifecycle.
 */
class AudioEqualizerApp final : public juce::JUCEApplication
{
public:
    AudioEqualizerApp() = default;

    // =========================================================================
    // JUCEApplication interface
    // =========================================================================

    const juce::String getApplicationName()    override { return "System Audio Equalizer"; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }

    /** Enforce singleton — a second .exe launch activates the running window. */
    bool moreThanOneInstanceAllowed() override { return false; }

    // -------------------------------------------------------------------------

    /** Called once the JUCE event loop is running. Creates the main window. */
    void initialise(const juce::String& /*commandLine*/) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    /** Releases the window (and through it, MainComponent) on exit. */
    void shutdown() override
    {
        mainWindow.reset();
    }

    /** Responds to Alt+F4 / OS close signal. */
    void systemRequestedQuit() override
    {
        quit();
    }

    /** Brings the existing window to front instead of opening a second copy. */
    void anotherInstanceStarted(const juce::String& /*commandLine*/) override
    {
        if (mainWindow != nullptr)
            mainWindow->toFront(true);
    }

    // =========================================================================
    // Inner class: MainWindow
    // =========================================================================

    /**
     * @class  MainWindow
     * @brief  Non-resizable DocumentWindow hosting MainComponent.
     *
     * The fixed 600 × 250 size preserves the three-column EQ layout.
     * Resizability is disabled because the component does not implement a
     * fluid layout — all positions are calculated from the fixed dimensions.
     */
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name,
                             juce::Colour(0xFF1E1E1E),   // dark background in title bar
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);

            // setContentOwned: DocumentWindow takes ownership of the component
            // and automatically matches window size to component's preferred size.
            setContentOwned(new MainComponent(), true);

            // Lock the window so the 600×250 EQ layout is always preserved.
            setResizable(false, false);

            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

// ==============================================================================
// Entry point — expands to platform-appropriate main() / WinMain()
// ==============================================================================
START_JUCE_APPLICATION(AudioEqualizerApp)
