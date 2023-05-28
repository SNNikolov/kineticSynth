#include "Sound.h"

using namespace yoss;
using namespace yoss::sound;


//-----------------------------------------------------------------------
std::vector<Sample> yoss::sound::ConvertBuffer_Int16ToSample(void* int16_buffer, int buffer_byte_size)
{
    auto buffer = (int16_t*)int16_buffer;
    auto max_val = (Sample)(((uint16_t)-1) / 2);
    int samples_num = buffer_byte_size / 2;

    std::vector<Sample> samples_vec;
    samples_vec.resize(samples_num);
    for (int i = 0; i < samples_num; i++)
        samples_vec[i] = (Sample)buffer[i] / max_val;
    
    return samples_vec;
}




