#include "PluginEditor.h"
#include <cmath>

namespace {
const auto dirt = juce::Colour::fromRGB(58, 40, 24);
const auto dirtDark = juce::Colour::fromRGB(39, 27, 18);
const auto black = juce::Colour::fromRGB(14, 14, 14);
const auto stoneGray = juce::Colour::fromRGB(110, 110, 110);
const auto stoneLight = juce::Colour::fromRGB(189, 190, 190);
const auto stoneDark = juce::Colour::fromRGB(58, 58, 58);
const auto grass = juce::Colour::fromRGB(121, 202, 64);
const auto gold = juce::Colour::fromRGB(255, 229, 39);
const auto pale = juce::Colour::fromRGB(241, 241, 241);
const auto gridGreen = juce::Colour::fromRGB(61, 106, 45);
const auto gridDark = juce::Colour::fromRGB(22, 39, 18);
const std::array<const char*, 9> names{{"SATURATION", "DISTORTION", "AIR", "PUNCH", "WET", "DE-ESS", "COLOR EQ", "CHORUS", "COMPRESSOR"}};
const std::array<int, 9> pids{{SauceDSP::sat, SauceDSP::dist, SauceDSP::air, SauceDSP::punch, SauceDSP::wet, SauceDSP::deess, SauceDSP::color, SauceDSP::chorus, SauceDSP::comp}};

struct DetailSpec { int owner; const char* id; const char* title; };
const std::array<DetailSpec, 24> specs{{
    {0, "sat_drive", "DRIVE"}, {0, "sat_tone", "TONE"}, {0, "sat_output", "OUTPUT"},
    {1, "dist_drive", "DRIVE"}, {1, "dist_hp", "LOW CUT"}, {1, "dist_lp", "HIGH CUT"}, {1, "dist_output", "OUTPUT"},
    {2, "air_freq", "AIR FREQ"}, {2, "air_boost", "AIR BOOST"},
    {3, "punch_attack", "ATTACK"}, {3, "punch_release", "RELEASE"},
    {4, "wet_output", "OUTPUT TRIM"},
    {5, "deess_freq", "SIBILANCE FREQ"}, {5, "deess_amount", "DEPTH"},
    {6, "color_tilt", "TILT"}, {6, "color_bias", "BIAS"},
    {7, "chorus_rate", "RATE"}, {7, "chorus_depth", "DEPTH"}, {7, "chorus_width", "WIDTH"},
    {8, "comp_threshold", "THRESHOLD"}, {8, "comp_ratio", "RATIO"}, {8, "comp_attack", "ATTACK"}, {8, "comp_release", "RELEASE"}, {8, "comp_makeup", "MAKEUP"}
}};

juce::Font pixelFont(float size, bool bold = true) {
    return juce::Font(juce::Font::getDefaultMonospacedFontName(), size, bold ? juce::Font::bold : juce::Font::plain);
}

int eqGainId(int idx) { return SauceDSP::eqGain1 + idx; }
int eqFreqId(int idx) { return SauceDSP::eqFreq1 + idx; }
} // namespace

void PixelLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float position, float, float, juce::Slider&) {
    auto bounds = juce::Rectangle<int>(x, y, w, h);
    const int size = std::min(w, h) - 10;
    const int left = bounds.getCentreX() - size / 2;
    const int top = bounds.getCentreY() - size / 2;
    g.setColour(black); g.fillRect(left - 4, top - 4, size + 8, size + 8);
    g.setColour(stoneGray); g.fillRect(left, top, size, size);
    g.setColour(stoneLight); g.fillRect(left, top, size, 4); g.fillRect(left, top, 4, size);
    g.setColour(stoneDark); g.fillRect(left, top + size - 4, size, 4); g.fillRect(left + size - 4, top, 4, size);
    g.setColour(juce::Colour::fromRGB(50, 50, 50)); g.fillRect(left + 10, top + 10, size - 20, size - 20);
    const auto angle = (-2.35f + position * 4.7f);
    const int bx = left + size / 2 + static_cast<int>(std::sin(angle) * (size * .29f));
    const int by = top + size / 2 - static_cast<int>(std::cos(angle) * (size * .29f));
    g.setColour(gold); g.fillRect(bx - 4, by - 4, 8, 8);
    g.setColour(black); g.fillRect(left + size / 2 - 3, top + size / 2 - 3, 6, 6);
}

