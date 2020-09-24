#define S0 4
#define S1 5
#define S2 6
#define E_A 17
#define E_B 20
#define E_C 21

//functions for functional allocation
namespace std
{
    void __throw_bad_alloc()
    {
        Serial.println("Unable to allocate memory");
    }

    void __throw_length_error(char const *e)
    {
        Serial.print("Length Error :");
        Serial.println(e);
    }

    void __throw_bad_function_call()
    {
        Serial.println("Bad function call!");
    }
} // namespace std

void sendBits(byte i)
{
    // digitalWrite(4, 0);
    // digitalWrite(5, 0);
    // digitalWrite(6, 0);
    digitalWrite(S0, i & 0b001);
    digitalWrite(S1, i & 0b010);
    digitalWrite(S2, i & 0b100);
}

void setupPorts()
{
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(E_A, INPUT);
    pinMode(E_B, INPUT);
    pinMode(E_C, INPUT);
}