#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace SauceDSP {
constexpr int numParams = 45;
enum Id {
    sat, dist, air, punch, wet, deess, color, chorus, comp,
    satDrive, satTone, satOutput,
    distDrive, distHP, distLP, distOutput,
    airFreq, airBoost,
    punchAttack, punchRelease,
    wetOutput,
    deessFreq, deessAmount,
    colorTilt, colorBias,
    chorusRate, chorusDepth, chorusWidth,
    compThreshold, compRatio, compAttack, compRelease, compMakeup,
    eqGain1, eqGain2, eqGain3, eqGain4, eqGain5, eqGain6,
    eqFreq1, eqFreq2, eqFreq3, eqFreq4, eqFreq5, eqFreq6
};
constexpr int eqBands = 6;
inline constexpr std::array<const char*, numParams> ids {{
    "sat", "dist", "air", "punch", "wet", "deess", "color", "chorus", "comp",
    "sat_drive", "sat_tone", "sat_output",
    "dist_drive", "dist_hp", "dist_lp", "dist_output",
    "air_freq", "air_boost",
    "punch_attack", "punch_release",
    "wet_output",
    "deess_freq", "deess_amount",
    "color_tilt", "color_bias",
    "chorus_rate", "chorus_depth", "chorus_width",
    "comp_threshold", "comp_ratio", "comp_attack", "comp_release", "comp_makeup",
    "eq_gain1", "eq_gain2", "eq_gain3", "eq_gain4", "eq_gain5", "eq_gain6",
    "eq_freq1", "eq_freq2", "eq_freq3", "eq_freq4", "eq_freq5", "eq_freq6"
}};
inline constexpr std::array<float, numParams> defaults {{
    0.f, 0.f, 0.f, 0.f, 100.f, 0.f, 0.f, 0.f, 0.f,
    60.f, 50.f, 0.f,
    60.f, 65.f, 15000.f, 0.f,
    5800.f, 55.f,
    90.f, 180.f,
    0.f,
    6600.f, 50.f,
    0.f, 50.f,
    0.8f, 40.f, 50.f,
    -18.f, 4.f, 12.f, 80.f, 0.f,
    -12.f, 0.f, 0.f, 0.f, 9.f, 0.f,
    60.f, 160.f, 650.f, 1800.f, 5500.f, 12000.f
}};

inline double clamp(double x, double lo, double hi) noexcept { return std::min(hi, std::max(lo, x)); }
inline double onePole(double hz, double sampleRate) noexcept {
    hz = clamp(hz, 5.0, sampleRate * 0.45);
    return 1.0 - std::exp(-6.283185307179586 * hz / sampleRate);
}
inline double softclip(double x) noexcept { return std::tanh(clamp(x, -24.0, 24.0)); }
inline double decibels(double db) noexcept { return std::pow(10.0, db / 20.0); }

struct Channel {
    double hp = 0, lp = 0, satToneLP = 0, airLP = 0, deessLP = 0, deessEnvelope = 0;
    double fast = 0, slow = 0, colorLow = 0, colorMid = 0, compEnv = 0;
    std::array<double, eqBands> eqLP{};
    double chorusPhase = 0;
    std::vector<float> delay;
    size_t index = 0;
    void reset() noexcept {
        hp = lp = satToneLP = airLP = deessLP = deessEnvelope = fast = slow = 0;
        colorLow = colorMid = compEnv = chorusPhase = 0; index = 0;
        eqLP.fill(0.0);
        std::fill(delay.begin(), delay.end(), 0.f);
    }
};

class Engine {
public:
    void prepare(double newRate, int channels) {
        sr = std::max(8000.0, newRate);
        for (auto& c : states) {
            c.delay.assign(static_cast<size_t>(std::ceil(sr * 0.12)) + 8, 0.f);
            c.reset();
        }
        usedChannels = std::clamp(channels, 1, 2);
    }
    void reset() noexcept { for (auto& c : states) c.reset(); }

