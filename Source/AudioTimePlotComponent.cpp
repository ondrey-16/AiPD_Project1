#include "AudioTimePlotComponent.h"
#include "AudioTimeParams.h"

AudioTimePlotComponent::AudioTimePlotComponent()
{
	this->openGLContext.setRenderer(this);
	this->openGLContext.setContinuousRepainting(true);
	this->openGLContext.setComponentPaintingEnabled(false);
	this->openGLContext.attachTo(*this);
	this->sampleRate = 0;
}

AudioTimePlotComponent::~AudioTimePlotComponent()
{
	this->openGLContext.detach();
}

void AudioTimePlotComponent::setAudioData(juce::AudioBuffer<float> audioData, uint32_t sampleRate)
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->audioData.makeCopyOf(audioData);
	this->sampleRate = sampleRate;

	this->timestampsX.clear();
	this->frameTimestampsX.clear();
	this->clipTimestampsX.clear();
	this->audioValsY.clear();
	this->volumesY.clear();
	this->STEsY.clear();
	this->ZCRsY.clear();
	this->autoCorrFrequences.clear();
	this->AMDFFrequences.clear();
	this->VSTDsY.clear();
	this->VDRsY.clear();
	this->VUsY.clear();
	this->LSTERsY.clear();
	this->EntropiesY.clear();
	this->ZSTDsY.clear();
	this->HZCRRsY.clear();
	this->speechMusicDetectedClips.clear();
	this->detectedFrames.clear();

	if (!audioData.getNumSamples() || !sampleRate)
	{
		return;
	}

	const auto* samples = audioData.getReadPointer(0);
	const int numSamples = audioData.getNumSamples();

	this->timestampsX.resize(numSamples);
	this->audioValsY.resize(numSamples);

	for (size_t i = 0; i < numSamples; i++)
	{
		this->timestampsX[i] = (float)i / (float)sampleRate;
		this->audioValsY[i] = samples[i];
	}

	this->volumesY = audioFrameParamsAnalyzer.getVolume(audioData, sampleRate);
	this->STEsY = audioFrameParamsAnalyzer.getSTE(audioData, sampleRate);
	this->ZCRsY = audioFrameParamsAnalyzer.getZCR(audioData, sampleRate);

	float frameTimestamp = (float)audioFrameParamsAnalyzer.frameSize / (float)(sampleRate);
	float val = frameTimestamp / 2.0f;

	this->frameTimestampsX.resize(this->volumesY.size());
	for (size_t i = 0; i < this->volumesY.size(); i++)
	{
		this->frameTimestampsX[i] = val;
		val += frameTimestamp;
	}

	this->VSTDsY = audioFrameParamsAnalyzer.getVSTD(audioData, sampleRate);
	this->VDRsY = audioFrameParamsAnalyzer.getVDR(audioData, sampleRate);
	this->VUsY = audioFrameParamsAnalyzer.getVU(audioData, sampleRate);
	this->LSTERsY = audioFrameParamsAnalyzer.getLSTER(audioData, sampleRate);
	this->EntropiesY = audioFrameParamsAnalyzer.getEntropy(audioData, sampleRate);
	this->ZSTDsY = audioFrameParamsAnalyzer.getZSTD(audioData, sampleRate);
	this->HZCRRsY = audioFrameParamsAnalyzer.getHZCRR(audioData, sampleRate);

	int framesPerClip = sampleRate / audioFrameParamsAnalyzer.frameSize;
	float clipTimestamp = (float)(framesPerClip * audioFrameParamsAnalyzer.frameSize) / (float)sampleRate;
	val = clipTimestamp / 2.0f;
	this->clipTimestampsX.resize(this->VSTDsY.size());
	for (size_t i = 0; i < this->VSTDsY.size(); i++)
	{
		this->clipTimestampsX[i] = val;
		val += clipTimestamp;
	}

	if (silentFramesChecked && this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->detectedFrames = this->audioFrameParamsAnalyzer.silenceDetection(this->audioData, this->sampleRate);
	}
	else if (sonorousFramesChecked && this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->detectedFrames = this->audioFrameParamsAnalyzer.sonorousDetection(this->audioData, this->sampleRate);
		this->autoCorrFrequences = this->audioFrameParamsAnalyzer.getFundamentalFreqAutoCorr(this->audioData, this->sampleRate);
		this->AMDFFrequences = this->audioFrameParamsAnalyzer.getFundamentalFreqAMDF(this->audioData, this->sampleRate);
	}
	else if (musicSpeechClipsChecked && this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->speechMusicDetectedClips = this->audioFrameParamsAnalyzer.musicSpeechDetection(this->audioData, this->sampleRate);
	}
}

void AudioTimePlotComponent::showSilentFrames()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->silentFramesChecked = true;
	this->sonorousFramesChecked = false;
	this->musicSpeechClipsChecked = false;
	if (this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->detectedFrames = this->audioFrameParamsAnalyzer.silenceDetection(this->audioData, this->sampleRate);
	}
}

