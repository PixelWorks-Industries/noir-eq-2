#include "PluginProcessor.h"
#include "PluginEditor.h"

NoireqAudioProcessor::APVTS::ParameterLayout NoireqAudioProcessor::createParameterLayout()
{
    using namespace juce;
    APVTS::ParameterLayout p;
    p.add(std::make_unique<AudioParameterFloat>("lowFreq", "LOW FREQ", NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.35f), 90.0f));
    p.add(std::make_unique<AudioParameterFloat>("lowGain", "LOW", NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    p.add(std::make_unique<AudioParameterFloat>("midFreq", "MID FREQ", NormalisableRange<float>(100.0f, 8000.0f, 1.0f, 0.3f), 1200.0f));
    p.add(std::make_unique<AudioParameterFloat>("midGain", "MID", NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    p.add(std::make_unique<AudioParameterFloat>("midQ", "MID Q", NormalisableRange<float>(0.2f, 8.0f, 0.01f, 0.5f), 1.0f));
    p.add(std::make_unique<AudioParameterFloat>("highFreq", "HIGH FREQ", NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.3f), 9000.0f));
    p.add(std::make_unique<AudioParameterFloat>("highGain", "HIGH", NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    p.add(std::make_unique<AudioParameterFloat>("threshold", "THRESHOLD", NormalisableRange<float>(-48.0f, 0.0f, 0.1f), -18.0f));
    p.add(std::make_unique<AudioParameterFloat>("ratio", "RATIO", NormalisableRange<float>(1.0f, 20.0f, 0.1f), 3.0f));
    p.add(std::make_unique<AudioParameterFloat>("attack", "ATTACK", NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.5f), 15.0f));
    p.add(std::make_unique<AudioParameterFloat>("release", "RELEASE", NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.5f), 120.0f));
    p.add(std::make_unique<AudioParameterFloat>("makeup", "MAKEUP", NormalisableRange<float>(-6.0f, 18.0f, 0.1f), 0.0f));
    p.add(std::make_unique<AudioParameterBool>("compOn", "COMP", true));
    p.add(std::make_unique<AudioParameterBool>("eqOn", "EQ", true));
    return p;
}

NoireqAudioProcessor::NoireqAudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo(), true).withOutput("Output", AudioChannelSet::stereo(), true)),
   parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (auto& v : spectrum) v.store(0.0f);
    for (auto& v : inputPeak) v.store(0.0f);
    for (auto& v : outputPeak) v.store(0.0f);
}

bool NoireqAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{ return l.getMainInputChannelSet() == l.getMainOutputChannelSet() && (l.getMainOutputChannelSet() == juce::AudioChannelSet::mono() || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()); }

void NoireqAudioProcessor::prepareToPlay(double sr, int block)
{
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) block, 1u };
    chainL.prepare(spec); chainR.prepare(spec);
    for (auto& v : spectrum) v.store(0.0f);
    for (auto& v : inputPeak) v.store(0.0f);
    for (auto& v : outputPeak) v.store(0.0f);
    gainReductionDb.store(0.0f);
    fftData.fill(0.0f); fftWritePosition = 0;
    filtersInitialised = false;
    updateFilters();
}
void NoireqAudioProcessor::releaseResources() {}

void NoireqAudioProcessor::updateFilters()
{
    const auto sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    auto val = [this](const char* id) { return parameters.getRawParameterValue(id)->load(); };

    const std::array<float, 7> eqNow {
        val("lowFreq"), val("lowGain"), val("midFreq"), val("midGain"),
        val("midQ"), val("highFreq"), val("highGain")
    };
    const std::array<float, 4> compNow {
        val("threshold"), val("ratio"), val("attack"), val("release")
    };

    if (! filtersInitialised || eqNow != lastEqParams)
    {
        auto low = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sr, eqNow[0], 0.707f, juce::Decibels::decibelsToGain(eqNow[1]));
        auto mid = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sr, eqNow[2], eqNow[4], juce::Decibels::decibelsToGain(eqNow[3]));
        auto high = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sr, eqNow[5], 0.707f, juce::Decibels::decibelsToGain(eqNow[6]));
        for (auto* c : { &chainL, &chainR })
        {
            c->get<0>().coefficients = low;
            c->get<1>().coefficients = mid;
            c->get<2>().coefficients = high;
        }
        lastEqParams = eqNow;
    }

    if (! filtersInitialised || compNow != lastCompParams)
    {
        for (auto* c : { &chainL, &chainR })
        {
            c->get<4>().setThreshold(compNow[0]);
            c->get<4>().setRatio(compNow[1]);
            c->get<4>().setAttack(compNow[2]);
            c->get<4>().setRelease(compNow[3]);
        }
        lastCompParams = compNow;
    }
    filtersInitialised = true;
}

void NoireqAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateFilters();
    const bool eq = parameters.getRawParameterValue("eqOn")->load() > 0.5f;
    const bool comp = parameters.getRawParameterValue("compOn")->load() > 0.5f;
    const float threshold = parameters.getRawParameterValue("threshold")->load();
    const float ratio = juce::jmax(1.0f, parameters.getRawParameterValue("ratio")->load());
    const float makeup = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("makeup")->load());
    float blockPeakIn[2] { 0.0f, 0.0f }, blockPeakOut[2] { 0.0f, 0.0f };

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getReadPointer(ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
            blockPeakIn[ch] = juce::jmax(blockPeakIn[ch], std::abs(data[n]));

        auto block = juce::dsp::AudioBlock<float>(buffer).getSingleChannelBlock((size_t) ch);
        juce::dsp::ProcessContextReplacing<float> context(block);
        auto& chain = ch == 0 ? chainL : chainR;
        chain.setBypassed<0>(!eq); chain.setBypassed<1>(!eq); chain.setBypassed<2>(!eq);
        chain.setBypassed<3>(true);
        chain.setBypassed<4>(!comp);
        chain.process(context);
        if (makeup != 1.0f) buffer.applyGain(ch, 0, buffer.getNumSamples(), makeup);

        const auto* out = buffer.getReadPointer(ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
            blockPeakOut[ch] = juce::jmax(blockPeakOut[ch], std::abs(out[n]));
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        const float in = blockPeakIn[ch], out = blockPeakOut[ch];
        inputPeak[(size_t) ch].store(juce::jmax(in, inputPeak[(size_t) ch].load() * 0.86f));
        outputPeak[(size_t) ch].store(juce::jmax(out, outputPeak[(size_t) ch].load() * 0.86f));
    }

    // A useful live GR estimate from the block peak and the configured static curve.
    // It is a meter estimate, not an internal readout from JUCE's compressor detector.
    const float peakDb = juce::Decibels::gainToDecibels(juce::jmax(blockPeakIn[0], blockPeakIn[1]), -100.0f);
    const float over = juce::jmax(0.0f, peakDb - threshold);
    const float gr = comp ? over * (1.0f - 1.0f / ratio) : 0.0f;
    gainReductionDb.store(gr + gainReductionDb.load() * 0.72f);

    // Feed a mono tap into a 2048-point FFT. Audio thread owns the FFT buffer;
    // the UI reads only the smoothed atomic band values.
    const int channels = buffer.getNumChannels();
    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        float sample = 0.0f;
        for (int ch = 0; ch < channels; ++ch) sample += buffer.getSample(ch, n);
        sample /= (float) juce::jmax(1, channels);
        fftData[(size_t) fftWritePosition++] = sample;
        if (fftWritePosition == fftSize)
        {
            std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
            fftWindow.multiplyWithWindowingTable(fftData.data(), fftSize);
            fft.performFrequencyOnlyForwardTransform(fftData.data());
            for (int band = 0; band < 64; ++band)
            {
                const float lo = std::pow((float) (fftSize / 2), band / 64.0f);
                const float hi = std::pow((float) (fftSize / 2), (band + 1) / 64.0f);
                const int bin = juce::jlimit(1, fftSize / 2 - 1, (int) ((lo + hi) * 0.5f));
                const float magnitude = juce::jlimit(0.0f, 1.0f, fftData[(size_t) bin] * (4.0f / (float) fftSize));
                const float previous = spectrum[(size_t) band].load();
                spectrum[(size_t) band].store(juce::jmax(magnitude, previous * 0.84f));
            }
            fftWritePosition = 0;
        }
    }
}

juce::AudioProcessorEditor* NoireqAudioProcessor::createEditor() { return new NoireqAudioProcessorEditor(*this); }
void NoireqAudioProcessor::getStateInformation(juce::MemoryBlock& d) { auto s = parameters.copyState(); std::unique_ptr<juce::XmlElement> xml(s.createXml()); copyXmlToBinary(*xml, d); }
void NoireqAudioProcessor::setStateInformation(const void* d, int n) { std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(d, n)); if (xml && xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NoireqAudioProcessor(); }
