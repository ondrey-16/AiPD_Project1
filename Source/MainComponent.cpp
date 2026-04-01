#include "MainComponent.h"
#include "AudioTimeParams.h"

MainComponent::MainComponent()
{
	setApplicationCommandManagerToWatch(&commandManager);
	menuBar.reset(new juce::MenuBarComponent(this));
	addAndMakeVisible(menuBar.get());
    addAndMakeVisible(silenceButton);
    addAndMakeVisible(sonorousButton);
    addAndMakeVisible(speechMusicButton);
    addAndMakeVisible(audioTimePlot);
    silenceButton.setButtonText("Silence detection");
    sonorousButton.setButtonText("Sonorous frames detection");
    speechMusicButton.setButtonText("Music/Speech clips detection");
    silenceButton.setClickingTogglesState(true);
    sonorousButton.setClickingTogglesState(true);
    speechMusicButton.setClickingTogglesState(true);
    silenceButton.setRadioGroupId(1);
    sonorousButton.setRadioGroupId(1);
    speechMusicButton.setRadioGroupId(1);
    setSize (1200, 800);
	audioData.setSize(0, 0);
    silenceButton.onClick = [this] {
        audioTimePlot.showSilentFrames();
        repaint();
    };
    sonorousButton.onClick = [this] {
        audioTimePlot.showSonorousFrames();
        repaint();
    };
    speechMusicButton.onClick = [this] {
        audioTimePlot.showMusicSpeechClips();
        repaint();
    };
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
	if (menuBar != nullptr)
    {
        menuBar->setBounds(0, 0, getWidth(), 24);
    }
    silenceButton.setBounds(50, 50, 150, 30);
    sonorousButton.setBounds(50, 80, 200, 30);
    speechMusicButton.setBounds(50, 110, 220, 30);

    auto bounds = getLocalBounds();

    auto plotArea = bounds.reduced(50);
    plotArea.removeFromBottom(30);
    plotArea.removeFromTop(100);

    audioTimePlot.setBounds(plotArea);
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "New file:", "Save:"};
}

juce::PopupMenu MainComponent::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
	juce::PopupMenu menu;

	if (topLevelMenuIndex == 0)
    {
        menu.addItem(1, "Add .wav file");
    }
    else if (topLevelMenuIndex == 1)
    {
        menu.addItem(2, "Save parameters into file");
    }

	return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
	if (menuItemID == 1)
    {
		fileChooser = std::make_unique<juce::FileChooser>("Choose a .wav file", juce::File::getCurrentWorkingDirectory(), "*.wav");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& chooser)
            {
                auto result = chooser.getResult();
                if (result.existsAsFile())
                {
                    juce::Logger::writeToLog("Selected file: " + result.getFullPathName());
                    juce::Logger::writeToLog("Added .wav file");
				    WAVDataAnalyzer wavDataAnalyzer;
				    audioData = wavDataAnalyzer.parseWAVFile(result);
				    sampleRate = wavDataAnalyzer.getSampleRate();
                    audioTimePlot.setAudioData(audioData, sampleRate);
                    repaint();
                }
                else
                {
                    juce::Logger::writeToLog("No file selected");
                }
		    });
    }
    else if (menuItemID == 2)
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save parameters into txt file",
            juce::File::getCurrentWorkingDirectory().getChildFile("parameters.txt"),
            "*.txt");

        fileChooser->launchAsync(
            juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
            [this](const juce::FileChooser& chooser)
            {
                auto result = chooser.getResult();

                if (result != juce::File{})
                {
                    juce::Logger::writeToLog("Saving to file: " + result.getFullPathName());

                    if (result.getFileExtension() != ".txt")
                        result = result.withFileExtension(".txt");

                    juce::String content;

                    content << "|Frame parameters|\n\n";

                    addParamsToFile("Volume", audioTimePlot.volumesY, content);
                    addParamsToFile("STE", audioTimePlot.STEsY, content);
                    addParamsToFile("ZCR", audioTimePlot.ZCRsY, content);

                    content << "|Frequences|\n\n";

                    addParamsToFile("Autocorrelation frequences", audioTimePlot.autoCorrFrequences, content);
                    addParamsToFile("AMDF frequences", audioTimePlot.AMDFFrequences, content);

                    content << "|Clip parameters|\n\n";

                    addParamsToFile("VSTD", audioTimePlot.VSTDsY, content);
                    addParamsToFile("VDR", audioTimePlot.VDRsY, content);
                    addParamsToFile("VU", audioTimePlot.VUsY, content);
                    addParamsToFile("LSTER", audioTimePlot.LSTERsY, content);
                    addParamsToFile("Energy Entropy", audioTimePlot.EntropiesY, content);
                    addParamsToFile("ZSTD", audioTimePlot.ZSTDsY, content);
                    addParamsToFile("HZCRR", audioTimePlot.HZCRRsY, content);

                    if (result.replaceWithText(content))
                        juce::Logger::writeToLog("File saved successfully");
                    else
                        juce::Logger::writeToLog("Error while saving file");
                }
                else
                {
                    juce::Logger::writeToLog("Save cancelled");
                }
            });
    }
}

