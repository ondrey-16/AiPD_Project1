#include "WAVDataAnalyzer.h"

juce::AudioBuffer<float> WAVDataAnalyzer::parseWAVFile(const juce::File& wavFile)
{
	audioData.clear();
	audioData.setSize(0, 0);
	byteAudioData.clear();
	if (wavFile.exists())
	{
		juce::FileInputStream stream(wavFile);

		if (stream.openedOk())
		{
			stream.read(riffChunkID, 4);
			riffChunkID[4] = '\0';
			stream.read(&riffChunkSize, sizeof(riffChunkSize));
			changeEndianness2Host32(&riffChunkSize, BIG_LITTLE_ENDIAN::LITTLE);
			stream.read(waveFormat, 4);
			waveFormat[4] = '\0';
			if (strcmp(riffChunkID, "RIFF") != 0 || strcmp(waveFormat, "WAVE") != 0)
			{
				std::cerr << "Not a valid WAV file." << std::endl;
				return audioData;
			}
			stream.read(subchunk1ID, 4);
			subchunk1ID[4] = '\0';
			if (strcmp(subchunk1ID, "fmt ") != 0)
			{
				std::cerr << "Unsupported WAV file format." << std::endl;
				return audioData;
			}
			stream.read(&subchunk1Size, sizeof(subchunk1Size));
			changeEndianness2Host32(&subchunk1Size, BIG_LITTLE_ENDIAN::LITTLE);
			stream.read(&audioFormat, sizeof(audioFormat));
			changeEndianness2Host16(&audioFormat, BIG_LITTLE_ENDIAN::LITTLE);
			stream.read(&numChannels, sizeof(numChannels));
			changeEndianness2Host16(&numChannels, BIG_LITTLE_ENDIAN::LITTLE);
			stream.read(&sampleRate, sizeof(sampleRate));
			changeEndianness2Host32(&sampleRate, BIG_LITTLE_ENDIAN::LITTLE);
			stream.read(&byteRate, sizeof(byteRate));
			changeEndianness2Host32(&byteRate, BIG_LITTLE_ENDIAN::LITTLE);
			stream.read(&blockAlign, sizeof(blockAlign));
			changeEndianness2Host16(&blockAlign, BIG_LITTLE_ENDIAN::LITTLE);
			stream.read(&bitsPerSample, sizeof(bitsPerSample));
			changeEndianness2Host16(&bitsPerSample, BIG_LITTLE_ENDIAN::LITTLE);

			if (subchunk1Size > 16)
			{
				stream.skipNextBytes(subchunk1Size - 16);
			}
			while (!stream.isExhausted())
			{
				stream.read(subchunk2ID, 4);
				subchunk2ID[4] = '\0';
				stream.read(&dataSize, sizeof(dataSize));
				changeEndianness2Host32(&dataSize, BIG_LITTLE_ENDIAN::LITTLE);

				if (strcmp(subchunk2ID, "data") == 0)
				{
					break;
				}

				stream.skipNextBytes(dataSize);
			}

			byteAudioData.resize(dataSize);
			stream.read(byteAudioData.data(), dataSize);
			if (hostEndianness == BIG)
			{
				if (bitsPerSample == 16)
				{
					for (size_t i = 0; i < byteAudioData.size(); i += 2)
					{
						std::swap(byteAudioData[i], byteAudioData[i + 1]);
					}
				}
				else if (bitsPerSample == 32)
				{
					for (size_t i = 0; i < byteAudioData.size(); i += 4)
					{
						std::swap(byteAudioData[i], byteAudioData[i + 3]);
						std::swap(byteAudioData[i + 1], byteAudioData[i + 2]);
					}
				}
			}

			if (audioFormat == 3 && bitsPerSample == 32)
			{
				convertAudioDataFloat();
			}
			else if (audioFormat == 1)
			{
				if (bitsPerSample == 16)
				{
					convertAudioData16();
				}
				else
				{
					convertAudioData32();
				}
			}
			else
			{
				audioData.clear();
				audioData.setSize(0, 0);
				byteAudioData.clear();
			}
		}
	}
	return audioData;
}

uint32_t WAVDataAnalyzer::getSampleRate() const
{
	return sampleRate;
}

void WAVDataAnalyzer::changeEndianness2Host16(uint16_t* data, BIG_LITTLE_ENDIAN dataEndianness)
{
	if (hostEndianness != dataEndianness)
	{
		*data = (*data >> 8) | (*data << 8);
	}
}

void WAVDataAnalyzer::changeEndianness2Host32(uint32_t* data, BIG_LITTLE_ENDIAN dataEndianness)
{
	if (hostEndianness != dataEndianness)
	{
		*data = (*data >> 24) | ((*data << 8) & 0x00FF0000) | ((*data >> 8) & 0x0000FF00) | (*data << 24);
	}
}

void WAVDataAnalyzer::convertAudioData16()
{
	int sampleSize = bitsPerSample / 8;
	audioData.setSize(numChannels, dataSize / sampleSize / numChannels);

	for (int ch = 0; ch < numChannels; ch++)
	{
		float* dst = audioData.getWritePointer(ch);
		for (int i = 0; i < audioData.getNumSamples(); i++)
		{
			int off = i * sampleSize * numChannels + ch * sampleSize;
			int16_t sample = 0;
			std::memcpy(&sample, byteAudioData.data() + off, sampleSize);
			dst[i] = (float)sample / 32768.f;
		}
	}
}

void WAVDataAnalyzer::convertAudioData32()
{
	int sampleSize = bitsPerSample / 8;
	audioData.setSize(numChannels, dataSize / sampleSize / numChannels);

	for (int ch = 0; ch < numChannels; ch++)
	{
		float* dst = audioData.getWritePointer(ch);
		for (int i = 0; i < audioData.getNumSamples(); i++)
		{
			int off = i * sampleSize * numChannels + ch * sampleSize;
			int32_t sample = 0;
			std::memcpy(&sample, byteAudioData.data() + off, sampleSize);
			dst[i] = (float)sample / 2147483648.0f;
		}
	}
}

void WAVDataAnalyzer::convertAudioDataFloat()
{
	int sampleSize = bitsPerSample / 8;
	audioData.setSize(numChannels, dataSize / sampleSize / numChannels);

	for (int ch = 0; ch < numChannels; ch++)
	{
		float* dst = audioData.getWritePointer(ch);
		for (int i = 0; i < audioData.getNumSamples(); i++)
		{
			int off = i * sampleSize * numChannels + ch * sampleSize;
			float sample = 0;
			std::memcpy(&sample, byteAudioData.data() + off, sampleSize);
			dst[i] = sample;
		}
	}
}
