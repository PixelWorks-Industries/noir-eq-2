#include "PluginEditor.h"
#include <BinaryData.h>

namespace {
const juce::Colour bg { 0xff101210 }, panel { 0xff181c18 }, panel2 { 0xff20251f }, lineCol { 0xff343d34 }, ink { 0xffe8ece4 }, muted { 0xff899287 }, acid { 0xffc7f36b }, orange { 0xffffa45c };
const std::array<const char*, 12> ids { "lowFreq", "lowGain", "midFreq", "midGain", "midQ", "highFreq", "highGain", "threshold", "ratio", "attack", "release", "makeup" };
const std::array<const char*, 12> names { "LOW FREQ", "LOW GAIN", "MID FREQ", "MID GAIN", "MID Q", "HIGH FREQ", "HIGH GAIN", "THRESHOLD", "RATIO", "ATTACK", "RELEASE", "MAKEUP" };
juce::Font noirFont(float height, bool bold = false)
{
    // JUCE 8: build FontOptions with the chainable API (no string+float+style constructor).
    auto options = juce::FontOptions().withName("Arial").withHeight(height);
    if (bold) options = options.withStyle("Bold");
    return juce::Font(options);
}
float logNorm(float hz) { return std::log(juce::jlimit(20.0f, 20000.0f, hz) / 20.0f) / std::log(1000.0f); }
float normHz(float n) { return 20.0f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, n)); }
}

void NoirLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    const float side = (float)juce::jmin(w, h) - 13.0f;
    const float cx = x + w * 0.5f, cy = y + h * 0.43f, r = side * 0.5f;
    const float angle = start + pos * (end - start);
    juce::Rectangle<float> bounds(cx-r, cy-r, side, side);
    g.setColour(juce::Colour(0xff0c0f0c)); g.fillEllipse(bounds.expanded(2.0f));
    g.setColour(juce::Colour(0xff303830)); g.drawEllipse(bounds, 1.0f);
    juce::Path track; track.addCentredArc(cx, cy, r-3.0f, r-3.0f, 0.0f, start, end, true);
    g.setColour(juce::Colour(0xff3b4439)); g.strokePath(track, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path arc; arc.addCentredArc(cx, cy, r-3.0f, r-3.0f, 0.0f, start, angle, true);
    g.setColour(juce::Colour(0xffc7f36b)); g.strokePath(arc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path body; body.addEllipse(bounds.reduced(5.0f));
    g.setColour(juce::Colour(0xff242a23)); g.fillPath(body);
    juce::Path pointer; pointer.addRoundedRectangle(-1.3f, -r+8.0f, 2.6f, r*0.48f, 1.2f);
    g.setColour(juce::Colour(0xffe8ece4)); g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(cx,cy));
    g.setColour(juce::Colour(0xff596253)); g.drawEllipse(bounds.reduced(5.0f), 0.8f);
}

NoireqAudioProcessorEditor::NoireqAudioProcessorEditor(NoireqAudioProcessor& p)
 : AudioProcessorEditor(&p), audioProcessor(p), eqAttach(p.parameters, "eqOn", eqButton), compAttach(p.parameters, "compOn", compButton)
{
    setSize(1000, 700);
    // Embedded original SVG artwork: subtle machined-surface grain + NOIR monogram.
    // These are compiled into the plugin, so no external asset folder is needed at runtime.
    if (auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(BinaryData::noir_plate_svg, BinaryData::noir_plate_svgSize)))
        surfaceTexture = juce::Drawable::createFromSVG(*xml);
    if (auto xml = juce::XmlDocument::parse(juce::String::fromUTF8(BinaryData::noir_wordmark_svg, BinaryData::noir_wordmark_svgSize)))
        brandMark = juce::Drawable::createFromSVG(*xml);
    for (size_t i = 0; i < controls.size(); ++i) {
        controls[i] = std::make_unique<Control>(); auto& c = *controls[i];
        c.slider.setLookAndFeel(&noirLook);
        c.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        c.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 19);
        c.slider.setColour(juce::Slider::rotarySliderFillColourId, acid);
        c.slider.setColour(juce::Slider::rotarySliderOutlineColourId, lineCol);
        c.slider.setColour(juce::Slider::thumbColourId, ink);
        c.slider.setColour(juce::Slider::textBoxTextColourId, ink);
        c.slider.setColour(juce::Slider::textBoxBackgroundColourId, panel);
        c.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        c.label.setText(names[i], juce::dontSendNotification); c.label.setFont(noirFont(10.0f, true));
        c.label.setColour(juce::Label::textColourId, muted); c.label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(c.slider); addAndMakeVisible(c.label);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters, ids[i], c.slider);
    }
    for (auto* b : { &eqButton, &compButton, &resetButton }) {
        b->setColour(juce::TextButton::buttonColourId, panel2); b->setColour(juce::TextButton::buttonOnColourId, acid);
        b->setColour(juce::TextButton::textColourOffId, muted); b->setColour(juce::TextButton::textColourOnId, bg);
        addAndMakeVisible(*b);
    }
    eqButton.setClickingTogglesState(true); compButton.setClickingTogglesState(true);
    resetButton.onClick = [this] { for (auto* id : ids) if (auto* v = audioProcessor.parameters.getParameter(id)) v->setValueNotifyingHost(v->getDefaultValue()); };
    startTimerHz(24);
}