void MainComponent::drawFrameParamPlot(juce::Graphics& g, juce::Rectangle<int> plotArea, const std::vector<float>& params, juce::Colour color)
{
    g.setColour(color);

    int width = plotArea.getWidth();
    juce::Path path;

    const size_t N = params.size();
    for (size_t i = 0; i < N; i++)
    {
        int x1 = juce::jmap((float)i, 0.0f, (float)N, (float)plotArea.getX(), (float)plotArea.getRight());
        int x2 = juce::jmap((float)(i + 1), 0.0f, (float)N, (float)plotArea.getX(), (float)plotArea.getRight());
        int x = (x1 + x2) / 2;
        int y = juce::jmap((float)params[i], -1.0f, 1.0f, (float)plotArea.getBottom(), (float)plotArea.getY());

        if (i == 0)
        {
            path.startNewSubPath(x, y);
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    g.strokePath(path, juce::PathStrokeType(1.0f));
}

void MainComponent::drawAudioPlot(juce::Graphics& g, juce::Rectangle<int> plotArea, const std::vector<float>& params, juce::Colour color)
{
    g.setColour(color);

    int width = plotArea.getWidth();

    for (int x = 0; x < width; ++x)
    {
        int pos1 = x / (float)(width - 1) * (params.size() - 1);
        int pos2 = std::min((int)((x + 1) / (float)(width - 1) * (params.size() - 1)), (int)params.size() - 1);

        float minVal = params[pos1];
        float maxVal = params[pos1];
        for (int i = pos1 + 1; i <= pos2; i++)
        {
            minVal = std::min(minVal, params[i]);
            maxVal = std::max(maxVal, params[i]);
        }

        int yPos1 = juce::jmap(minVal, -1.0f, 1.0f, (float)plotArea.getBottom(), (float)plotArea.getY());
        int yPos2 = juce::jmap(maxVal, -1.0f, 1.0f, (float)plotArea.getBottom(), (float)plotArea.getY());
        int xPos = x + plotArea.getX();

        g.drawLine(xPos, yPos1, xPos, yPos2);
    }
}

void MainComponent::drawDetection(juce::Graphics& g, juce::Rectangle<int> plotArea, const std::vector<bool>& silentFrames, juce::Colour color)
{
    g.setColour(color);

    int width = plotArea.getWidth();

    const size_t N = silentFrames.size();
    for (size_t i = 0; i < N; i++)
    {
        if (!silentFrames[i])
        {
            continue;
        }

        int x1 = juce::jmap((float)i, 0.0f, (float)N, (float)plotArea.getX(), (float)plotArea.getRight());
        while (i + 1 < N && silentFrames[i + 1])
        {
            i++;
        }
        int x2 = juce::jmap((float)(i + 1), 0.0f, (float)N, (float)plotArea.getX(), (float)plotArea.getRight());

        g.fillRect(x1, plotArea.getY(), x2 - x1, plotArea.getHeight());
    }
}

void MainComponent::addParamsToFile(const juce::String& name, const std::vector<float>& data, juce::String& content)
{
    content << name << ":\n";
    for (size_t i = 0; i < data.size(); i++)
    {
        content << i + 1 << ". " << data[i] << "\n";
    }
    content << "\n";
}
