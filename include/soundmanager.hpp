#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <iostream>

ma_engine initializeSoundEngine()
{
    ma_engine engine;
    ma_result result;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS)
        std::cerr << "Error: sound engine failed to initialize\n";

    return engine;
}