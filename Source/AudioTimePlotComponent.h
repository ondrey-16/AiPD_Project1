#pragma once

#include <JuceHeader.h>
#include "AudioTimeParams.h"
#include "imgui.h"
#include "implot/implot.h"
#include "backends/imgui_impl_opengl3.h"
#include <mutex>

class AudioTimePlotComponent : public juce::Component, juce::OpenGLRenderer
{
public:
	AudioTimePlotComponent();
	~AudioTimePlotComponent() override;
	void setAudioData(juce::AudioBuffer<float> audioData, uint32_t sampleRate);
	void showSilentFrames();
	void showSonorousFrames();
	void showMusicSpeechClips();

	void paint(juce::Graphics& g) override;
	void resized() override;

	void mouseMove(const juce::MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;
	void mouseDown(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;
	void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& msd) override;

	std::vector<float> volumesY;
	std::vector<float> STEsY;
	std::vector<float> ZCRsY;
	std::vector<float> VSTDsY;
	std::vector<float> VDRsY;
	std::vector<float> VUsY;
	std::vector<float> LSTERsY;
	std::vector<float> EntropiesY;
	std::vector<float> ZSTDsY;
	std::vector<float> HZCRRsY;
	std::vector<float> autoCorrFrequences;
	std::vector<float> AMDFFrequences;

private:
	juce::OpenGLContext openGLContext;
	AudioTimeParams audioFrameParamsAnalyzer;
	juce::AudioBuffer<float> audioData;
	uint32_t sampleRate;
	bool silentFramesChecked = false;
	bool sonorousFramesChecked = false;
	bool musicSpeechClipsChecked = false;
	juce::Point<float> lastCursorPos{ 0.0f, 0.0f };
	bool leftMouseDown = false;
	float wheelDelta = 0.0f;

	std::vector<float> timestampsX;
	std::vector<float> frameTimestampsX;
	std::vector<float> clipTimestampsX;
	std::vector<float> audioValsY;
	std::vector<bool> detectedFrames;
	std::vector<int> speechMusicDetectedClips;

	std::mutex setDataMutex;

	void renderPlot();
	void updateImGuiInput();
	void fillDetectedFrames(ImU32 color);
	void fillMusicSpeechClips();
	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
};