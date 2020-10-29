#include <Audio.h>
#include "ValueInterface.h"
#include <vector>

class AudioInfra
{
public:
    void begin()
    {
        createParamLists();
        AudioMemory(10); // 10 * 256 * 2 bytes
        sgtl5000_1.enable();
        sgtl5000_1.volume(0.6);

        patchCord1 = new AudioConnection(waveform1, 0, filter1, 0);
        patchCord2 = new AudioConnection(filter1, 0, outputGain, 0);
        patchCord3 = new AudioConnection(outputGain, 0, i2s1, 0);
        patchCord4 = new AudioConnection(outputGain, 0, i2s1, 1);

        waveform1.begin(1.0, 440, WAVEFORM_SAWTOOTH);
        filter1.frequency(500);
        filter1.resonance(1.6);
        filter1.octaveControl(3);
        outputGain.gain(0.8);
    }

    void handleNoteOn(byte channel, byte note, byte velocity)
    {
        float frequency = pow(2.0, (note - 69.0) / 12.0) * 440.0;
        waveform1.frequency(frequency);
    }

    //encoder 2
    void updateFilterList()
    {
        filter1.frequency(paramLists[2][0].value);
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
        case 2:
            updateFilterList();
        case 7:
            updateGlobalList();
            break;
        default:
            break;
        }
    }

private:
    AudioControlSGTL5000 sgtl5000_1;
    AudioSynthWaveform waveform1;
    AudioFilterStateVariable filter1;
    AudioAmplifier outputGain;
    AudioOutputI2S i2s1;

    AudioConnection *patchCord1;
    AudioConnection *patchCord2;
    AudioConnection *patchCord3;
    AudioConnection *patchCord4;

    std::vector<Value> paramLists[8];
    char paramListsPointers[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    void createParamLists()
    {

        //WAVEFORM1 -----------------------------------------------------

        //WAVEFORM2 -----------------------------------------------------
        //FILTER --------------------------------------------------------
        Value filterFrequency(100, 10000, 7000, "FREQUENCY", 100, true);
        paramLists[2].push_back(filterFrequency);
        //FILTER ENVELOPE -----------------------------------------------
        //AMP ENVELOPE --------------------------------------------------
        //FX ------------------------------------------------------------
        //LFO -----------------------------------------------------------
        //GLOBAL --------------------------------------------------------
        Value globalVolume(0, 1.0, 0.3, "VOLUME", 100);
        paramLists[7].push_back(globalVolume);
    }
};