void AudioTimePlotComponent::showSonorousFrames()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->sonorousFramesChecked = true;
	this->silentFramesChecked = false;
	this->musicSpeechClipsChecked = false;
	if (this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->detectedFrames = this->audioFrameParamsAnalyzer.sonorousDetection(this->audioData, this->sampleRate);
		this->autoCorrFrequences = this->audioFrameParamsAnalyzer.getFundamentalFreqAutoCorr(this->audioData, this->sampleRate);
		this->AMDFFrequences = this->audioFrameParamsAnalyzer.getFundamentalFreqAMDF(this->audioData, this->sampleRate);
	}
}

void AudioTimePlotComponent::showMusicSpeechClips()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->musicSpeechClipsChecked = true;
	this->silentFramesChecked = false;
	this->sonorousFramesChecked = false;
	if (this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->speechMusicDetectedClips = this->audioFrameParamsAnalyzer.musicSpeechDetection(this->audioData, this->sampleRate);
	}
}

void AudioTimePlotComponent::paint(juce::Graphics& g)
{
}

void AudioTimePlotComponent::resized()
{}

void AudioTimePlotComponent::mouseMove(const juce::MouseEvent & e)
{
	this->lastCursorPos = e.position;
}

void AudioTimePlotComponent::mouseDrag(const juce::MouseEvent& e)
{
	this->lastCursorPos = e.position;
	this->leftMouseDown = e.mods.isLeftButtonDown();
}

void AudioTimePlotComponent::mouseDown(const juce::MouseEvent& e)
{
	this->lastCursorPos = e.position;
	this->leftMouseDown = e.mods.isLeftButtonDown();
}

void AudioTimePlotComponent::mouseUp(const juce::MouseEvent & e)
{
	this->lastCursorPos = e.position;
	this->leftMouseDown = e.mods.isLeftButtonDown();
}

void AudioTimePlotComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& msd)
{
	this->lastCursorPos = e.position;
	this->wheelDelta = msd.deltaY;
}

void AudioTimePlotComponent::renderPlot()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)getWidth(), (float)getHeight()), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

	ImGui::Begin("PlotHost", nullptr, flags);

	if (!this->timestampsX.empty() && !this->audioValsY.empty())
	{
		float maxWidth = ImGui::GetContentRegionAvail().x;
		float maxHeight = ImGui::GetContentRegionAvail().y;

		if (ImPlot::BeginPlot("Audio graph + frame parameters", ImVec2(maxWidth, (this->sonorousFramesChecked) ? maxHeight * 0.33f : maxHeight * 0.5f)))
		{
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

			ImPlot::SetupAxes("Time [s]", "Amplitude/Value");

			if (this->silentFramesChecked)
			{
				this->fillDetectedFrames(IM_COL32(0, 200, 0, 60));
			}
			else if (this->sonorousFramesChecked)
			{
				this->fillDetectedFrames(IM_COL32(0, 0, 200, 60));
			}
			else if (this->musicSpeechClipsChecked)
			{
				this->fillMusicSpeechClips();
			}

			ImPlot::PlotLine("Audio", this->timestampsX.data(), this->audioValsY.data(), (int)this->audioValsY.size());
			ImPlot::PlotLine("Volume", this->frameTimestampsX.data(), this->volumesY.data(), (int)this->volumesY.size());
			ImPlot::PlotLine("STE", this->frameTimestampsX.data(), this->STEsY.data(), (int)this->STEsY.size());
			ImPlot::PlotLine("ZCR", this->frameTimestampsX.data(), this->ZCRsY.data(), (int)this->ZCRsY.size());

			ImPlot::PopStyleVar(2);
			ImPlot::EndPlot();
		}
		if (this->sonorousFramesChecked && ImPlot::BeginPlot("Fundamental frequences", ImVec2(maxWidth, maxHeight * 0.33f)))
		{
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

			ImPlot::SetupAxes("Time [s]", "Frequency [Hz]");

			ImPlot::PlotLine("Frequences by Autocorrelation", this->frameTimestampsX.data(), this->autoCorrFrequences.data(), (int)this->autoCorrFrequences.size());
			ImPlot::PlotLine("Frequences by AMDF", this->frameTimestampsX.data(), this->AMDFFrequences.data(), (int)this->AMDFFrequences.size());

			ImPlot::PopStyleVar(2);
			ImPlot::EndPlot();
		}
		if (ImPlot::BeginPlot("Audio clip parameters", ImVec2(maxWidth, (this->sonorousFramesChecked) ? maxHeight * 0.33f : maxHeight * 0.5f)))
		{
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

			ImPlot::SetupAxes("Time [s]", "Amplitude/Value");

			if (clipTimestampsX.size() == 1)
			{
				ImPlot::PlotScatter("VSTD", this->clipTimestampsX.data(), this->VSTDsY.data(), (int)this->VSTDsY.size());
				ImPlot::PlotScatter("VDR", this->clipTimestampsX.data(), this->VDRsY.data(), (int)this->VDRsY.size());
				ImPlot::PlotScatter("VU", this->clipTimestampsX.data(), this->VUsY.data(), (int)this->VUsY.size());
				ImPlot::PlotScatter("LSTER", this->clipTimestampsX.data(), this->LSTERsY.data(), (int)this->LSTERsY.size());
				ImPlot::PlotScatter("Entropy", this->clipTimestampsX.data(), this->EntropiesY.data(), (int)this->EntropiesY.size());
				ImPlot::PlotScatter("ZSTD", this->clipTimestampsX.data(), this->ZSTDsY.data(), (int)this->ZSTDsY.size());
				ImPlot::PlotScatter("HZCRR", this->clipTimestampsX.data(), this->HZCRRsY.data(), (int)this->HZCRRsY.size());
			}
			else
			{
				ImPlot::PlotLine("VSTD", this->clipTimestampsX.data(), this->VSTDsY.data(), (int)this->VSTDsY.size());
				ImPlot::PlotLine("VDR", this->clipTimestampsX.data(), this->VDRsY.data(), (int)this->VDRsY.size());
				ImPlot::PlotLine("VU", this->clipTimestampsX.data(), this->VUsY.data(), (int)this->VUsY.size());
				ImPlot::PlotLine("LSTER", this->clipTimestampsX.data(), this->LSTERsY.data(), (int)this->LSTERsY.size());
				ImPlot::PlotLine("Entropy", this->clipTimestampsX.data(), this->EntropiesY.data(), (int)this->EntropiesY.size());
				ImPlot::PlotLine("ZSTD", this->clipTimestampsX.data(), this->ZSTDsY.data(), (int)this->ZSTDsY.size());
				ImPlot::PlotLine("HZCRR", this->clipTimestampsX.data(), this->HZCRRsY.data(), (int)this->HZCRRsY.size());
			}

			ImPlot::PopStyleVar(2);
			ImPlot::EndPlot();
		}
	}

	ImGui::End();
	ImGui::PopStyleVar(2);
}

