#include <Audio.h>
#include "ValueInterface.h"
#include <vector>
#include "Envelope.h"
#define NOTE_INTERVAL 1.0594630943592953
struct OscillatorState_t
{
    float semitone;
    float detune;
    char note;
    float amplitude;
    float wf;

    void setState(char _note, float _semitone, float _detune, float _wf, float _amplitude = 0)
    {
        note = _note;
        semitone = _semitone;
        detune = _detune;
        amplitude = _amplitude;
        wf = _wf;
    }

    void setNote(char _note)
    {
        note = _note;
    }
};

void setupWaveShaper(float *curve, int buffer_length, float amount)
{
    float deg = PI / 180.0f;
    float x;
    for (int i = 0; i < buffer_length; ++i)
    {
        x = (float)i * 2.0f / buffer_length - 1.0f;
        curve[i] = (3.0 + amount) * x * 20.0 * deg / (PI + amount * abs(x));
    }
}

class AudioInfra
{
public:
    void begin()
    {
        createParamLists();
        AudioMemory(130);
        sgtl5000_1.enable();
        sgtl5000_1.volume(0.6);

        patchCord1 = new AudioConnection(waveform1, 0, mixer1, 0); // oscilador 1 para o mixer
        patchCord2 = new AudioConnection(waveform2, 0, mixer1, 1); // oscilador 2 para o mixer

        patchCord3 = new AudioConnection(fltEnv, 0, filterModulation, 0);  // envelope do filtro oara o controle de modulação
        patchCord4 = new AudioConnection(filterModulation, 0, filter1, 1); // controle de modulação controlando o cutoff do filtro
        patchCord5 = new AudioConnection(mixer1, 0, filter1, 0);           // sinal entrando no filtro

        patchCord6 = new AudioConnection(filter1, 0, filterSelector, 0); // passa baixas
        patchCord7 = new AudioConnection(filter1, 1, filterSelector, 1); // passa banda
        patchCord8 = new AudioConnection(filter1, 2, filterSelector, 2); // passa altas

        patchCord9 = new AudioConnection(filterSelector, 0, ampMultiply, 0); // sinal entrando na multiplicação
        patchCord10 = new AudioConnection(ampEnv, 0, ampMultiply, 1);        // envelope de amplitude entrando na multiplicação

        patchCord11 = new AudioConnection(ampMultiply, distortion);
        
        patchCord12 = new AudioConnection(distortion, 0, delayDryWetMixer, 0);

        patchCord12 = new AudioConnection(distortion, 0, delayMixer, 0);
        patchCord13 = new AudioConnection(delayMixer, delayFx);
        patchCord14 = new AudioConnection(delayFx, 0, delayMixer, 1);
        patchCord15 = new AudioConnection(delayFx, 0, delayDryWetMixer, 1);

        patchCord16 = new AudioConnection(delayDryWetMixer, 0, outputGain, 0);
        patchCord17 = new AudioConnection(outputGain, 0, i2s1, 0);       // indo para o DAC
        patchCord18 = new AudioConnection(outputGain, 0, i2s1, 1);
        patchCord19 = new AudioConnection(outputGain, 0, usbOut, 0); // audio na USB
        patchCord20 = new AudioConnection(outputGain, 0, usbOut, 1);

        waveform1.begin(1.0, 440, WAVEFORM_SAWTOOTH);
        waveform2.begin(0.0, 440, WAVEFORM_SAWTOOTH);
        filter1.frequency(500);
        filter1.resonance(1.6);
        filter1.octaveControl(3);
        outputGain.gain(0.8);
        delayFx.delay(0, 200);
        delayMixer.gain(1, 0.7);
        for (byte i = 0; i < 8; i++) // configurando todos os parametros de acordo com a param list
            updateIList(i);
    }

    void changeWaveform1Frequency(byte note, float semitones, float detune)
    {
        wf1State.setNote(note);
        float frequency = pow(2.0, (note - 69.0) / 12.0) * 440.0 * pow(NOTE_INTERVAL, semitones + detune);
        waveform1.frequency(frequency);
    }

    void changeWaveform2Frequency(byte note, float semitones, float detune) // pelo amor de deus faz ser uma função só essa e a de cima
    {
        wf2State.setNote(note);
        float frequency = pow(2.0, (note - 69.0) / 12.0) * 440.0 * pow(NOTE_INTERVAL, semitones + detune);
        waveform2.frequency(frequency);
    }

    void handleNoteOn(byte channel, byte note, byte velocity)
    {
        changeWaveform1Frequency(note, wf1State.semitone, wf1State.detune);
        changeWaveform2Frequency(note, wf2State.semitone, wf2State.detune);
        ampEnv.noteOn();
        fltEnv.noteOn();
    }

    void handleNoteOff()
    {
        ampEnv.noteOff();
        fltEnv.noteOff();
    }

