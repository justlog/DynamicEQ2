#pragma once
#include "JuceHeader.h"

struct StereoIIR
{

	StereoIIR();
	
	~StereoIIR();

	void setType(int);

	void reset();

	void prepare(dsp::ProcessSpec&);

	void processSample(AudioBuffer<float>&);

	void updateParams();

	void setSmoothedVals(LinearSmoothedValue<float>*, LinearSmoothedValue<float>*, LinearSmoothedValue<float>*, float*, float*, float*);
	void setSmoothedVals(LinearSmoothedValue<float>*);

	LinearSmoothedValue<float>* frequencySmoothed;
	LinearSmoothedValue<float>* qSmoothed;
	LinearSmoothedValue<float>* gainSmoothed;
	float* ratioPtr;
	float* threshPtr;
	float* decayPtr;

	std::array<dsp::IIR::Filter<float>, 2> IIRFilters;
	int CoeffsType;
	float currentFreq;
	float sampleRate;
};