    void process(float* const* channels, int channelCount, int samples,
                 const std::array<float, numParams>& p) noexcept {
        if (!channels || samples <= 0) return;
        const int number = std::min({ channelCount, usedChannels, 2 });

        const double satAmt = clamp(p[sat] * 0.03, 0.0, 1.0);
        const double distAmt = clamp(p[dist] * 0.02, 0.0, 1.0);
        const double airAmt = clamp(p[air] * 0.03, 0.0, 1.0);
        const double punchAmt = clamp(p[punch] * 0.03, 0.0, 1.0);
        const double wetAmt = clamp((p[wet] * 0.01), 0.0, 1.0);
        const double deessAmt = clamp(p[deess] * 0.03, 0.0, 1.0);
        const double colorAmt = clamp(p[color] * 0.03, 0.0, 1.0);
        const double chorusAmt = clamp(p[chorus] * 0.03, 0.0, 1.0);
        const double compAmt = clamp(p[comp] * 0.03, 0.0, 1.0);

        const double satDrive = 1.0 + 7.0 * (p[satDrive] / 100.0);
        const double satToneCoeff = onePole(350.0 + p[satTone] * 95.0, sr);
        const double satOut = decibels(p[satOutput]);

        const double distDrive = 1.0 + 18.0 * (p[distDrive] / 100.0);
        const double distHpCoeff = onePole(p[distHP], sr);
        const double distLpCoeff = onePole(p[distLP], sr);
        const double distOut = decibels(p[distOutput]);

        const double airCoeff = onePole(p[airFreq], sr);
        const double airBoost = 0.25 + (p[airBoost] / 100.0) * 2.6;

        const double punchAttackCoeff = onePole(p[punchAttack], sr);
        const double punchReleaseCoeff = onePole(p[punchRelease], sr);

        const double deessCoeff = onePole(p[deessFreq], sr);
        const double deessExtra = 0.35 + (p[deessAmount] / 100.0) * 2.4;
        const double deessEnvCoeff = onePole(120.0, sr);

        const double colorLowCoeff = onePole(220.0, sr);
        const double colorMidCoeff = onePole(2400.0, sr);
        const double colorTilt = clamp(p[colorTilt] / 100.0, -1.0, 1.0);
        const double colorBias = p[colorBias] / 100.0;

        const double chorusRate = clamp(p[chorusRate], 0.05, 8.0);
        const double chorusDepth = clamp(p[chorusDepth] / 100.0, 0.0, 1.0);
        const double chorusWidth = clamp(p[chorusWidth] / 100.0, 0.0, 1.0);
        const double chorusPhaseStep = 6.283185307179586 * chorusRate / sr;

        const double compThreshold = p[compThreshold];
        const double compRatio = clamp(p[compRatio], 1.0, 20.0);
        const double compAttackCoeff = onePole(1000.0 / clamp((double)p[compAttack], 1.0, 250.0), sr);
        const double compReleaseCoeff = onePole(1000.0 / clamp((double)p[compRelease], 5.0, 500.0), sr);
        const double compMakeup = decibels(p[compMakeup]);

        std::array<double, eqBands> eqGain{};
        std::array<double, eqBands> eqFreq{};
        std::array<double, eqBands> eqCoeff{};
        for (int i = 0; i < eqBands; ++i) {
            eqGain[(size_t)i] = p[eqGain1 + i];
            eqFreq[(size_t)i] = p[eqFreq1 + i];
            eqCoeff[(size_t)i] = onePole(eqFreq[(size_t)i], sr);
        }
        const bool eqOn = std::any_of(eqGain.begin(), eqGain.end(), [](double g) { return std::abs(g) > 0.001; });
        const bool processing = satAmt > 0 || distAmt > 0 || airAmt > 0 || punchAmt > 0 || deessAmt > 0 || colorAmt > 0 || chorusAmt > 0 || compAmt > 0 || eqOn || wetAmt < 0.999 || std::abs(p[wetOutput]) > 0.001;

        if (!processing) return;

        const double wetTrim = decibels(p[wetOutput]);
        for (int ch = 0; ch < number; ++ch) {
            float* data = channels[ch];
            if (!data) continue;
            Channel& s = states[(size_t)ch];
            for (int i = 0; i < samples; ++i) {
                const double dry = std::isfinite(data[i]) ? clamp(data[i], -32.0, 32.0) : 0.0;
                double x = dry;

                if (satAmt > 0.0) {
                    s.satToneLP += satToneCoeff * (x - s.satToneLP);
                    const double bright = x - s.satToneLP;
                    const double coloured = (s.satToneLP * (1.15 - p[satTone] / 100.0)) + (bright * (0.65 + p[satTone] / 100.0));
                    const double driven = softclip(coloured * (1.0 + satAmt * satDrive * 4.0));
                    x = x * (1.0 - 0.78 * satAmt) + driven * (0.78 * satAmt);
                    x *= satOut;
                }

                if (distAmt > 0.0) {
                    s.hp += distHpCoeff * (x - s.hp);
                    const double hpSignal = x - s.hp;
                    s.lp += distLpCoeff * (hpSignal - s.lp);
                    const double driven = softclip(s.lp * (1.0 + distAmt * distDrive * 10.0));
                    x = x * (1.0 - 0.82 * distAmt) + driven * (0.82 * distAmt);
                    x *= distOut;
                }

                if (airAmt > 0.0) {
                    s.airLP += airCoeff * (x - s.airLP);
                    const double bright = x - s.airLP;
                    x += bright * airAmt * airBoost;
                }

                if (punchAmt > 0.0) {
                    const double amplitude = std::abs(x);
                    s.fast += punchAttackCoeff * (amplitude - s.fast);
                    s.slow += punchReleaseCoeff * (amplitude - s.slow);
                    const double transient = clamp((s.fast - s.slow) * 7.5, -0.55, 2.3);
                    x *= 1.0 + punchAmt * transient;
                }

                if (compAmt > 0.0) {
                    const double level = std::max(1.0e-9, std::abs(x));
                    const double inDb = 20.0 * std::log10(level);
                    double overDb = inDb - compThreshold;
                    if (overDb < 0.0) overDb = 0.0;
                    const double compressedDb = overDb - (overDb / compRatio);
                    const double targetReduction = -compressedDb;
                    const double coeff = (targetReduction < s.compEnv ? compAttackCoeff : compReleaseCoeff);
                    s.compEnv += coeff * (targetReduction - s.compEnv);
                    const double gain = decibels(s.compEnv * compAmt) * compMakeup;
                    x *= gain;
                }

                if (deessAmt > 0.0) {
                    s.deessLP += deessCoeff * (x - s.deessLP);
                    const double high = x - s.deessLP;
                    s.deessEnvelope += deessEnvCoeff * (std::abs(high) - s.deessEnvelope);
                    const double reduction = deessAmt * deessExtra * clamp((s.deessEnvelope - 0.008) * 24.0, 0.0, 0.92);
                    x = s.deessLP + high * (1.0 - reduction);
                }

                if (colorAmt > 0.0) {
                    s.colorLow += colorLowCoeff * (x - s.colorLow);
                    s.colorMid += colorMidCoeff * (x - s.colorMid);
                    const double lows = s.colorLow;
                    const double mids = s.colorMid - s.colorLow;
                    const double highs = x - s.colorMid;
                    x += colorAmt * ((lows - highs) * colorTilt * 0.95 + mids * (colorBias - 0.5) * 0.55 + highs * 0.15);
                }

                if (eqOn) {
                    std::array<double, eqBands> parts{};
                    for (int b = 0; b < eqBands; ++b) {
                        s.eqLP[(size_t)b] += eqCoeff[(size_t)b] * (x - s.eqLP[(size_t)b]);
                    }
                    parts[0] = s.eqLP[0];
                    for (int b = 1; b < eqBands; ++b)
                        parts[(size_t)b] = s.eqLP[(size_t)b] - s.eqLP[(size_t)b - 1];
                    double sum = 0.0;
                    for (int b = 0; b < eqBands; ++b) {
                        const double g = decibels(eqGain[(size_t)b]);
                        sum += parts[(size_t)b] * g;
                    }
                    sum += (x - s.eqLP[eqBands - 1]);
                    x = sum;
                }

                if (chorusAmt > 0.0 && s.delay.size() > 8) {
                    const size_t size = s.delay.size();
                    const double mod = 1.0 + std::sin(s.chorusPhase) * chorusWidth;
                    const double delayMs = 11.0 + chorusDepth * 18.0 * mod;
                    const double offset = clamp(delayMs * 0.001 * sr, 1.0, (double)size - 3.0);
                    const double readPos = (double)s.index + (double)size - offset;
                    const size_t ix = (size_t)readPos % size;
                    const size_t ix2 = (ix + 1) % size;
                    const double frac = readPos - std::floor(readPos);
                    const double delayed = (1.0 - frac) * s.delay[ix] + frac * s.delay[ix2];
                    s.delay[s.index] = (float)x;
                    s.index = (s.index + 1) % size;
                    s.chorusPhase += chorusPhaseStep;
                    if (s.chorusPhase >= 6.283185307179586)
                        s.chorusPhase -= 6.283185307179586;
                    x = x * (1.0 - 0.6 * chorusAmt) + delayed * (0.6 * chorusAmt);
                }

                const double mixed = (dry * (1.0 - wetAmt) + x * wetAmt) * wetTrim;
                data[i] = (float)clamp(std::isfinite(mixed) ? mixed : 0.0, -32.0, 32.0);
            }
        }
    }

private:
    double sr = 44100.0;
    int usedChannels = 2;
    std::array<Channel, 2> states;
};
} // namespace SauceDSP