    //encoder 0
    void updateWaveform1List()
    {
        float semitones = paramLists[0][0].value;
        float detune = paramLists[0][1].value;
        char wf = (char)paramLists[0][2].value;
        // Serial.printf("note: %d, semitones: %f, detune: %f, wf: %f\n", note, semitones, detune, wf);
        wf1State.setState(lastNote, semitones, detune, wf, 1);
        switch (wf)
        {
        case 0:
            waveform1.begin(WAVEFORM_TRIANGLE);
            break;
        case 1:
            waveform1.begin(WAVEFORM_SAWTOOTH);
            break;
        case 2:
            waveform1.begin(WAVEFORM_SQUARE);
            break;
        // case 3:
        //     waveform1.begin(SPECIAL_WF);
        //     break;
        default:
            break;
        }
        // lastNote = note == 0 ? lastNote : note;
        changeWaveform1Frequency(lastNote, semitones, detune);
    }

    //encoder1
    void updateWaveform2List()
    {
        float semitones = paramLists[1][0].value;
        float detune = paramLists[1][1].value;
        char wf = (char)paramLists[1][2].value;
        float amplitude = paramLists[1][3].value;
        wf2State.setState(lastNote, semitones, detune, wf, amplitude);
        Serial.printf("semitones=%f detune=%f amplitude=%f\n", semitones, detune, amplitude);
        switch (wf)
        {
        case 0:
            waveform2.begin(WAVEFORM_TRIANGLE);
            break;
        case 1:
            waveform2.begin(WAVEFORM_SAWTOOTH);
            break;
        case 2:
            waveform2.begin(WAVEFORM_SQUARE);
            break;
        // case 3:
        //     waveform2.begin(SPECIAL_WF);
        default:
            break;
        }
        changeWaveform2Frequency(lastNote, semitones, detune);
        waveform2.amplitude(amplitude);
    }

    void changeFilterType(byte t)
    {
        filterSelector.gain(0, (float)(t == 0));
        filterSelector.gain(1, (float)(t == 1));
        filterSelector.gain(2, (float)(t == 2));
    }
    //encoder 2
    void updateFilterList()
    {
        filter1.frequency(paramLists[2][0].value);
        filter1.resonance(paramLists[2][1].value);
        changeFilterType((byte)paramLists[2][2].value);
        filterModulation.gain(0, paramLists[2][3].value);
    }

    //encoder 3
    void updateFilterEvelopeList()
    {
        fltEnv.setCoefficients(paramLists[3][1].value, 0.001, paramLists[3][0].value, paramLists[3][2].value, paramLists[3][3].value);
    }

    //encoder 4
    void updateAmpEvelopeList()
    {
        ampEnv.setCoefficients(paramLists[4][1].value, 0.001, paramLists[4][0].value, paramLists[4][2].value, paramLists[4][3].value);
    }

    void setDelayDryWet(float amount)
    {
        delayDryWetMixer.gain(0, 1 - amount);
        delayDryWetMixer.gain(1, amount);
    }

    void updateEffectsList()
    {
        setupWaveShaper(distCurve, 129, paramLists[5][0].value);
        distortion.shape(distCurve, 129);

        delayFx.delay(0, (int)paramLists[5][1].value); // tempo de delay (tamanho do buffer de delay)
        setDelayDryWet(paramLists[5][2].value); // dry wet
        delayMixer.gain(1, paramLists[5][3].value); // multiplicador do sinal de feedback
    }
    //encoder 7
    void updateGlobalList()
    {

        outputGain.gain(paramLists[7][0].value);
    }

    Value *getCurrentValue(char i)
    {
        //find current value in i-th list and return its pointer
        return &paramLists[i][paramListsPointers[i]];
    }

    Value *getNextValue(char i)
    {
        // increment list value pointer and return it
        paramListsPointers[i] = (paramListsPointers[i] + 1) % paramLists[i].size();
        return &paramLists[i][paramListsPointers[i]];
    }

    void updateIList(char i)
    {
        switch (i)
        {
        case 0: // bloco do oscilador 1
            updateWaveform1List();
            break;
        case 1: // bloco do oscilador 2
            updateWaveform2List();
            break;
        case 2: // bloco do filtro
            updateFilterList();
            break;
        case 3: // bloco do envelope do filtro
            updateFilterEvelopeList();
            break;
        case 4: // bloco do envelope da amplitude
            updateAmpEvelopeList();
            break;
        case 5:
            updateEffectsList();
            break;
        case 7: // bloco do volume total
            updateGlobalList();
            break;
        default:
            break;
        }
    }

private:
    //state management
    OscillatorState_t wf1State;
    OscillatorState_t wf2State;
    int lastNote = 69;
    float distCurve[129];

    //audio blocks
    AudioControlSGTL5000 sgtl5000_1;
    AudioSynthWaveform waveform1;
    AudioSynthWaveform waveform2;
    Envelope ampEnv;
    Envelope fltEnv;
    AudioMixer4 mixer1;
    AudioMixer4 filterSelector;
    AudioMixer4 filterModulation;
    AudioFilterStateVariable filter1;
    AudioAmplifier outputGain;
    AudioEffectMultiply ampMultiply;
    AudioEffectWaveshaper distortion;
    AudioMixer4 delayDryWetMixer;
    AudioMixer4 delayMixer;
    AudioEffectDelay delayFx;
    AudioOutputI2S i2s1;
    AudioOutputUSB usbOut;