void PixelLook::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) {
    auto r = b.getLocalBounds();
    g.setColour(black); g.fillRect(r.translated(2, 3));
    g.setColour(stoneDark); g.fillRect(r.reduced(1).withTrimmedBottom(2));
    g.setColour(down ? grass : (over ? stoneLight : stoneGray)); g.fillRect(r.reduced(4).withTrimmedBottom(2));
    g.setColour(stoneLight); g.fillRect(r.reduced(4).removeFromTop(2));
}

void PixelLook::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) {
    g.setFont(pixelFont(b.getHeight() < 32 ? 13.f : 17.f));
    g.setColour(pale);
    g.drawText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, true);
}

juce::Point<float> EQGraph::point(int band) const {
    const double freq = processor.param(eqFreqId(band));
    const double gain = processor.param(eqGainId(band));
    const auto r = getLocalBounds().reduced(18, 12).toFloat();
    const float x = r.getX() + static_cast<float>(std::log(freq / 30.0) / std::log(16500.0 / 30.0)) * r.getWidth();
    const float y = r.getY() + static_cast<float>((36.0 - gain) / 72.0) * r.getHeight();
    return { x, y };
}

void EQGraph::paint(juce::Graphics& g) {
    auto r = getLocalBounds();
    g.fillAll(gridDark);
    g.setColour(gridGreen);
    for (int i = 1; i < 12; ++i) g.fillRect(r.getWidth() * i / 12, 0, 1, r.getHeight());
    for (int i = 1; i < 8; ++i) g.fillRect(0, r.getHeight() * i / 8, r.getWidth(), 1);
    g.setColour(juce::Colour::fromRGB(104, 140, 87));
    g.fillRect(0, r.getHeight() / 2, r.getWidth(), 2);

    static const std::array<double, SauceDSP::eqBands> widths{{1.8, 1.35, 1.1, 1.1, 1.3, 1.7}};
    juce::Path curve;
    for (int x = 0; x < r.getWidth(); x += 3) {
        const double f = 30.0 * std::pow(16500.0 / 30.0, double(x) / std::max(1, r.getWidth()));
        double gain = 0.0;
        for (int k = 0; k < SauceDSP::eqBands; ++k) {
            const double fs = processor.param(eqFreqId(k));
            const double gs = processor.param(eqGainId(k));
            const double delta = std::log2(f / fs);
            gain += gs * std::exp(-0.5 * delta * delta / (widths[(size_t)k] * widths[(size_t)k]));
        }
        const float y = float(r.getHeight() * .5 - gain * r.getHeight() / 72.0);
        if (x == 0) curve.startNewSubPath((float)x, y); else curve.lineTo((float)x, y);
    }
    g.setColour(grass); g.strokePath(curve, juce::PathStrokeType(3.0f));

    for (int k = 0; k < SauceDSP::eqBands; ++k) {
        auto p = point(k);
        g.setColour(black); g.fillRect((int)p.x - 9, (int)p.y - 9, 18, 18);
        g.setColour((k % 2 == 0) ? gold : grass); g.fillRect((int)p.x - 6, (int)p.y - 6, 12, 12);
    }

    g.setFont(pixelFont(12));
    g.setColour(pale);
    g.drawText("+36", 5, 4, 35, 16, juce::Justification::centredLeft);
    g.drawText("0", 5, r.getHeight() / 2 - 8, 20, 16, juce::Justification::centredLeft);
    g.drawText("-36", 5, r.getHeight() - 20, 35, 16, juce::Justification::centredLeft);
    g.drawText("30 HZ", 8, r.getHeight() - 20, 70, 16, juce::Justification::centredLeft);
    g.drawText("16.5 KHZ", r.getWidth() - 100, r.getHeight() - 20, 92, 16, juce::Justification::centredRight);
    g.setColour(stoneLight);
    g.drawRect(r, 2);
}

void EQGraph::mouseDown(const juce::MouseEvent& e) {
    float best = 625;
    selected = -1;
    for (int k = 0; k < SauceDSP::eqBands; ++k) {
        const float dist = point(k).getDistanceSquaredFrom(e.position);
        if (dist < best) { best = dist; selected = k; }
    }
    if (selected >= 0) {
        processor.parameters.getParameter(SauceDSP::ids[(size_t)eqGainId(selected)])->beginChangeGesture();
        processor.parameters.getParameter(SauceDSP::ids[(size_t)eqFreqId(selected)])->beginChangeGesture();
        move(e);
    }
}

