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

class AudioInfra
{
public:
    void begin()
    {
        createParamLists();
        AudioMemory(130);
        sgtl5000_1.enable();
        sgtl5000_1.volume(0.6);

        patchCord1 = new AudioConnection(waveform1, 0, mixer1, 0);
        patchCord2 = new AudioConnection(waveform2, 0, mixer1, 1);

        patchCord3 = new AudioConnection(fltEnv, 0, filterModulation, 0);
        patchCord4 = new AudioConnection(filterModulation, 0, filter1, 1);
        patchCord5 = new AudioConnection(mixer1, 0, filter1, 0);

        patchCord6 = new AudioConnection(filter1, 0, filterSelector, 0);
        patchCord7 = new AudioConnection(filter1, 1, filterSelector, 1);
        patchCord8 = new AudioConnection(filter1, 2, filterSelector, 2);


        patchCord9 = new AudioConnection(filterSelector, 0, ampMultiply, 0);
        patchCord10 = new AudioConnection(ampEnv, 0, ampMultiply, 1);

        patchCord11 = new AudioConnection(ampMultiply, 0, outputGain, 0);
        patchCord12 = new AudioConnection(outputGain, 0, i2s1, 0);
        patchCord13 = new AudioConnection(outputGain, 0, i2s1, 1);
        patchCord14 = new AudioConnection(outputGain, 0, usbOut, 0);
        patchCord15 = new AudioConnection(outputGain, 0, usbOut, 1);

        waveform1.begin(1.0, 440, WAVEFORM_SAWTOOTH);
        waveform2.begin(0.0, 440, WAVEFORM_SAWTOOTH);
        filter1.frequency(500);
        filter1.resonance(1.6);
        filter1.octaveControl(3);
        outputGain.gain(0.8);
        for(byte i = 0; i < 8; i++)
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

    void handleNoteOff() {
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
        case 0:
            updateWaveform1List();
            break;
        case 1:
            updateWaveform2List();
            break;
        case 2:
            updateFilterList();
            break;
        case 3:
            updateFilterEvelopeList();
            break;
        case 4:
            updateAmpEvelopeList();
            break;
        case 7:
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
        //LFO -----------------------------------------------------------
        //GLOBAL --------------------------------------------------------
        Value globalVolume(0, 1.0, 0.3, "VOLUME", 100);
        paramLists[7].push_back(globalVolume);
    }

    void noteOff() {
        ampEnv.noteOff();
        fltEnv.noteOff();
    }
};
