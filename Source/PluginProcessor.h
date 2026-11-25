#pragma once
#include <JuceHeader.h>
#include <array>

class NoireqAudioProcessor final : public juce::AudioProcessor
{
public:
    NoireqAudioProcessor();
    ~NoireqAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "NOIR EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();

    float getInputPeak(int channel) const noexcept { return inputPeak[(size_t) juce::jlimit(0, 1, channel)].load(); }
    float getOutputPeak(int channel) const noexcept { return outputPeak[(size_t) juce::jlimit(0, 1, channel)].load(); }
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }
    float getSpectrumBin(int index) const noexcept { return spectrum[(size_t) juce::jlimit(0, 63, index)].load(); }
private:
    juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Filter<float>, juce::dsp::Compressor<float>> chainL, chainR;
    void updateFilters();
    std::array<float, 7> lastEqParams {};
    std::array<float, 4> lastCompParams {};
    bool filtersInitialised = false;
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> fftWindow { fftSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, fftSize * 2> fftData {};
    int fftWritePosition = 0;
    std::array<std::atomic<float>, 64> spectrum {};
    std::array<std::atomic<float>, 2> inputPeak {}, outputPeak {};
    std::atomic<float> gainReductionDb { 0.0f };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoireqAudioProcessor)
};