void NoireqAudioProcessorEditor::timerCallback() { repaint(); }

void NoireqAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    if (surfaceTexture) surfaceTexture->drawWithin(g, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit, 0.42f);
    // Header: compact hardware-style identity, no floating-card chrome.
    if (brandMark) brandMark->drawWithin(g, { 28.0f, 11.0f, 190.0f, 48.0f }, juce::RectanglePlacement::centred, 1.0f);
    g.setColour(acid); g.setFont(noirFont(8.5f, true)); g.drawText("TONE / DYNAMICS", juce::Rectangle<int>(29, 51, 180, 10), juce::Justification::left);
    g.setColour(muted); g.setFont(noirFont(10.0f)); g.drawText("STEREO PROCESSOR     01", juce::Rectangle<int>(700, 29, 270, 16), juce::Justification::right);
    g.setColour(lineCol); g.drawLine(28, 65, 972, 65, 1.0f);

    // Compact stereo peak meters in the header. Peak values arrive atomically from the audio thread.
    auto drawHeaderMeter = [&g](float x, const juce::String& tag, float value, juce::Colour colour)
    {
        g.setColour(muted); g.setFont(noirFont(8.5f, true));
        g.drawText(tag, juce::Rectangle<int>((int)x, 17, 30, 10), juce::Justification::left);
        const juce::Rectangle<float> track(x, 31.0f, 74.0f, 7.0f);
        g.setColour(juce::Colour(0xff252b24)); g.fillRoundedRectangle(track, 2.0f);
        const float db = juce::Decibels::gainToDecibels(juce::jmax(0.00001f, value), -60.0f);
        const float amount = juce::jlimit(0.0f, 1.0f, (db + 48.0f) / 48.0f);
        g.setColour(colour); g.fillRoundedRectangle(track.withWidth(track.getWidth() * amount), 2.0f);
        g.setColour(muted); g.setFont(noirFont(8.0f));
        g.drawText(juce::String(db, 1) + " dB", juce::Rectangle<int>((int)x, 42, 74, 10), juce::Justification::left);
    };
    drawHeaderMeter(360.0f, "IN", juce::jmax(audioProcessor.getInputPeak(0), audioProcessor.getInputPeak(1)), muted.brighter(0.35f));
    drawHeaderMeter(450.0f, "OUT", juce::jmax(audioProcessor.getOutputPeak(0), audioProcessor.getOutputPeak(1)), acid);

    graphBounds = { 28.0f, 91.0f, 944.0f, 286.0f };
    g.setColour(panel); g.fillRoundedRectangle(graphBounds, 8.0f);
    g.setColour(lineCol);
    for (int i = 0; i <= 10; ++i) { float x = graphBounds.getX() + graphBounds.getWidth() * i / 10.0f; g.drawVerticalLine((int)x, graphBounds.getY(), graphBounds.getBottom()); }
    for (int i = 0; i <= 6; ++i) { float y = graphBounds.getY() + graphBounds.getHeight() * i / 6.0f; g.drawHorizontalLine((int)y, graphBounds.getX(), graphBounds.getRight()); }
    const float zeroY = graphBounds.getCentreY();
    // Live 64-band FFT trace, drawn behind the EQ response.
    juce::Path spectrumPath;
    for (int i = 0; i < 64; ++i)
    {
        const float level = audioProcessor.getSpectrumBin(i);
        const float x = graphBounds.getX() + (float)i / 63.0f * graphBounds.getWidth();
        const float y = graphBounds.getBottom() - 12.0f - level * (graphBounds.getHeight() * 0.58f);
        if (i == 0) spectrumPath.startNewSubPath(x, y); else spectrumPath.lineTo(x, y);
    }
    juce::Path spectrumFill = spectrumPath;
    spectrumFill.lineTo(graphBounds.getRight(), graphBounds.getBottom() - 10.0f);
    spectrumFill.lineTo(graphBounds.getX(), graphBounds.getBottom() - 10.0f);
    spectrumFill.closeSubPath();
    g.setColour(juce::Colour(0xff8d9c83).withAlpha(0.10f)); g.fillPath(spectrumFill);
    g.setColour(juce::Colour(0xff8d9c83).withAlpha(0.46f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.15f));
    g.setColour(muted.withAlpha(0.7f)); g.drawHorizontalLine((int)zeroY, graphBounds.getX(), graphBounds.getRight());
    g.setFont(noirFont(9.0f));
    const char* hzLabels[] = { "20", "40", "100", "250", "500", "1k", "2k", "5k", "10k", "20k" };
    for (int i = 0; i < 10; ++i) g.drawText(hzLabels[i], juce::Rectangle<int>((int)(graphBounds.getX() + i * graphBounds.getWidth() / 10.0f + 3), (int)graphBounds.getBottom() - 17, 40, 12), juce::Justification::left);
    g.setColour(muted); g.drawText("+12", juce::Rectangle<int>(34, (int)graphBounds.getY() + 4, 32, 12), juce::Justification::centredLeft); g.drawText("0", juce::Rectangle<int>(34, (int)zeroY - 7, 32, 12), juce::Justification::centredLeft); g.drawText("-12", juce::Rectangle<int>(34, (int)graphBounds.getBottom() - 29, 32, 12), juce::Justification::centredLeft);

    auto get = [this](const char* id) { return audioProcessor.parameters.getRawParameterValue(id)->load(); };
    const float lf=get("lowFreq"), lg=get("lowGain"), mf=get("midFreq"), mg=get("midGain"), mq=get("midQ"), hf=get("highFreq"), hg=get("highGain");
    auto xFor = [this](float hz) { return graphBounds.getX() + logNorm(hz) * graphBounds.getWidth(); };
    auto yFor = [this](float db) { return graphBounds.getCentreY() - db * (graphBounds.getHeight() / 28.0f); };
    juce::Path response; bool first = true;
    for (int i=0;i<=240;++i) {
        float hz=normHz(i/240.0f), db=0.0f;
        db += lg / (1.0f + std::pow(lf / hz, 4.0f));
        float octave=std::log2(hz/mf); db += mg * std::exp(-0.5f * std::pow(octave * mq * 1.6f, 2.0f));
        db += hg / (1.0f + std::pow(hz / hf, 4.0f));
        float x=xFor(hz), y=yFor(db); if(first){response.startNewSubPath(x,y);first=false;} else response.lineTo(x,y);
    }
    juce::Path fill=response; fill.lineTo(graphBounds.getRight(),zeroY); fill.lineTo(graphBounds.getX(),zeroY); fill.closeSubPath();
    g.setColour(acid.withAlpha(0.09f)); g.fillPath(fill);
    g.setColour(acid.withAlpha(0.16f)); g.strokePath(response, juce::PathStrokeType(7.0f));
    g.setColour(acid); g.strokePath(response, juce::PathStrokeType(2.0f));
    const std::array<float,3> freqs{lf,mf,hf}, gains{lg,mg,hg};
    for(int i=0;i<3;++i){ float x=xFor(freqs[(size_t)i]), y=yFor(gains[(size_t)i]); g.setColour(bg); g.fillEllipse(x-8,y-8,16,16); g.setColour(i==1?orange:acid); g.drawEllipse(x-7,y-7,14,14,2.0f); g.fillEllipse(x-2.5f,y-2.5f,5,5); }

    g.setColour(lineCol); g.drawLine(28, 397, 972, 397, 1.0f);
    g.setColour(muted); g.setFont(noirFont(10.0f, true)); g.drawText("SHAPE", juce::Rectangle<int>(28, 410, 120, 16), juce::Justification::centredLeft); g.drawText("COMPRESS", juce::Rectangle<int>(28, 535, 120, 16), juce::Justification::centredLeft);
    g.setColour(lineCol); g.drawRoundedRectangle(28.0f, 432.0f, 944.0f, 91.0f, 6.0f, 1.0f); g.drawRoundedRectangle(28.0f, 557.0f, 944.0f, 105.0f, 6.0f, 1.0f);
    g.setColour(muted); g.setFont(noirFont(9.0f)); g.drawText("DRAG NODES TO SHAPE THE RESPONSE  /  LIVE FFT", juce::Rectangle<int>(590, 410, 380, 14), juce::Justification::right);

    // Compressor gain-reduction meter: fast attack visual, smoothed in the processor.
    const float gr = juce::jlimit(0.0f, 18.0f, audioProcessor.getGainReductionDb());
    const juce::Rectangle<float> grTrack(850.0f, 536.0f, 122.0f, 5.0f);
    g.setColour(juce::Colour(0xff252b24)); g.fillRoundedRectangle(grTrack, 2.0f);
    g.setColour(orange); g.fillRoundedRectangle(grTrack.withWidth(grTrack.getWidth() * (gr / 18.0f)), 2.0f);
    g.setColour(muted); g.setFont(noirFont(8.0f, true));
    g.drawText("GR EST.  " + juce::String(gr, 1) + " dB", juce::Rectangle<int>(820, 524, 152, 11), juce::Justification::right);
    g.setColour(lineCol); g.drawLine(28, 678, 972, 678, 1.0f);
    g.setColour(muted); g.setFont(noirFont(9.0f));
    g.drawText("ZAIN AUDIO  /  NOIR EQ", juce::Rectangle<int>(28, 681, 300, 14), juce::Justification::centredLeft);
    g.drawText("VST3     •     STEREO     •     48K READY", juce::Rectangle<int>(650, 681, 322, 14), juce::Justification::right);
}