void EQGraph::mouseDrag(const juce::MouseEvent& e) { if (selected >= 0) move(e); }

void EQGraph::move(const juce::MouseEvent& e) {
    if (selected < 0) return;
    auto r = getLocalBounds().reduced(18, 12).toFloat();
    const double xf = std::clamp(double((e.position.x - r.getX()) / r.getWidth()), 0.0, 1.0);
    const double yf = std::clamp(double((e.position.y - r.getY()) / r.getHeight()), 0.0, 1.0);
    const float hz = static_cast<float>(30.0 * std::pow(16500.0 / 30.0, xf));
    const float db = static_cast<float>(36.0 - yf * 72.0);
    auto* freqParam = processor.parameters.getParameter(SauceDSP::ids[(size_t)eqFreqId(selected)]);
    auto* gainParam = processor.parameters.getParameter(SauceDSP::ids[(size_t)eqGainId(selected)]);
    freqParam->setValueNotifyingHost(freqParam->convertTo0to1(hz));
    gainParam->setValueNotifyingHost(gainParam->convertTo0to1(db));
    repaint();
}

void EQGraph::mouseUp(const juce::MouseEvent&) {
    if (selected >= 0) {
        processor.parameters.getParameter(SauceDSP::ids[(size_t)eqGainId(selected)])->endChangeGesture();
        processor.parameters.getParameter(SauceDSP::ids[(size_t)eqFreqId(selected)])->endChangeGesture();
        selected = -1;
    }
}

