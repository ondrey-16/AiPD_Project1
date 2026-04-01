#pragma once
#include "WAVDataAnalyzer.h"
#include "AudioTimePlotComponent.h"

class MainComponent  : public juce::Component, juce::MenuBarModel
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

	juce::StringArray getMenuBarNames() override;
	juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
	void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

private:
	struct LegendRecord
	{
		juce::Colour colour;
		juce::String name;
	};

	std::vector<LegendRecord> legendRecords =
	{
		{juce::Colours::darkred, "Audio"},
		{juce::Colours::green, "Volume"},
		{juce::Colours::darkturquoise, "STE"},
		{juce::Colours::orange, "ZCR"}
	};

	uint32_t sampleRate = 0;
	juce::AudioBuffer<float> audioData;
	std::unique_ptr<juce::MenuBarComponent> menuBar;
	std::unique_ptr<juce::FileChooser> fileChooser;
	juce::ApplicationCommandManager commandManager;
	juce::ToggleButton silenceButton;
	juce::ToggleButton sonorousButton;
	juce::ToggleButton speechMusicButton;
	AudioTimePlotComponent audioTimePlot;

	void drawFrameParamPlot(juce::Graphics& g, juce::Rectangle<int> plotArea, const std::vector<float>& params, juce::Colour color);
	void drawAudioPlot(juce::Graphics& g, juce::Rectangle<int> plotArea, const std::vector<float>& params, juce::Colour color);
	void drawDetection(juce::Graphics& g, juce::Rectangle<int> plotArea, const std::vector<bool>& silentFrames, juce::Colour color);
	void addParamsToFile(const juce::String& name, const std::vector<float>& data, juce::String& content);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
