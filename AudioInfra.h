#include <Audio.h>

class AudioInfra
{
public:
    void begin()
    {
        AudioMemory(40); // 10 * 256 * 2 bytes
        sgtl5000_1.enable();
        sgtl5000_1.volume(0.6);

        patchCord1 = new AudioConnection(waveform1, 0, i2s1, 0);
        patchCord2 = new AudioConnection(waveform1, 0, i2s1, 1);

        waveform1.begin(1.0, 440, WAVEFORM_SAWTOOTH);
    }

    void handleNoteOn(byte channel, byte note, byte velocity)
    {
        float frequency = pow(2.0, (note - 69.0) / 12.0) * 440.0;
        waveform1.frequency(frequency);
    }

private:
    AudioControlSGTL5000 sgtl5000_1;
    AudioSynthWaveform waveform1;
    AudioOutputI2S i2s1;

    AudioConnection *patchCord1;
    AudioConnection *patchCord2;
};
