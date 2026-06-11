#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
KissOfShameAudioProcessor::KissOfShameAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", AudioChannelSet::stereo(), true)
                         .withOutput("Output", AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "KissOfShame", createParameterLayout())
{
    pInput        = apvts.getRawParameterValue(ParamIDs::input);
    pShame        = apvts.getRawParameterValue(ParamIDs::shame);
    pHiss         = apvts.getRawParameterValue(ParamIDs::hiss);
    pAge          = apvts.getRawParameterValue(ParamIDs::age);
    pBlend        = apvts.getRawParameterValue(ParamIDs::blend);
    pOutput       = apvts.getRawParameterValue(ParamIDs::output);
    pFlange       = apvts.getRawParameterValue(ParamIDs::flange);
    pBypass       = apvts.getRawParameterValue(ParamIDs::bypass);
    pTapeType     = apvts.getRawParameterValue(ParamIDs::tapeType);
    pPrintThrough = apvts.getRawParameterValue(ParamIDs::printThrough);
    pEnvironment  = apvts.getRawParameterValue(ParamIDs::environment);
}

AudioProcessorValueTreeState::ParameterLayout KissOfShameAudioProcessor::createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    const NormalisableRange<float> unit(0.0f, 1.0f, 0.001f);

    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { ParamIDs::input, 1 },  "Input",  unit, 0.5f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { ParamIDs::shame, 1 },  "Shame",  unit, 0.0f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { ParamIDs::hiss, 1 },   "Hiss",   unit, 0.0f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { ParamIDs::age, 1 },    "Age",    unit, 0.0f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { ParamIDs::blend, 1 },  "Blend",  unit, 1.0f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { ParamIDs::output, 1 }, "Output", unit, 0.5f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID { ParamIDs::flange, 1 }, "Flange", unit, 0.0f));

    layout.add(std::make_unique<AudioParameterBool>(ParameterID { ParamIDs::bypass, 1 },       "Bypass",        false));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { ParamIDs::tapeType, 1 },     "Tape Type", false));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { ParamIDs::printThrough, 1 }, "Print Through", false));

    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID { ParamIDs::environment, 1 }, "Environment",
        StringArray { "Off", "Studio Closet", "Humid Cellar", "Hot Locker", "Hurricane Sandy" }, 0));

    return layout;
}

AudioProcessorParameter* KissOfShameAudioProcessor::getBypassParameter() const
{
    return apvts.getParameter(ParamIDs::bypass);
}

//==============================================================================
void KissOfShameAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    graph.prepare(sampleRate, samplesPerBlock, jmax(1, getTotalNumOutputChannels()));
    updateGraphFromParameters();
}

void KissOfShameAudioProcessor::releaseResources()
{
}

bool KissOfShameAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == AudioChannelSet::mono() || in == AudioChannelSet::stereo();
}

void KissOfShameAudioProcessor::updateGraphFromParameters()
{
    graph.setBypass(pBypass->load() > 0.5f);
    graph.setInputDrive(pInput->load());
    graph.setOutputLevel(pOutput->load());
    graph.setTapeType(pTapeType->load() > 0.5f);
    graph.setShame(pShame->load(), shameExtreme.load());
    graph.setHissLevel(pHiss->load());
    graph.setBlendLevel(pBlend->load());
    graph.setFlangeDepth(pFlange->load());
    graph.setPrintThrough(pPrintThrough->load() > 0.5f);
    graph.setCurrentEnvironment((EShameEnvironments) (int) pEnvironment->load());
    graph.setAge(pAge->load());
}

void KissOfShameAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, numSamples);

    updateGraphFromParameters();

    graph.processGraph(buffer, buffer.getNumChannels());

    graphBypassed.store(graph.isGraphBypassed());

    // Levels for the VU meters; the editor polls these on its timer.
    curRMSL.store(buffer.getRMSLevel(0, 0, numSamples));
    curRMSR.store(buffer.getRMSLevel(jmin(1, buffer.getNumChannels() - 1), 0, numSamples));

    // Transport state for the reel animation.
    bool playing = false;
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            playing = position->getIsPlaying();

    transportPlaying.store(playing);
    if (playing)
        blockCounter.fetch_add(1);
}

//==============================================================================
String KissOfShameAudioProcessor::getUIEra() const
{
    const ScopedLock sl(eraLock);
    return uiEra;
}

void KissOfShameAudioProcessor::setUIEra(const String& era)
{
    const ScopedLock sl(eraLock);
    uiEra = era;
}

//==============================================================================
void KissOfShameAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty(StateIDs::shameExtreme, shameExtreme.load(), nullptr);
    state.setProperty(StateIDs::uiEra, getUIEra(), nullptr);
    state.setProperty(StateIDs::showReels, showReels.load(), nullptr);
    state.setProperty(StateIDs::uiScale, uiScale.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void KissOfShameAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName(apvts.state.getType()))
        {
            auto state = ValueTree::fromXml(*xml);

            shameExtreme.store((bool) state.getProperty(StateIDs::shameExtreme, false));
            setUIEra(state.getProperty(StateIDs::uiEra, "heritage").toString());
            showReels.store((bool) state.getProperty(StateIDs::showReels, true));
            setUIScale((float) (double) state.getProperty(StateIDs::uiScale, 1.0));

            apvts.replaceState(state);
        }
    }
}

//==============================================================================
AudioProcessorEditor* KissOfShameAudioProcessor::createEditor()
{
    return new KissOfShameAudioProcessorEditor(*this);
}

//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KissOfShameAudioProcessor();
}
