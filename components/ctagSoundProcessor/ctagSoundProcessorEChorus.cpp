#include "ctagSoundProcessorEChorus.hpp"
#include "helpers/ctagFastMath.hpp"
#include "esp_heap_caps.h"
#include <cmath>

using namespace CTAG::SP;
using namespace CTAG::SP::HELPERS;


void ctagSoundProcessorEChorus::Process(const ProcessData &data) {
    float overallscale = 1.0;

    float speed = powf((prate/4095.f),3.f) * 0.001f;
    speed *= overallscale;
    int loopLimit = (int)(totalsamples * 0.499f);
    int count;
    float range = powf((pdepth/4095.f),3.f) * loopLimit * 0.12f;
    float wet = (pwet / 4095.f);
    float modulation = range*wet;
    float dry = 1.0f - wet;
    float tupi = 3.141592653589793238f * 2.0f;
    float offset;
    float start[4];

    float inputSampleL;
    float inputSampleR;
    float drySampleL;
    float drySampleR;
    //now we'll precalculate some stuff that needn't be in every sample
    start[0] = range;
    start[1] = range * 2.f;
    start[2] = range * 3.f;
    start[3] = range * 4.f;

    for(int i=0;i<bufSz;i++)
    {
        inputSampleL = data.buf[i*2];
        inputSampleR = data.buf[i*2 + 1];
        drySampleL = inputSampleL;
        drySampleR = inputSampleR;

        airFactorL = airPrevL - inputSampleL;
        if (fpFlip) {airEvenL += airFactorL; airOddL -= airFactorL; airFactorL = airEvenL;}
        else {airOddL += airFactorL; airEvenL -= airFactorL; airFactorL = airOddL;}
        airOddL = (airOddL - ((airOddL - airEvenL)/256.0f)) / 1.0001f;
        airEvenL = (airEvenL - ((airEvenL - airOddL)/256.0f)) / 1.0001f;
        airPrevL = inputSampleL;
        inputSampleL += (airFactorL*wet);
        //air, compensates for loss of highs in flanger's interpolation

        airFactorR = airPrevR - inputSampleR;
        if (fpFlip) {airEvenR += airFactorR; airOddR -= airFactorR; airFactorR = airEvenR;}
        else {airOddR += airFactorR; airEvenR -= airFactorR; airFactorR = airOddR;}
        airOddR = (airOddR - ((airOddR - airEvenR)/256.0f)) / 1.0001f;
        airEvenR = (airEvenR - ((airEvenR - airOddR)/256.0f)) / 1.0001f;
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
        count = gcount + (int)floorf(offset);
        inputSampleL = dL[count] * (1.f-(offset-floorf(offset))); //less as value moves away from .0
        inputSampleL += dL[count+1]; //we can assume always ufastsing this in one way or another?
        inputSampleL += (dL[count+2] * (offset-floorf(offset))); //greater as value moves away from .0
        inputSampleL -= (((dL[count]-dL[count+1])-(dL[count+1]-dL[count+2]))/50.f); //interpolation hacks 'r us

        inputSampleR = dR[count] * (1.f-(offset-floorf(offset))); //less as value moves away from .0
        inputSampleR += dR[count+1]; //we can assume always ufastsing this in one way or another?
        inputSampleR += (dR[count+2] * (offset-floorf(offset))); //greater as value moves away from .0
        inputSampleR -= (((dR[count]-dR[count+1])-(dR[count+1]-dR[count+2]))/50.f); //interpolation hacks 'r us

        offset = start[1] + (modulation * fastsin(sweep + 1.0f));
        count = gcount + (int)floorf(offset);
        inputSampleL += dL[count] * (1-(offset-floorf(offset))); //less as value moves away from .0
        inputSampleL += dL[count+1]; //we can assume always using this in one way or another?
        inputSampleL += (dL[count+2] * (offset-floorf(offset))); //greater as value moves away from .0
        inputSampleL -= (((dL[count]-dL[count+1])-(dL[count+1]-dL[count+2]))/50.f); //interpolation hacks 'r us

        inputSampleR += dR[count] * (1-(offset-floorf(offset))); //less as value moves away from .0
        inputSampleR += dR[count+1]; //we can assume always using this in one way or another?
        inputSampleR += (dR[count+2] * (offset-floorf(offset))); //greater as value moves away from .0
        inputSampleR -= (((dR[count]-dR[count+1])-(dR[count+1]-dR[count+2]))/50); //interpolation hacks 'r us


        offset = start[2] + (modulation * fastsin(sweep + 2.0f));
        count = gcount + (int)floorf(offset);
        inputSampleL += dL[count] * (1-(offset-floorf(offset))); //less as value moves away from .0
        inputSampleL += dL[count+1]; //we can assume always using this in one way or another?
        inputSampleL += (dL[count+2] * (offset-floorf(offset))); //greater as value moves away from .0
        inputSampleL -= (((dL[count]-dL[count+1])-(dL[count+1]-dL[count+2]))/50.f); //interpolation hacks 'r us

        inputSampleR += dR[count] * (1-(offset-floorf(offset))); //less as value moves away from .0
        inputSampleR += dR[count+1]; //we can assume always using this in one way or another?
        inputSampleR += (dR[count+2] * (offset-floorf(offset))); //greater as value moves away from .0
        inputSampleR -= (((dR[count]-dR[count+1])-(dR[count+1]-dR[count+2]))/50.f); //interpolation hacks 'r us


        offset = start[3] + (modulation * fastsin(sweep + 3.0f));
        count = gcount + (int)floorf(offset);
        inputSampleL += dL[count] * (1-(offset-floorf(offset))); //less as value moves away from .0
        inputSampleL += dL[count+1]; //we can assume always using this in one way or another?
        inputSampleL += (dL[count+2] * (offset-floorf(offset))); //greater as value moves away from .0
        inputSampleL -= (((dL[count]-dL[count+1])-(dL[count+1]-dL[count+2]))/50.f); //interpolation hacks 'r us

        inputSampleR += dR[count] * (1-(offset-floorf(offset))); //less as value moves away from .0
        inputSampleR += dR[count+1]; //we can assume always using this in one way or another?
        inputSampleR += (dR[count+2] * (offset-floorf(offset))); //greater as value moves away from .0
        inputSampleR -= (((dR[count]-dR[count+1])-(dR[count+1]-dR[count+2]))/50.f); //interpolation hacks 'r us

        inputSampleL *= 0.125f; //to get a comparable level
        inputSampleR *= 0.125f; //to get a comparable level

        sweep += speed;
        if (sweep > tupi){sweep -= tupi;}
        //still scrolling through the samples, remember

        if (wet != 1.0f) {
            inputSampleL = (inputSampleL * wet) + (drySampleL * dry);
            inputSampleR = (inputSampleR * wet) + (drySampleR * dry);
        }
        fpFlip = !fpFlip;

        data.buf[i*2] = inputSampleL;
        data.buf[i*2 + 1]  = inputSampleR;
    }
    
}

ctagSoundProcessorEChorus::ctagSoundProcessorEChorus() {
    // construct internal data model
    knowYourself();
    model = std::make_unique<ctagSPDataModel>(id, isStereo);
    LoadPreset(0);

    dL = (float*)heap_caps_malloc(totalsamples * sizeof(float), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    dR = (float*)heap_caps_malloc(totalsamples * sizeof(float), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    for(int count = 0; count < totalsamples-1; count++) {dL[count] = 0; dR[count] = 0;}
    sweep = 3.141592653589793238f / 2.0f;
    gcount = 0;
    airPrevL = 0.0f;
    airEvenL = 0.0f;
    airOddL = 0.0f;
    airFactorL = 0.0f;
    airPrevR = 0.0f;
    airEvenR = 0.0f;
    airOddR = 0.0f;
    airFactorR = 0.0f;
    fpFlip = true;
    fpNShapeL = 0.0f;
    fpNShapeR = 0.0f;
}

ctagSoundProcessorEChorus::~ctagSoundProcessorEChorus() {
    heap_caps_free(dL);
    heap_caps_free(dR);
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