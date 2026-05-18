#pragma once
#include <vector>
///////////////////////////////////////////////////////////////////////////////
// Our own chosen name for all possible keys
///////////////////////////////////////////////////////////////////////////////
enum Keys
{
    W, A, S, D, ESCAPE, SPACE, SHIFT, L, NUM_KEYS
};


struct IInput
{
    virtual void Update() = 0; 
    virtual bool IsPressed(Keys key) = 0;
    virtual bool IsJustPressed(Keys key) = 0;
};

extern IInput* input; 