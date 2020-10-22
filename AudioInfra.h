#include <Audio.h>

class AudioInfra
{
public:
    void begin()
    {
        AudioMemory(40); // 10 * 256 * 2 bytes
        sgtl5000_1.enable();
        sgtl5000_1.volume(0.6);

        patchCord1 = new AudioConnection(waveform1, outputGain);
        patchCord2 = new AudioConnection(outputGain, 0, i2s1, 0);
        patchCord3 = new AudioConnection(outputGain, 0, i2s1, 1);

        waveform1.begin(1.0, 440, WAVEFORM_SAWTOOTH);
        outputGain.gain(0.8);
    }

    void handleNoteOn(byte channel, byte note, byte velocity)
    {
        float frequency = pow(2.0, (note - 69.0) / 12.0) * 440.0;
        waveform1.frequency(frequency);
    }

    void setVolume(float vol)
    {
        outputGain.gain(vol);
    }

private:
    AudioControlSGTL5000 sgtl5000_1;
    AudioSynthWaveform waveform1;
    AudioAmplifier outputGain;
    AudioOutputI2S i2s1;

    AudioConnection *patchCord1;
    AudioConnection *patchCord2;
    AudioConnection *patchCord3;
};