VocalSauceEditor::VocalSauceEditor(VocalSauceProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p), graph(p) {
    setLookAndFeel(&look);
    setSize(width, height);

    for (size_t i = 0; i < effects.size(); ++i) {
        auto& tile = effects[i];
        addAndMakeVisible(tile.knob); addAndMakeVisible(tile.more);
        addAndMakeVisible(tile.name); addAndMakeVisible(tile.value);
        tile.name.setFont(pixelFont(16)); tile.name.setColour(juce::Label::textColourId, pale);
        tile.name.setJustificationType(juce::Justification::centred);
        tile.name.setText(names[i], juce::dontSendNotification);
        tile.value.setFont(pixelFont(14)); tile.value.setColour(juce::Label::textColourId, gold);
        tile.value.setJustificationType(juce::Justification::centred);
        tile.knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        tile.knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        tile.knob.setDoubleClickReturnValue(true, (i == 4) ? 100.0 : 0.0);
        tile.attachment = std::make_unique<Attachment>(processor.parameters, SauceDSP::ids[(size_t)pids[i]], tile.knob);
        tile.knob.onValueChange = [this, i] {
            effects[i].value.setText(juce::String(effects[i].knob.getValue(), 1) + " %", juce::dontSendNotification);
        };
        tile.knob.onValueChange();
        tile.more.onClick = [this, i] { setExpanded((int)i); };
    }

    addAndMakeVisible(graph); addAndMakeVisible(eqExpand);
    eqExpand.onClick = [this] { setExpanded(9); };

    for (int i = 0; i < eqVisibleKnobs; ++i) {
        addAndMakeVisible(eqKnobs[(size_t)i]); addAndMakeVisible(eqLabels[(size_t)i]);
        eqKnobs[(size_t)i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
        eqKnobs[(size_t)i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
        eqKnobs[(size_t)i].setColour(juce::Slider::textBoxTextColourId, gold);
        eqKnobs[(size_t)i].setColour(juce::Slider::textBoxOutlineColourId, black);
        eqKnobs[(size_t)i].setColour(juce::Slider::textBoxBackgroundColourId, dirtDark);
        eqLabels[(size_t)i].setText("BAND " + juce::String(i + 1), juce::dontSendNotification);
        eqLabels[(size_t)i].setFont(pixelFont(12));
        eqLabels[(size_t)i].setColour(juce::Label::textColourId, pale);
        eqLabels[(size_t)i].setJustificationType(juce::Justification::centred);
        eqAttachments[(size_t)i] = std::make_unique<Attachment>(processor.parameters, SauceDSP::ids[(size_t)eqGainId(i)], eqKnobs[(size_t)i]);
    }

    for (size_t i = 0; i < details.size(); ++i) {
        auto& d = details[i]; d.owner = specs[i].owner;
        addAndMakeVisible(d.slider); addAndMakeVisible(d.label);
        d.slider.setSliderStyle(juce::Slider::LinearHorizontal);
        d.slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 24);
        d.slider.setColour(juce::Slider::trackColourId, grass);
        d.slider.setColour(juce::Slider::thumbColourId, gold);
        d.slider.setColour(juce::Slider::textBoxTextColourId, gold);
        d.slider.setColour(juce::Slider::textBoxOutlineColourId, black);
        d.slider.setColour(juce::Slider::textBoxBackgroundColourId, dirtDark);
        d.label.setText(specs[i].title, juce::dontSendNotification);
        d.label.setFont(pixelFont(13)); d.label.setColour(juce::Label::textColourId, pale);
        d.attachment = std::make_unique<Attachment>(processor.parameters, specs[i].id, d.slider);
    }

    addAndMakeVisible(detailHint);
    detailHint.setFont(pixelFont(14, false));
    detailHint.setColour(juce::Label::textColourId, pale);
    detailHint.setJustificationType(juce::Justification::centred);
    detailHint.setText("PRESS + TO OPEN EFFECT SETTINGS", juce::dontSendNotification);

    addAndMakeVisible(saveButton); addAndMakeVisible(loadButton); addAndMakeVisible(version);
    saveButton.onClick = [this] { savePreset(); };
    loadButton.onClick = [this] { loadPreset(); };
    version.setText("V0.3 REWORK  |  WINDOWS X64 VST3  |  USER PRESETS READY", juce::dontSendNotification);
    version.setFont(pixelFont(11)); version.setColour(juce::Label::textColourId, stoneLight);

    resized();
    setExpanded(-1);
    startTimerHz(20);
}

VocalSauceEditor::~VocalSauceEditor() { stopTimer(); chooser.reset(); setLookAndFeel(nullptr); }

void VocalSauceEditor::stone(juce::Graphics& g, juce::Rectangle<int> r) {
    g.setColour(black); g.fillRect(r.translated(3, 4));
    g.setColour(stoneDark); g.fillRect(r);
    g.setColour(stoneGray); g.fillRect(r.reduced(4));
    g.setColour(stoneLight); g.fillRect(r.reduced(4).removeFromTop(3));
}

void VocalSauceEditor::paint(juce::Graphics& g) {
    g.fillAll(dirt);
    juce::Random rng(1717);
    const juce::Colour grains[] { juce::Colour::fromRGB(69, 49, 33), juce::Colour::fromRGB(49, 34, 22), juce::Colour::fromRGB(84, 58, 41) };
    for (int i = 0; i < 3200; ++i) {
        g.setColour(grains[i % 3]);
        g.fillRect(rng.nextInt(width / 4) * 4, rng.nextInt(height / 4) * 4, 4, 4);
    }
    g.setColour(grass); g.fillRect(0, 0, width, 18);
    stone(g, { 14, 18, width - 28, 72 });
    g.setFont(pixelFont(30)); g.setColour(black);
    g.drawText("17TESS VOCAL SAUCE", 35, 37, 650, 38, juce::Justification::centredLeft);
    g.setColour(pale);
    g.drawText("17TESS VOCAL SAUCE", 32, 34, 650, 38, juce::Justification::centredLeft);
    g.setFont(pixelFont(16)); g.setColour(gold);
    g.drawText("PIXEL EDITION", width - 280, 42, 238, 28, juce::Justification::centredRight);

    for (int i = 0; i < effectCount; ++i) {
        const int row = (i < 5) ? 0 : 1;
        const int col = (i < 5) ? i : i - 5;
        const int x = (row == 0) ? 14 + col * 250 : 140 + col * 250;
        const int y = 104 + row * 160;
        stone(g, { x, y, 236, 148 });
        if (expanded == i) { g.setColour(grass); g.drawRect(x + 3, y + 3, 230, 142, 3); }
    }

    stone(g, { 14, 430, 760, 400 });
    stone(g, { 788, 430, 478, 400 });
    g.setColour(pale); g.setFont(pixelFont(18));
    g.drawText("EQ / DRAG THE PIXEL DOTS", 28, 444, 480, 28, juce::Justification::centredLeft);
    juce::String title = "DETAIL SETTINGS";
    if (expanded >= 0 && expanded < effectCount) title = juce::String(names[(size_t)expanded]) + " / DETAILS";
    else if (expanded == 9) title = "EQ / ADVANCED";
    g.drawText(title, 804, 444, 410, 28, juce::Justification::centredLeft);

    g.setColour(stoneLight); g.fillRect(16, 846, width - 32, 2);
    g.setFont(pixelFont(12)); g.setColour(grass);
    g.drawText("NO FACTORY PRESETS", 20, 862, 240, 24, juce::Justification::centredLeft);
}

void VocalSauceEditor::resized() {
    for (int i = 0; i < effectCount; ++i) {
        const int row = (i < 5) ? 0 : 1;
        const int col = (i < 5) ? i : i - 5;
        const int x = (row == 0) ? 14 + col * 250 : 140 + col * 250;
        const int y = 104 + row * 160;
        auto& tile = effects[(size_t)i];
        tile.name.setBounds(x + 12, y + 10, 194, 24);
        tile.more.setBounds(x + 198, y + 8, 28, 28);
        tile.knob.setBounds(x + 72, y + 38, 92, 86);
        tile.value.setBounds(x + 44, y + 122, 148, 21);
    }

    graph.setBounds(30, 478, 728, 190);
    eqExpand.setBounds(726, 445, 30, 26);
    for (int i = 0; i < eqVisibleKnobs; ++i) {
        const int row = i / 3;
        const int col = i % 3;
        const int baseX = 52 + col * 228;
        const int baseY = 684 + row * 76;
        eqKnobs[(size_t)i].setBounds(baseX, baseY, 90, 72);
        eqLabels[(size_t)i].setBounds(baseX + 88, baseY + 19, 84, 24);
    }

    detailHint.setBounds(820, 560, 410, 62);
    int visibleIndex = 0;
    for (auto& d : details) {
        if (d.owner == expanded) {
            const int y = 486 + visibleIndex * 46;
            d.label.setBounds(815, y, 148, 26);
            d.slider.setBounds(968, y, 275, 26);
            ++visibleIndex;
        }
    }

    saveButton.setBounds(792, 856, 210, 44);
    loadButton.setBounds(1015, 856, 250, 44);
    version.setBounds(270, 858, 470, 36);
}

void VocalSauceEditor::setExpanded(int selectedEffect) {
    expanded = (selectedEffect == expanded ? -1 : selectedEffect);
    bool hasDetails = false;
    for (auto& d : details) {
        const bool show = (d.owner == expanded);
        d.slider.setVisible(show);
        d.label.setVisible(show);
        hasDetails |= show;
    }
    detailHint.setVisible(!hasDetails);
    if (!hasDetails) {
        juce::String msg = "PRESS + TO OPEN EFFECT SETTINGS";
        if (expanded == 9) msg = "EQ NOW HAS 6 FREE DOTS / RANGE -36 TO +36 DB";
        detailHint.setText(msg, juce::dontSendNotification);
    }
    resized();
    repaint();
}

void VocalSauceEditor::showError(const juce::String& msg) {
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "17TESS VOCAL SAUCE", msg);
}

