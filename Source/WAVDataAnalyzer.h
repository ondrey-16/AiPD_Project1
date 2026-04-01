#pragma once

#include <JuceHeader.h>
#include <vector>

enum BIG_LITTLE_ENDIAN
{
	BIG,
	LITTLE
};

class WAVDataAnalyzer
{
public:
	WAVDataAnalyzer()
	{
		unsigned int endianTestNum = 1;
		char* endianTestBytes = reinterpret_cast<char*>(&endianTestNum);
		if (endianTestBytes[0] == 1)
			hostEndianness = LITTLE;
		else
			hostEndianness = BIG;
	}
	~WAVDataAnalyzer()
	{
		audioData.clear();
		byteAudioData.clear();
	}
	juce::AudioBuffer<float> parseWAVFile(const juce::File& wavFile);
	uint32_t getSampleRate() const;

private:
	juce::AudioBuffer<float> audioData;
	BIG_LITTLE_ENDIAN hostEndianness;
	char riffChunkID[5];
	uint32_t riffChunkSize = 0;
	char waveFormat[5];
	char subchunk1ID[5];
	uint32_t subchunk1Size = 0;
	uint16_t audioFormat = 0;
	uint16_t numChannels = 0;
	uint32_t sampleRate = 0;
	uint32_t byteRate = 0;
	uint16_t blockAlign = 0;
	uint16_t bitsPerSample = 0;
	char subchunk2ID[5];
	uint32_t dataSize = 0;
	std::vector<char> byteAudioData;

	void changeEndianness2Host16(uint16_t* data, BIG_LITTLE_ENDIAN dataEndianness);
	void changeEndianness2Host32(uint32_t* data, BIG_LITTLE_ENDIAN dataEndianness);
	void convertAudioData16();
	void convertAudioData32();
	void convertAudioDataFloat();
};