void AudioTimePlotComponent::updateImGuiInput()
{
	ImGuiIO& io = ImGui::GetIO();

	float scale = (float)openGLContext.getRenderingScale();
	io.DisplayFramebufferScale = ImVec2(scale, scale);

	io.DisplaySize = ImVec2((float)getWidth(), (float)getHeight());
	io.MousePos = ImVec2((float)this->lastCursorPos.x, (float)this->lastCursorPos.y);
	io.MouseDown[0] = this->leftMouseDown;
	io.MouseWheel = this->wheelDelta;

	this->wheelDelta = 0.0f;
}

void AudioTimePlotComponent::fillDetectedFrames(ImU32 color)
{
	if (!this->detectedFrames.size())
	{
		return;
	}

	auto limits = ImPlot::GetPlotLimits();
	auto* drawList = ImPlot::GetPlotDrawList();

	const float frameTime = (float)this->audioFrameParamsAnalyzer.frameSize / (float)this->sampleRate;

	ImPlot::PushPlotClipRect();

	for (size_t i = 0; i < this->detectedFrames.size(); i++)
	{
		if (!this->detectedFrames[i])
		{
			continue;
		}

		int j = i;
		while (j < this->detectedFrames.size() && this->detectedFrames[j])
		{
			j++;
		}

		float x1 = i * frameTime;
		float x2 = j * frameTime;

		ImVec2 p1 = ImPlot::PlotToPixels(ImPlotPoint(x1, limits.Y.Max));
		ImVec2 p2 = ImPlot::PlotToPixels(ImPlotPoint(x2, limits.Y.Min));

		drawList->AddRectFilled(p1, p2, color);

		i = j - 1;
	}

	ImPlot::PopPlotClipRect();
}

void AudioTimePlotComponent::fillMusicSpeechClips()
{
	if (!this->speechMusicDetectedClips.size())
	{
		return;
	}

	auto limits = ImPlot::GetPlotLimits();
	auto* drawList = ImPlot::GetPlotDrawList();

	const float clipTime = 1.0f;

	ImPlot::PushPlotClipRect();

	for (size_t i = 0; i < this->speechMusicDetectedClips.size(); i++)
	{
		if (!this->speechMusicDetectedClips[i])
		{
			continue;
		}

		float x1 = i * clipTime;
		float x2 = (i + 1) * clipTime;

		ImVec2 p1 = ImPlot::PlotToPixels(ImPlotPoint(x1, limits.Y.Max));
		ImVec2 p2 = ImPlot::PlotToPixels(ImPlotPoint(x2, limits.Y.Min));

		if (this->speechMusicDetectedClips[i] == 1)
		{
			drawList->AddRectFilled(p1, p2, IM_COL32(255, 0, 0, 60));
		}
		else if (this->speechMusicDetectedClips[i] == 2)
		{
			drawList->AddRectFilled(p1, p2, IM_COL32(255, 0, 255, 60));
		}
	}

	ImPlot::PopPlotClipRect();
}

void AudioTimePlotComponent::newOpenGLContextCreated()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplOpenGL3_Init("#version 130");
}

void AudioTimePlotComponent::renderOpenGL()
{
	juce::OpenGLHelpers::clear(juce::Colours::black);
	updateImGuiInput();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();
	renderPlot();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void AudioTimePlotComponent::openGLContextClosing()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}