void VocalSauceEditor::savePreset() {
    auto initial = processor.getPresetDirectory().getChildFile("my_vocal.17tesspreset");
    chooser = std::make_unique<juce::FileChooser>("Save 17TESS preset", initial, "*.17tesspreset");
    juce::Component::SafePointer<VocalSauceEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
        juce::FileBrowserComponent::canSelectFiles |
        juce::FileBrowserComponent::warnAboutOverwriting,
        [safe](const juce::FileChooser& dialog) {
            if (!safe) return;
            auto file = dialog.getResult(); if (file == juce::File()) return;
            if (!file.hasFileExtension("17tesspreset")) file = file.withFileExtension(".17tesspreset");
            juce::String error;
            if (!safe->processor.savePreset(file, error)) safe->showError(error);
        });
}

void VocalSauceEditor::loadPreset() {
    chooser = std::make_unique<juce::FileChooser>("Load 17TESS preset", processor.getPresetDirectory(), "*.17tesspreset");
    juce::Component::SafePointer<VocalSauceEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectFiles,
        [safe](const juce::FileChooser& dialog) {
            if (!safe) return;
            auto file = dialog.getResult(); if (!file.existsAsFile()) return;
            juce::String error;
            if (!safe->processor.loadPreset(file, error)) safe->showError(error);
        });
}

void VocalSauceEditor::timerCallback() { graph.repaint(); }
