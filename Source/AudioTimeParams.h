#pragma once
#include <vector>
#include <JuceHeader.h>

static class AudioTimeParams
{
public:
	/* Parametry ramkowe */
	static std::vector<float> getVolume(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getSTE(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getZCR(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getFundamentalFreqAutoCorr(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getFundamentalFreqAMDF(juce::AudioBuffer<float> audioData, int sampleRate);
	/* Detekcja ciszy/fragmentów dźwięcznych */
	static std::vector<bool> silenceDetection(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<bool> sonorousDetection(juce::AudioBuffer<float> audioData, int sampleRate);
	/* Parametry na poziomie klipu*/
	static std::vector<float> getVSTD(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getVDR(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getVU(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getLSTER(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getEntropy(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getZSTD(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getHZCRR(juce::AudioBuffer<float> audioData, int sampleRate);
	/*Detekcja mowy/muzyki*/
	static std::vector<int> musicSpeechDetection(juce::AudioBuffer<float> audioData, int sampleRate);
	static const int frameSize = 512;
private:
	static std::vector<float> getFundamentalFreq(juce::AudioBuffer<float> audioData, int sampleRate, float (*estimateFunction)(float, float), bool (*conditionFuncion)(float, float));
	static float autoCorrelation(float a, float b);
	static float AMDF(float a, float b);
	static bool autoCorrelationCondition(float a, float b);
	static bool AMDFCondition(float a, float b);
	static std::vector<float> getSegmentsEnergy(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getClipEnergy(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getNormalizedEnergies(juce::AudioBuffer<float> audioData, int sampleRate);
};