#include "AudioTimeParams.h"
#include <JuceHeader.h>
#include <cmath>

std::vector<float> AudioTimeParams::getVolume(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto volumes = getSTE(audioData, sampleRate);
	for (size_t i = 0; i < volumes.size(); i++)
	{
		volumes[i] = std::sqrtf(volumes[i]);
	}

	return volumes;
}

std::vector<float> AudioTimeParams::getSTE(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto* samples = audioData.getReadPointer(0);
	int numSamples = audioData.getNumSamples();
	std::vector<float> STEs;

	for (int i = 0; i < numSamples; i += frameSize)
	{
		float volume = 0.0f;
		int end = (i + frameSize >= numSamples) ? numSamples - i : frameSize;
		for (int j = 0; j < end; j++)
		{
			if (i + j >= numSamples)
			{
				break;
			}
			float sample = samples[i + j];
			volume += sample * sample;
		}
		volume /= (float)end;
		STEs.push_back(volume);
	}

	return STEs;
}

std::vector<float> AudioTimeParams::getZCR(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto* samples = audioData.getReadPointer(0);
	int numSamples = audioData.getNumSamples();
	std::vector<float> ZCRs;

	for (int i = 0; i < numSamples; i += frameSize)
	{
		float ZCR = 0.0f;
		int end = (i + frameSize >= numSamples) ? numSamples - i : frameSize;
		for (int j = 1; j < end; j++)
		{
			if (i + j >= numSamples)
			{
				break;
			}
			int sign0 = 0, sign1 = 0;
			if (samples[i + j - 1] > 0)
			{
				sign0 = 1;
			}
			else if (samples[i + j - 1] < 0)
			{
				sign0 = -1;
			}
			if (samples[i + j] > 0)
			{
				sign1 = 1;
			}
			else if (samples[i + j] < 0)
			{
				sign1 = -1;
			}
			ZCR += std::abs(sign1 - sign0);
		}
		ZCR /= (float)end;
		ZCR /= 2.0f;
		ZCRs.push_back(ZCR);
	}

	return ZCRs;
}

std::vector<float> AudioTimeParams::getFundamentalFreqAutoCorr(juce::AudioBuffer<float> audioData, int sampleRate)
{
	return getFundamentalFreq(audioData, sampleRate, autoCorrelation, autoCorrelationCondition);
}

std::vector<float> AudioTimeParams::getFundamentalFreqAMDF(juce::AudioBuffer<float> audioData, int sampleRate)
{
	return getFundamentalFreq(audioData, sampleRate, AMDF, AMDFCondition);
}

std::vector<bool> AudioTimeParams::silenceDetection(juce::AudioBuffer<float> audioData, int sampleRate)
{
	const float volumeLimit = 0.005;
	const float ZCRLimit = 0.3;

	auto volumes = getVolume(audioData, sampleRate);
	auto ZCRs = getZCR(audioData, sampleRate);

	size_t N = std::min(volumes.size(), ZCRs.size());

	std::vector<bool> silentFrames;

	for (size_t i = 0; i < N; i++)
	{
		silentFrames.push_back((volumes[i] <= volumeLimit && ZCRs[i] <= ZCRLimit));
	}

	return silentFrames;
}

std::vector<bool> AudioTimeParams::sonorousDetection(juce::AudioBuffer<float> audioData, int sampleRate)
{
	const float STELimit = 0.005;
	const float ZCRLimit = 0.1;

	auto STEs = getSTE(audioData, sampleRate);
	auto ZCRs = getZCR(audioData, sampleRate);

	size_t N = std::min(STEs.size(), ZCRs.size());

	std::vector<bool> sonorousFrames;

	for (size_t i = 0; i < N; i++)
	{
		sonorousFrames.push_back((STEs[i] >= STELimit && ZCRs[i] <= ZCRLimit));
	}

	return sonorousFrames;
}

