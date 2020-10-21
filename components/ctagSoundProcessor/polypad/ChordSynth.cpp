#include "ChordSynth.hpp"
#include <cstring>
#include "stmlib/utils/random.h"

void CTAG::SP::ChordSynth::NoteOff() {
    adsr.Gate(false);
}

void CTAG::SP::ChordSynth::SetCutoff(const uint32_t &cutoff) {
    params_.filter_freq = cutoff;
    svf.set_frequency(cutoff);
}

void CTAG::SP::ChordSynth::SetResonance(const uint32_t &resonance) {
    params_.filter_reso = resonance;
    svf.set_resonance(resonance);
}

void IRAM_ATTR CTAG::SP::ChordSynth::Process(float *buf, const uint32_t& ofs) {
    memset(buffer, 0, 32*2);

    // vibrato and render buffer
    int i=0;
    for(auto& osc:v_osc){
        osc.SetPitch(params_.pitch + scale[i++] * 128 + static_cast<int16_t>(lfo1.Process() * params_.lfo1_amt * 128.f));
        osc.Render(buffer, 32);
    }

    // apply filter with lfo and eg
    float eg = adsr.Process();
    float ffreq = static_cast<float>(params_.filter_freq); // base
    ffreq += params_.eg_filt_amt * eg * 16383.f; // eg
    ffreq += lfo2.Process() * params_.lfo2_amt * 16383.f; // lfo
    CONSTRAIN(ffreq, 0, 16383)

    svf.set_frequency(static_cast<uint16_t>(ffreq));
    for(int i=0;i<32;i++){
        buffer[i] = svf.Process(buffer[i]);
    }

    for(int i=0;i<32;i++){
        buf[i*2 + ofs] += static_cast<float>(buffer[i]>>1) / 32767.f * eg;
    }
}

bool CTAG::SP::ChordSynth::IsDead() {
    return adsr.IsIdle();
}

CTAG::SP::ChordSynth::ChordSynth(const ChordParams& params) {
    params_ = params;
    stmlib::Random::Seed(esp_random());
    lfo1.SetSampleRate(44100.f / 32.f);
    lfo1.SetFrequencyPhase(params.lfo1_freq, 6.2f * stmlib::Random::GetFloat());
    lfo2.SetSampleRate(44100.f / 32.f);
    if(params.lfo2_random_phase)
        lfo2.SetFrequencyPhase(params.lfo2_freq, 6.2f * stmlib::Random::GetFloat());
    else
        lfo2.SetFrequencyPhase(params.lfo2_freq, 3.1415f);
    adsr.SetSampleRate(44100.f / 32.f);
    adsr.SetAttack(params.attack);
    adsr.SetDecay(params.decay);
    adsr.SetSustain(params.sustain);
    adsr.SetRelease(params.release);
    adsr.SetModeExp();
    adsr.Gate(true);
    svf.Init();
    svf.set_mode(static_cast<SvfMode>(params.filter_type));

    calcInversion(scale, params.chord, params.inversion, params.nnotes);

    for(int i=0;i<params.nnotes;i++){
        MiSuperSawOsc osc;
        osc.Init();
        osc.SetPitch(params.pitch + scale[i] * 128);
        v_osc.push_back(osc);
    }
}

void CTAG::SP::ChordSynth::SetFilterType(const SvfMode &mode) {
    mode_ = mode;
}

void CTAG::SP::ChordSynth::SetDetune(const uint32_t &detune) {
    for(auto& osc:v_osc){
        osc.SetDetune(detune);
    }
}

void IRAM_ATTR CTAG::SP::ChordSynth::calcInversion(int8_t *ht_steps, const int16_t &chord, const int16_t &inversion,
                                         const int16_t &nnotes) {
    int8_t inv[4];
    for(int i=0;i<4;i++){
        inv[i] = chords[chord][i+2+inversion];
    }
    memcpy(ht_steps, inv, 4);
}

float CTAG::SP::ChordSynth::GetTTL() {
    return adsr.GetOutput();
}

void CTAG::SP::ChordSynth::Hold() {
    adsr.Hold();
}
