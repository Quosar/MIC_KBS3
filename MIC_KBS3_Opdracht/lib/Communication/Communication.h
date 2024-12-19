#ifndef COMMUNICATION___H
#define COMMUNICATION___H

#include <avr/interrupt.h>
#include <avr/io.h>
#include <Snake.h>

class Communication {
public:
    volatile uint8_t communicationFrameCounter;
    volatile uint8_t communicationFrameCount;
    volatile bool runFrame;
    volatile bool runCommunicationFrame;
    volatile bool previousInBus;
    volatile uint32_t inBus;
    volatile uint32_t outBus;
    volatile uint8_t busBitIndex;
    volatile bool communicationInitialized;
    volatile bool printBus;
    volatile uint8_t snakeDirectionOther;
    volatile bool communicationNearlyInitialized;
    volatile bool gameRunning;
    volatile uint8_t posApple;
    volatile uint8_t posAppleOther;
    volatile bool appleGatheredByPlayer2;
    bool getSender();
    bool getRunFrame();
    void setupPins();
    void setupTimers();
    void SetupInterrupts();
    void initializeCommunication();
    uint32_t constructBus(Snake &snake);
    void deconstructBus(uint32_t bus, Snake &snake);
    void communicate();
private:
    uint8_t constructChecksum(uint32_t value);
};

#endif