std::vector<float> AudioTimeParams::getVSTD(juce::AudioBuffer<float> audioData, int sampleRate)
{
	std::vector<float> VSTDs;
	auto volumes = getVolume(audioData, sampleRate);
	int framesPerClip = sampleRate / frameSize;

	for (int i = 0; i < volumes.size(); i += framesPerClip)
	{
		int end = (i + framesPerClip >= volumes.size()) ? volumes.size() - i: framesPerClip;

		if (end == 1)
		{
			VSTDs.push_back(0.0f);
			continue;
		}

		float maxVolume = 0.0f, avg = 0.0f;
		for (int j = 0; j < end; j++)
		{
			avg += volumes[i + j];
			if (volumes[i + j] > maxVolume)
			{
				maxVolume = volumes[i + j];
			}
		}
		avg /= (float)end;
		float stdVal = 0.0f;
		for (int j = 0; j < end; j++)
		{
			stdVal += std::pow((double)(volumes[i + j] - avg), 2.0);
		}
		stdVal /= (float)(end - 1);
		stdVal = std::sqrtf(stdVal);
		VSTDs.push_back((maxVolume > 0.0f) ? stdVal / maxVolume : 0.0f);
	}

	return VSTDs;
}

std::vector<float> AudioTimeParams::getVDR(juce::AudioBuffer<float> audioData, int sampleRate)
{
	std::vector<float> VDRs;
	auto volumes = getVolume(audioData, sampleRate);
	int framesPerClip = sampleRate / frameSize;

	for (int i = 0; i < volumes.size(); i += framesPerClip)
	{
		int end = (i + framesPerClip >= volumes.size()) ? volumes.size() - i : framesPerClip;
		float maxVolume = 0.0f, minVolume = 1.0f;
		for (int j = 0; j < end; j++)
		{
			if (volumes[i + j] > maxVolume)
			{
				maxVolume = volumes[i + j];
			}
			if (volumes[i + j] < minVolume)
			{
				minVolume = volumes[i + j];
			}
		}
		VDRs.push_back((maxVolume > 0.0f) ? (maxVolume - minVolume) / maxVolume : 0.0f);
	}

	return VDRs;
}

std::vector<float> AudioTimeParams::getVU(juce::AudioBuffer<float> audioData, int sampleRate)
{
	std::vector<float> VU;
	std::vector<float> extremes;
	auto volumes = getVolume(audioData, sampleRate);
	int framesPerClip = sampleRate / frameSize;

	for (int i = 0; i < volumes.size(); i += framesPerClip)
	{
		extremes.clear();
		int end = (i + framesPerClip >= volumes.size()) ? volumes.size() - i : framesPerClip;
		for (int j = 1; j < end - 1; j++)
		{
			if ((volumes[i + j] > volumes[i + j - 1] && volumes[i + j] > volumes[i + j + 1]) ||
				(volumes[i + j] < volumes[i + j - 1] && volumes[i + j] < volumes[i + j + 1]))
			{
				extremes.push_back(volumes[i + j]);
			}
		}
		if (extremes.empty())
		{
			VU.push_back(0.0f);
			continue;
		}
		
		float extremesDiffSum = 0.0f;
		for (int j = 0; j < extremes.size() - 1; j++)
		{
			extremesDiffSum += std::abs(extremes[j + 1] - extremes[j]);
		}
		VU.push_back(extremesDiffSum / (extremes.size() - 1));
	}

	return VU;
}

std::vector<float> AudioTimeParams::getLSTER(juce::AudioBuffer<float> audioData, int sampleRate)
{
	std::vector<float> LSTERs;
	auto STEs = getSTE(audioData, sampleRate);
	int framesPerClip = sampleRate / frameSize;

	for (int i = 0; i < STEs.size(); i += framesPerClip)
	{
		int end = (i + framesPerClip >= STEs.size()) ? STEs.size() - i : framesPerClip;
		float avg = 0.0f;
		for (int j = 0; j < end; j++)
		{
			avg += STEs[i + j];

		}
		avg /= (float)end;
		float LSTERVal = 0.0f;
		for (int j = 0; j < end; j++)
		{
			float sgn = 0.5f * avg - STEs[i + j];
			if (sgn < 0.0f)
			{
				sgn = -1.0f;
			}
			else if (sgn > 0.0f)
			{
				sgn = 1.0f;
			}
			LSTERVal += sgn + 1;
		}
		LSTERVal /= (float)(end * 2);
		LSTERs.push_back(LSTERVal);
	}

	return LSTERs;
}

