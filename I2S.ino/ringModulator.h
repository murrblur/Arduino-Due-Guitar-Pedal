#include "effect.h"
#include <math.h>
#include <complex.h>
#include <iostream>



class RingModulator : public Effect
{
private:
  float Fc = 440;
  float Fs = 300;
  int bypass = 1;
public:
  void setFs(float Fs);
  void setFc(float Fc);
  void process_samples(float *input, float *output);
};
