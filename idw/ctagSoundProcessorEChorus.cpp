#include "ctagSoundProcessorEChorus.hpp"
#include <cmath>

using namespace CTAG::SP;


void ctagSoundProcessorEChorus::Process(const ProcessData &data) {
    float overallscale = 1.0;

    float speed = pow((prate/4095.f),3) * 0.001;
    speed *= overallscale;
    int loopLimit = (int)(totalsamples * 0.499);
    int count;
    float range = pow((pdepth/4095.f),3) * loopLimit * 0.12;
    float wet = (pwet / 4095.f);
    float modulation = range*wet;
    float dry = 1.0 - wet;
    float tupi = 3.141592653589793238 * 2.0;
    float offset;
    float start[4];

    float inputSampleL;
    float inputSampleR;
    float drySampleL;
    float drySampleR;
    //now we'll precalculate some stuff that needn't be in every sample
    start[0] = range;
    start[1] = range * 2;
    start[2] = range * 3;
    start[3] = range * 4;

    for(int i=0;i<bufSz;i++)
    {
        inputSampleL = data.buf[i*2];
        inputSampleR = data.buf[i*2 + 1];
        if (inputSampleL<1.2e-38 && -inputSampleL<1.2e-38) {
            static int noisesource = 0;
            //this declares a variable before anything else is compiled. It won't keep assigning
            //it to 0 for every sample, it's as if the declaration doesn't exist in this context,
            //but it lets me add this denormalization fix in a single place rather than updating
            //it in three different locations. The variable isn't thread-safe but this is only
            //a random seed and we can share it with whatever.
            noisesource = noisesource % 1700021; noisesource++;
            int residue = noisesource * noisesource;
            residue = residue % 170003; residue *= residue;
            residue = residue % 17011; residue *= residue;
            residue = residue % 1709; residue *= residue;
            residue = residue % 173; residue *= residue;
            residue = residue % 17;
            float applyresidue = residue;
            applyresidue *= 0.00000001;
            applyresidue *= 0.00000001;
            inputSampleL = applyresidue;
        }
        if (inputSampleR<1.2e-38 && -inputSampleR<1.2e-38) {
            static int noisesource = 0;
            noisesource = noisesource % 1700021; noisesource++;
            int residue = noisesource * noisesource;
            residue = residue % 170003; residue *= residue;
            residue = residue % 17011; residue *= residue;
            residue = residue % 1709; residue *= residue;
            residue = residue % 173; residue *= residue;
            residue = residue % 17;
            float applyresidue = residue;
            applyresidue *= 0.00000001;
            applyresidue *= 0.00000001;
            inputSampleR = applyresidue;
            //this denormalization routine produces a white noise at -300 dB which the noise
            //shaping will interact with to produce a bipolar output, but the noise is actually
            //all positive. That should stop any variables from going denormal, and the routine
            //only kicks in if digital black is input. As a final touch, if you save to 24-bit
            //the silence will return to being digital black again.
        }
        drySampleL = inputSampleL;
        drySampleR = inputSampleR;

        airFactorL = airPrevL - inputSampleL;
        if (fpFlip) {airEvenL += airFactorL; airOddL -= airFactorL; airFactorL = airEvenL;}
        else {airOddL += airFactorL; airEvenL -= airFactorL; airFactorL = airOddL;}
        airOddL = (airOddL - ((airOddL - airEvenL)/256.0)) / 1.0001;
        airEvenL = (airEvenL - ((airEvenL - airOddL)/256.0)) / 1.0001;
        airPrevL = inputSampleL;
        inputSampleL += (airFactorL*wet);
        //air, compensates for loss of highs in flanger's interpolation

        airFactorR = airPrevR - inputSampleR;
        if (fpFlip) {airEvenR += airFactorR; airOddR -= airFactorR; airFactorR = airEvenR;}
        else {airOddR += airFactorR; airEvenR -= airFactorR; airFactorR = airOddR;}
        airOddR = (airOddR - ((airOddR - airEvenR)/256.0)) / 1.0001;
        airEvenR = (airEvenR - ((airEvenR - airOddR)/256.0)) / 1.0001;
        airPrevR = inputSampleR;
        inputSampleR += (airFactorR*wet);
        //air, compensates for loss of highs in flanger's interpolation

        if (gcount < 1 || gcount > loopLimit) {gcount = loopLimit;}
        count = gcount;
        dL[count+loopLimit] = dL[count] = inputSampleL;
        dR[count+loopLimit] = dR[count] = inputSampleR;
        gcount--;
        //float buffer

        offset = start[0] + (modulation * sin(sweep));
        count = gcount + (int)floor(offset);
        inputSampleL = dL[count] * (1-(offset-floor(offset))); //less as value moves away from .0
        inputSampleL += dL[count+1]; //we can assume always using this in one way or another?
        inputSampleL += (dL[count+2] * (offset-floor(offset))); //greater as value moves away from .0
        inputSampleL -= (((dL[count]-dL[count+1])-(dL[count+1]-dL[count+2]))/50); //interpolation hacks 'r us

        inputSampleR = dR[count] * (1-(offset-floor(offset))); //less as value moves away from .0
        inputSampleR += dR[count+1]; //we can assume always using this in one way or another?
        inputSampleR += (dR[count+2] * (offset-floor(offset))); //greater as value moves away from .0
        inputSampleR -= (((dR[count]-dR[count+1])-(dR[count+1]-dR[count+2]))/50); //interpolation hacks 'r us

        offset = start[1] + (modulation * sin(sweep + 1.0));
        count = gcount + (int)floor(offset);
        inputSampleL += dL[count] * (1-(offset-floor(offset))); //less as value moves away from .0
        inputSampleL += dL[count+1]; //we can assume always using this in one way or another?
        inputSampleL += (dL[count+2] * (offset-floor(offset))); //greater as value moves away from .0
        inputSampleL -= (((dL[count]-dL[count+1])-(dL[count+1]-dL[count+2]))/50); //interpolation hacks 'r us

        inputSampleR += dR[count] * (1-(offset-floor(offset))); //less as value moves away from .0
        inputSampleR += dR[count+1]; //we can assume always using this in one way or another?
        inputSampleR += (dR[count+2] * (offset-floor(offset))); //greater as value moves away from .0
        inputSampleR -= (((dR[count]-dR[count+1])-(dR[count+1]-dR[count+2]))/50); //interpolation hacks 'r us


        offset = start[2] + (modulation * sin(sweep + 2.0));
        count = gcount + (int)floor(offset);
        inputSampleL += dL[count] * (1-(offset-floor(offset))); //less as value moves away from .0
        inputSampleL += dL[count+1]; //we can assume always using this in one way or another?
        inputSampleL += (dL[count+2] * (offset-floor(offset))); //greater as value moves away from .0
        inputSampleL -= (((dL[count]-dL[count+1])-(dL[count+1]-dL[count+2]))/50); //interpolation hacks 'r us

        inputSampleR += dR[count] * (1-(offset-floor(offset))); //less as value moves away from .0
        inputSampleR += dR[count+1]; //we can assume always using this in one way or another?
        inputSampleR += (dR[count+2] * (offset-floor(offset))); //greater as value moves away from .0
        inputSampleR -= (((dR[count]-dR[count+1])-(dR[count+1]-dR[count+2]))/50); //interpolation hacks 'r us


        offset = start[3] + (modulation * sin(sweep + 3.0));
        count = gcount + (int)floor(offset);
        inputSampleL += dL[count] * (1-(offset-floor(offset))); //less as value moves away from .0
        inputSampleL += dL[count+1]; //we can assume always using this in one way or another?
        inputSampleL += (dL[count+2] * (offset-floor(offset))); //greater as value moves away from .0
        inputSampleL -= (((dL[count]-dL[count+1])-(dL[count+1]-dL[count+2]))/50); //interpolation hacks 'r us

        inputSampleR += dR[count] * (1-(offset-floor(offset))); //less as value moves away from .0
        inputSampleR += dR[count+1]; //we can assume always using this in one way or another?
        inputSampleR += (dR[count+2] * (offset-floor(offset))); //greater as value moves away from .0
        inputSampleR -= (((dR[count]-dR[count+1])-(dR[count+1]-dR[count+2]))/50); //interpolation hacks 'r us

        inputSampleL *= 0.125; //to get a comparable level
        inputSampleR *= 0.125; //to get a comparable level

        sweep += speed;
        if (sweep > tupi){sweep -= tupi;}
        //still scrolling through the samples, remember

        if (wet !=1.0) {
            inputSampleL = (inputSampleL * wet) + (drySampleL * dry);
            inputSampleR = (inputSampleR * wet) + (drySampleR * dry);
        }
        fpFlip = !fpFlip;


        //stereo 32 bit dither, made small and tidy.
        int expon; frexpf((float)inputSampleL, &expon);
        float dither = (rand()/(RAND_MAX*7.737125245533627e+25))*pow(2,expon+62);
        inputSampleL += (dither-fpNShapeL); fpNShapeL = dither;
        frexpf((float)inputSampleR, &expon);
        dither = (rand()/(RAND_MAX*7.737125245533627e+25))*pow(2,expon+62);
        inputSampleR += (dither-fpNShapeR); fpNShapeR = dither;
        //end 32 bit dither

        data.buf[i*2] = inputSampleL;
        data.buf[i*2 + 1]  = inputSampleR;
    }
    
}