std::vector<float> AudioTimeParams::getEntropy(juce::AudioBuffer<float> audioData, int sampleRate)
{
	std::vector<float> Entropies;
	auto normalizedEnergies = getNormalizedEnergies(audioData, sampleRate);
	const int K = 512;
	const int J = sampleRate / K;

	for (int i = 0; i < normalizedEnergies.size(); i += J)
	{
		float entropy = 0.0f;
		int clipEnd = std::min((int)(normalizedEnergies.size() - i), J);
		
		for (int j = 0; j < clipEnd; j++)
		{
			if (normalizedEnergies[i + j] > 0.0f)
			{
				entropy -= normalizedEnergies[i + j] * std::log2f(normalizedEnergies[i + j]);
			}
		}
		Entropies.push_back(entropy);
	}

	return Entropies;
}

std::vector<float> AudioTimeParams::getZSTD(juce::AudioBuffer<float> audioData, int sampleRate)
{
	std::vector<float> ZSTDs;
	auto ZCRs = getZCR(audioData, sampleRate);
	int framesPerClip = sampleRate / frameSize;

	for (int i = 0; i < ZCRs.size(); i += framesPerClip)
	{
		int end = (i + framesPerClip >= ZCRs.size()) ? ZCRs.size() - i : framesPerClip;

		if (end <= 1)
		{
			ZSTDs.push_back(0.0f);
			continue;
		}

		float avg = 0.0f;
		for (int j = 0; j < end; j++)
		{
			avg += ZCRs[i + j];
		}
		avg /= end;
		float ZSTD = 0.0f;
		for (int j = 0; j < end; j++)
		{
			ZSTD += std::pow((ZCRs[i + j] - avg), 2);
		}
		ZSTD /= (end - 1);
		ZSTDs.push_back(std::sqrtf(ZSTD));
	}

	return ZSTDs;
}

std::vector<float> AudioTimeParams::getHZCRR(juce::AudioBuffer<float> audioData, int sampleRate)
{
	std::vector<float> HZCRR;
	auto ZCRs = getZCR(audioData, sampleRate);
	int framesPerClip = sampleRate / frameSize;

	for (int i = 0; i < ZCRs.size(); i += framesPerClip)
	{
		int end = (i + framesPerClip >= ZCRs.size()) ? ZCRs.size() - i : framesPerClip;
		float avg = 0.0f;
		for (int j = 0; j < end; j++)
		{
			avg += ZCRs[i + j];
		}
		avg /= (float)end;
		float HZCRRVal = 0.0f;
		for (int j = 0; j < end; j++)
		{
			float sgn = ZCRs[i + j] - 1.5f * avg;
			if (sgn < 0.0f)
			{
				sgn = -1.0f;
			}
			else if (sgn > 0.0f)
			{
				sgn = 1.0f;
			}
			HZCRRVal += sgn + 1;
		}
		HZCRRVal /= (float)(end * 2);
		HZCRR.push_back(HZCRRVal);
	}

	return HZCRR;
}

std::vector<int> AudioTimeParams::musicSpeechDetection(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto ZSTDs = getZSTD(audioData, sampleRate);
	auto LSTERs = getLSTER(audioData, sampleRate);

	float speechZSTDLimit = 0.04f, speechLSTERLimit = 0.35f, musicZSTDLimit = 0.04f, musicLSTERLimit = 0.35f;

	size_t N = std::min(ZSTDs.size(), LSTERs.size());

	std::vector<int> detectionClips;

	for (size_t i = 0; i < N; i++)
	{
		int detection = 0; // brak detekcji
		if (ZSTDs[i] >= speechZSTDLimit && LSTERs[i] >= speechLSTERLimit)
		{
			detection = 1; // mowa
		}
		else if (ZSTDs[i] <= musicZSTDLimit && LSTERs[i] <= musicLSTERLimit)
		{
			detection = 2; // muzyka
		}

		detectionClips.push_back(detection);
	}

	return detectionClips;
}

