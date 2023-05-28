#include "Instrument.h"

#include <iterator>
#include <algorithm>

using namespace yoss;
using namespace yoss::math;
using namespace yoss::sound;


//-----------------------------------------------------------------------
// Static defines, consts and vars

//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Static members

//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
Frequency Instrument::UnnormalizeFrequency(PartOfOne normalized_freq)
{
    ASSERT(normalized_freq >= 0 && normalized_freq <= 1);
    
    Frequency freq = BEAT_FUNDAMENTAL_MIN_FREQUENCY + normalized_freq * normalized_freq * (BEAT_FUNDAMENTAL_MAX_FREQUENCY - BEAT_FUNDAMENTAL_MIN_FREQUENCY);
    return freq;
}

