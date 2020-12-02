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
        // Creates parameters to be controlled by encoders
        createParamLists();

        // Setup configs
        AudioMemory(130);
        sgtl5000_1.enable();
        sgtl5000_1.volume(0.6);

        // Setup Audio Connections - the actual sound blocks and transformers
        // This is based on the Teensy Library
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

        patchCord12 = new AudioConnection(distortion, 0, outputGain, 0); // gain stage
        patchCord13 = new AudioConnection(outputGain, 0, i2s1, 0);       // indo para o DAC
        patchCord14 = new AudioConnection(outputGain, 0, i2s1, 1);
        patchCord15 = new AudioConnection(outputGain, 0, usbOut, 0); // audio na USB
        patchCord16 = new AudioConnection(outputGain, 0, usbOut, 1);

        // Setup starting parameters
        waveform1.begin(1.0, 440, WAVEFORM_SAWTOOTH);
        waveform2.begin(0.0, 440, WAVEFORM_SAWTOOTH);
        filter1.frequency(500);
        filter1.resonance(1.6);
        filter1.octaveControl(3);
        outputGain.gain(0.8);
        
        // Config all parameters based on parameters list
        for (byte i = 0; i < 8; i++) 
            updateIList(i);
    }

    void changeWaveform1Frequency(byte note, float semitones, float detune)
    {
        // Sets note in state
        wf1State.setNote(note);

        // Calculate frequency based on note, semitones and detune
        float frequency = pow(2.0, (note - 69.0) / 12.0) * 440.0 * pow(NOTE_INTERVAL, semitones + detune);

        // Sets frequency waveform
        waveform1.frequency(frequency);
    }

    void changeWaveform2Frequency(byte note, float semitones, float detune)
    {
        wf2State.setNote(note);
        float frequency = pow(2.0, (note - 69.0) / 12.0) * 440.0 * pow(NOTE_INTERVAL, semitones + detune);
        waveform2.frequency(frequency);
    }

    // Callback called when note is pressed
    void handleNoteOn(byte channel, byte note, byte velocity)
    {
        // Set frequencies
        changeWaveform1Frequency(note, wf1State.semitone, wf1State.detune);
        changeWaveform2Frequency(note, wf2State.semitone, wf2State.detune);

        // Turn on envelopr
        ampEnv.noteOn();
        fltEnv.noteOn();
    }

    // Callback called when note is released
    void handleNoteOff()
    {
        // Turn off envelopes
        ampEnv.noteOff();
        fltEnv.noteOff();
    }

    //encoder 0
    void updateWaveform1List()
    {
        // Gets relevant data from correct values in paramList
        float semitones = paramLists[0][0].value;
        float detune = paramLists[0][1].value;
        char wf = (char)paramLists[0][2].value;

        // Updates waveform state
        wf1State.setState(lastNote, semitones, detune, wf, 1);

        // Updates waveform format
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

        default:
            break;
        }

        // Updates waveform frequency based on new semitones and detune
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

    void updateEffectsList()
    {
        setupWaveShaper(distCurve, 129, paramLists[5][0].value);
        distortion.shape(distCurve, 129);
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
        // Maps each encoder to each update function

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