std::vector<float> AudioTimeParams::getFundamentalFreq(juce::AudioBuffer<float> audioData, int sampleRate, float(*estimateFunction)(float, float), bool (*conditionFunction)(float, float))
{
	auto sonorousFrames = sonorousDetection(audioData, sampleRate);
	std::vector<float> fundamentalFreqs;
	auto* samples = audioData.getReadPointer(0);
	int numSamples = audioData.getNumSamples();

	int minFreq = 90;
	int maxFreq = 400;
	int minL = sampleRate / maxFreq;
	int maxL = sampleRate / minFreq;

	for (int i = 0; i < sonorousFrames.size(); i++)
	{
		if (!sonorousFrames[i])
		{
			fundamentalFreqs.push_back(0.0f);
			continue;
		}

		float ext = 0.0f;

		int end = std::min(i * frameSize + frameSize, numSamples);
		for (int j = 0; j < end - i * frameSize - minL; j++)
		{
			ext += estimateFunction(samples[i * frameSize + j + minL], samples[i * frameSize + j]);
		}
		ext /= end - i * frameSize - minL;

		int l = minL + 1;
		int bestL = minL;
		for (l; l <= maxL; l++)
		{
			float currVal = 0.0f;

			end = std::min(i * frameSize + frameSize, numSamples);
			for (int j = 0; j < end - i * frameSize - l; j++)
			{
				currVal += estimateFunction(samples[i * frameSize + j + l], samples[i * frameSize + j]);
			}
			currVal /= end - i * frameSize - l;

			if (conditionFunction(currVal, ext))
			{
				ext = currVal;
				bestL = l;
			}
		}

		if (bestL == minL || bestL > maxL)
		{
			fundamentalFreqs.push_back(0.0f);
			continue;
		}

		fundamentalFreqs.push_back((float)sampleRate / (float)bestL);
	}

	return fundamentalFreqs;
}

float AudioTimeParams::autoCorrelation(float a, float b)
{
	return a * b;
}

float AudioTimeParams::AMDF(float a, float b)
{
	return std::abs(a - b);
}

bool AudioTimeParams::autoCorrelationCondition(float a, float b)
{
	return a > b;
}

bool AudioTimeParams::AMDFCondition(float a, float b)
{
	return a < b;
}

std::vector<float> AudioTimeParams::getSegmentsEnergy(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto* samples = audioData.getReadPointer(0);
	int numSamples = audioData.getNumSamples();
	std::vector<float> STEs;
	int K = 512;

	for (int i = 0; i < numSamples; i += K)
	{
		float energy = 0.0f;
		int end = (i + K >= numSamples) ? numSamples - i : K;
		for (int j = 0; j < end; j++)
		{
			if (i + j >= numSamples)
			{
				break;
			}
			float sample = samples[i + j];
			energy += sample * sample;
		}
		STEs.push_back(energy);
	}

	return STEs;
}

std::vector<float> AudioTimeParams::getClipEnergy(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto* samples = audioData.getReadPointer(0);
	int numSamples = audioData.getNumSamples();
	int framesPerClip = sampleRate / frameSize;
	int samplesPerClip = framesPerClip * frameSize;
	std::vector<float> Energies;

	for (int i = 0; i < numSamples; i += samplesPerClip)
	{
		float energy = 0.0f;
		int end = (i + samplesPerClip >= numSamples) ? numSamples - i : samplesPerClip;
		for (int j = 0; j < end; j++)
		{
			float sample = samples[i + j];
			energy += sample * sample;
		}
		Energies.push_back(energy);
	}

	return Energies;
}

std::vector<float> AudioTimeParams::getNormalizedEnergies(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto clipEnergy = getClipEnergy(audioData, sampleRate);
	auto segmentsEnergy = getSegmentsEnergy(audioData, sampleRate);
	std::vector<float> normalizedEnergies;
	normalizedEnergies.resize(segmentsEnergy.size());
	int framesPerClip = sampleRate / frameSize;
	const int K = 512;
	const int J = framesPerClip * frameSize / K;

	for (int i = 0; i < clipEnergy.size(); i++)
	{
		int end = std::min((int)segmentsEnergy.size() - i * J, J);
		for (int j = 0; j < end; j++)
		{
			if (clipEnergy[i] <= 0.0f)
			{
				normalizedEnergies[i * J + j] = 0.0f;
			}
			else
			{
				normalizedEnergies[i * J + j] = segmentsEnergy[i * J + j] / clipEnergy[i];
			}
		}
	}

	return normalizedEnergies;
}