ctagSoundProcessorEChorus::ctagSoundProcessorEChorus() {
    // construct internal data model
    knowYourself();
    model = std::make_unique<ctagSPDataModel>(id, isStereo);
    LoadPreset(0);

    for(int count = 0; count < totalsamples-1; count++) {dL[count] = 0; dR[count] = 0;}
    sweep = 3.141592653589793238 / 2.0;
    gcount = 0;
    airPrevL = 0.0;
    airEvenL = 0.0;
    airOddL = 0.0;
    airFactorL = 0.0;
    airPrevR = 0.0;
    airEvenR = 0.0;
    airOddR = 0.0;
    airFactorR = 0.0;
    fpFlip = true;
    fpNShapeL = 0.0;
    fpNShapeR = 0.0;
}

ctagSoundProcessorEChorus::~ctagSoundProcessorEChorus() {
}

void ctagSoundProcessorEChorus::knowYourself() {
    // autogenerated code here
    // sectionCpp0
	pMapPar.emplace("bypass", [&](const int val){ bypass = val;});
	pMapTrig.emplace("bypass", [&](const int val){ trig_bypass = val;});
	pMapPar.emplace("pdepth", [&](const int val){ pdepth = val;});
	pMapCv.emplace("pdepth", [&](const int val){ cv_pdepth = val;});
	pMapPar.emplace("prate", [&](const int val){ prate = val;});
	pMapCv.emplace("prate", [&](const int val){ cv_prate = val;});
	pMapPar.emplace("pwet", [&](const int val){ pwet = val;});
	pMapCv.emplace("pwet", [&](const int val){ cv_pwet = val;});
	isStereo = true;
	id = "EChorus";
	// sectionCpp0
}