void NoireqAudioProcessorEditor::resized()
{
    const int left=28, width=944, cell=width/6;
    for(int i=0;i<6;++i){auto& c=*controls[(size_t)i]; c.label.setBounds(left+i*cell,438,cell,15); c.slider.setBounds(left+i*cell+8,451,cell-16,68);}
    for(int i=6;i<12;++i){auto& c=*controls[(size_t)i]; c.label.setBounds(left+(i-6)*cell,565,cell,15); c.slider.setBounds(left+(i-6)*cell+8,578,cell-16,70);}
    eqButton.setBounds(750, 22, 58, 27); compButton.setBounds(814,22,72,27); resetButton.setBounds(892,22,80,27);
}

void NoireqAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (!graphBounds.contains(e.position)) return;
    auto get=[this](const char* id){return audioProcessor.parameters.getRawParameterValue(id)->load();};
    const std::array<float,3> f{get("lowFreq"),get("midFreq"),get("highFreq")}, gain{get("lowGain"),get("midGain"),get("highGain")};
    float best=18.0f; draggedBand=-1;
    for(int i=0;i<3;++i){float x=graphBounds.getX()+logNorm(f[(size_t)i])*graphBounds.getWidth();float y=graphBounds.getCentreY()-gain[(size_t)i]*(graphBounds.getHeight()/28.0f);float d=e.position.getDistanceFrom({x,y});if(d<best){best=d;draggedBand=i;}}
    if(draggedBand<0){float bestX=1.0e9f;for(int i=0;i<3;++i){float x=graphBounds.getX()+logNorm(f[(size_t)i])*graphBounds.getWidth();float d=std::abs(e.position.x-x);if(d<bestX){bestX=d;draggedBand=i;}}}
    mouseDrag(e);
}
void NoireqAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if(draggedBand<0 || !graphBounds.contains(e.position)) return;
    auto set=[this](const char* id,float value){if(auto* p=audioProcessor.parameters.getParameter(id))p->setValueNotifyingHost(p->convertTo0to1(value));};
    float hz=normHz((e.position.x-graphBounds.getX())/graphBounds.getWidth()); float db=juce::jlimit(-12.0f,12.0f,(graphBounds.getCentreY()-e.position.y)/(graphBounds.getHeight()/28.0f));
    if(draggedBand==0){set("lowFreq",juce::jlimit(20.0f,500.0f,hz));set("lowGain",db);} else if(draggedBand==1){set("midFreq",juce::jlimit(100.0f,8000.0f,hz));set("midGain",db);} else {set("highFreq",juce::jlimit(2000.0f,20000.0f,hz));set("highGain",db);}
}
void NoireqAudioProcessorEditor::mouseUp(const juce::MouseEvent&) { draggedBand=-1; }