    //audio connections
    AudioConnection *patchCord1;
    AudioConnection *patchCord2;
    AudioConnection *patchCord3;
    AudioConnection *patchCord4;
    AudioConnection *patchCord5;
    AudioConnection *patchCord6;
    AudioConnection *patchCord7;
    AudioConnection *patchCord8;
    AudioConnection *patchCord9;
    AudioConnection *patchCord10;
    AudioConnection *patchCord11;
    AudioConnection *patchCord12;
    AudioConnection *patchCord13;
    AudioConnection *patchCord14;
    AudioConnection *patchCord15;
    AudioConnection *patchCord16;
    AudioConnection *patchCord17;
    AudioConnection *patchCord18;
    AudioConnection *patchCord19;
    AudioConnection *patchCord20;

    //infra objects
    std::vector<Value> paramLists[8];
    char paramListsPointers[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    void createParamLists()
    {

        //WAVEFORM1 -----------------------------------------------------
        Value wf1SemitoneValue(-48.0, 48.0, 0.0, "SEMITONE", 96);
        paramLists[0].push_back(wf1SemitoneValue);

        Value wf1Detune(-1.0, 1.0, 0.0, "DETUNE", 24);
        paramLists[0].push_back(wf1Detune);

        Value wf1Waveform(0.0, 3.0, 1.0, "WAVEFORM", 3);
        paramLists[0].push_back(wf1Waveform);

        //WAVEFORM2 -----------------------------------------------------
        Value wf2SemitoneValue(-48.0, 48.0, 0.0, "SEMITONE", 96);
        paramLists[1].push_back(wf2SemitoneValue);

        Value wf2Detune(-1.0, 1.0, 0.0, "DETUNE", 24);
        paramLists[1].push_back(wf2Detune);

        Value wf2Waveform(0.0, 3.0, 1.0, "WAVEFORM", 3);
        paramLists[1].push_back(wf2Waveform);

        Value wf2Level(0.0, 1.0, 0.0, "LEVEL", 15);
        paramLists[1].push_back(wf2Level);

        //FILTER --------------------------------------------------------
        Value filterFrequency(55.0, 10000, 7000, "FREQUENCY", 100, true);
        paramLists[2].push_back(filterFrequency);

        Value fltQValue(0.707, 5.0, 0.707, "RESONANCE", 20);
        paramLists[2].push_back(fltQValue);

        Value fltTypeValue(0.0, 2.0, 0.0, "TYPE", 2);
        paramLists[2].push_back(fltTypeValue);

        Value fltEnvModValue(0.0, 1.0, 0.5, "ENVMOD", 25);
        paramLists[2].push_back(fltEnvModValue);

        // FILTER ENVELOPE ----------------------------------------------
        Value fltEnvAttackValue(1.0, 200.0, 1.0, "ATTACK", 30, true);
        paramLists[3].push_back(fltEnvAttackValue);

        Value fltEnvDecayValue(1.0, 3000.0, 100.0, "DECAY", 70, true);
        paramLists[3].push_back(fltEnvDecayValue);

        Value fltEnvSustainValue(0.001, .99, 0.7, "SUSTAIN", 80);
        paramLists[3].push_back(fltEnvSustainValue);

        Value fltEnvReleaseValue(1.0, 3000.0, 100.0, "RELEASE", 70, true);
        paramLists[3].push_back(fltEnvReleaseValue);

        // AMP ENVELOPE --------------------------------------------------
        Value ampEnvAttackValue(1.0, 200.0, 1.0, "ATTACK", 30, true);
        paramLists[4].push_back(ampEnvAttackValue);

        Value ampEnvDecayValue(1.0, 3000.0, 100.0, "DECAY", 70, true);
        paramLists[4].push_back(ampEnvDecayValue);

        Value ampEnvSustainValue(0.001, 0.99, 0.7, "SUSTAIN", 80);
        paramLists[4].push_back(ampEnvSustainValue);

        Value ampEnvReleaseValue(1.0, 3000.0, 100.0, "RELEASE", 70, true);
        paramLists[4].push_back(ampEnvReleaseValue);

        //FX ------------------------------------------------------------
        Value fxDist(0, 100.0, 0.0, "DISTORTION", 100);
        paramLists[5].push_back(fxDist);

        Value fxDelayTime(0, 500.0, 120.0, "DELAY TIME", 100);
        paramLists[5].push_back(fxDelayTime);

        Value fxDelayDryWet(0, 1.0, 0.0, "DEL DRYWET", 100);
        paramLists[5].push_back(fxDelayDryWet);

        Value fxDelayFeedback(0, 1.0, 0.65, "DEL FEEDBACK", 100);
        paramLists[5].push_back(fxDelayFeedback);

        //LFO -----------------------------------------------------------
        //GLOBAL --------------------------------------------------------
        Value globalVolume(0, 1.0, 0.3, "VOLUME", 100);
        paramLists[7].push_back(globalVolume);
    }

    void noteOff()
    {
        ampEnv.noteOff();
        fltEnv.noteOff();
    }
};
