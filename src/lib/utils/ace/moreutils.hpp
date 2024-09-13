#include <stdint.h>
#include <stdio.h>

/**
 * Small helper class to do something every XX
 * T: Typename to use, for seconds an int8_t is enough
 * every: Every XX the event should run
 * wrap:  If the counter wraps, like with seconds the maximum wrap needs to be set
 * It's the task of the caller to call this function regularly
*/
template<typename T, int every, int wrap>
class Every
{
    T lastOne;
    bool isTime;
public:
    Every(T lastOne_) : lastOne(lastOne_), isTime(false) {};

    bool isItTime(T t)
    {
        isTime |= (t - lastOne + wrap) % wrap == every;
        if (isTime)
        {
            isTime = false;
            lastOne = t;
            return true;
        }
        return false;
    };

    void reset(T t)
    {
        isTime = false;
        lastOne = t;
    }
};

/**
 * Keep track of the state of a primitive variable and ensures that even for the first the time state is set to 'changed'
 *
*/
template<typename StateType>
class StateHolder
{
    enum
    {
        START,
        RUNNING
    } status;
    StateType state;
public:
    StateHolder(StateType start_) : status(START), state(start_) {};

    bool isChanged(StateType thisState)
    {
        auto changed = (status == START) || (state != thisState);
        status = RUNNING;
        state = thisState;
        return changed;
    }

};


/**
 * Dump the buffer as HEX to the terminal
*/
void print_buffer(uint8_t *buffer, uint8_t length);

template<typename T>
inline void printBuffer(T *buffer, uint8_t length, const char format[]="%0X")
{
    printf("Length(%d) ", length);
    for (uint8_t i = 0; i < length; i++)
    {
        printf(format, buffer[i]);
        if (i < length-1)
        {
            printf(" ");
        }
    }
}
