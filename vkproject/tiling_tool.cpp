#include <iostream>
#include <vector>
#include <inttypes.h>

using namespace std; 

//////////////////////////////////////////////////////////////////////////////
// Below is the code used to figure out which types of
// walls we really need to draw using our 2d marching         0 1 2
// cubeish approach. A tile will be drawn in a special        3   4
// way depending on which of the surrounding tiles are        5 6 7
// set. The tiles are ordered as to the left.
//////////////////////////////////////////////////////////////////////////////
unsigned char GetBit(unsigned char val, uint32_t i) { return ((val >> i) & 0x1) != 0; }
unsigned char UnSetBit(unsigned char &val, uint32_t i)
{
    val &= ~(1 << i);
    return val;
}
unsigned char SwapBits(unsigned char val, uint32_t i, uint32_t j)
{
    unsigned int x = ((val >> i) ^ (val >> j)) & 0x1U;
    return val ^ ((x << i) | (x << j));
}
unsigned char Rotate(unsigned char val, uint32_t times)
{
    if (times == 0) return val;
    unsigned char r = val;
    r = SwapBits(r, 0, 2);
    r = SwapBits(r, 0, 7);
    r = SwapBits(r, 0, 5);
    r = SwapBits(r, 1, 4);
    r = SwapBits(r, 1, 6);
    r = SwapBits(r, 1, 3);
    return Rotate(r, times - 1);
}
unsigned char FlipX(unsigned char val)
{
    unsigned char r = val;
    r = SwapBits(r, 0, 2);
    r = SwapBits(r, 3, 4);
    r = SwapBits(r, 5, 7);
    return r;
}
unsigned char FlipY(unsigned char val)
{
    unsigned char r = val;
    r = SwapBits(r, 0, 5);
    r = SwapBits(r, 1, 6);
    r = SwapBits(r, 2, 7);
    return r;
}
unsigned char FlipXAndY(unsigned char val) { return FlipY(FlipX(val)); }
unsigned char RemoveFreeCorners(unsigned char val)
{
    if (!GetBit(val, 1) || !GetBit(val, 3)) UnSetBit(val, 0);
    if (!GetBit(val, 1) || !GetBit(val, 4)) UnSetBit(val, 2);
    if (!GetBit(val, 4) || !GetBit(val, 6)) UnSetBit(val, 7);
    if (!GetBit(val, 6) || !GetBit(val, 3)) UnSetBit(val, 5);
    return val;
}


int main() 
{
    // These are the tiles I want to have to draw
    // +---+ +---+ +---+ +---+ +---+  
    // |   | |   | | 1 | | 1 | |   | 
    // | 1 | |11 | |11 | |11 | |111| 
    // |   | |   | |   | | 1 | |   | 
    // +---+ +---+ +---+ +---+ +---+  
    // +---+ +---+ +---+ +---+ +---+ 
    // |11 | |11 | |11 | | 1 | | 1 | 
    // |11 | |11 | |11 | |11 | |111| 
    // |   | | 1 | |11 | |11 | | 1 | 
    // +---+ +---+ +---+ +---+ +---+ 
    // +---+ +---+ +---+ +---+ +---+
    // |11 | |111| |111| |111| | 11|
    // |111| |111| |111| |111| |111|
    // | 1 | | 1 | |11 | |111| |11 |
    // +---+ +---+ +---+ +---+ +---+ 
 
    vector<uint8_t> available_tiles = {0b00000000, 0b00010000, 0b01010000, 0b01010010, 0b00011000,
                                       0b11010000, 0b11010010, 0b11010110, 0b01010110, 0b01011010,
                                       0b11011010, 0b11111010, 0b11111110, 0b11111111, 0b01111110};
    // It was easier to figure these out in reverse order, so reverse the order: 
    for (auto &tile : available_tiles)
    {
        uint8_t reversed = 0xFF;
        for (int i = 0; i < 8; i++)
        {
            if (!GetBit(tile, i)) UnSetBit(reversed, 7 - i);
        }
        tile = reversed; 
    }
    // Now just brute force through all 256 possible variations and find out if I 
    // can achieve each by rotating one of the known tiles. 
    for (uint32_t i = 0; i < 256; i++)
    {
        bool found = false;
        int tile_to_use = -1;
        int num_rotations = -1;
        for (uint32_t t = 0; t < available_tiles.size(); t++)
        {
            for (uint32_t r = 0; r < 4; r++)
            {
                uint8_t rotated = Rotate(available_tiles[t], r);
                if (rotated == RemoveFreeCorners(i))
                {
                    tile_to_use = t;
                    num_rotations = r;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (found)
        {
        }  // cout << hex << i << " = " << (uint32_t)available_tiles[tile_to_use] << ", rotated " << num_rotations << "
           // times.\n";
        else
            cout << hex << i << ": not found.\n";
    }

	return 0; 
}