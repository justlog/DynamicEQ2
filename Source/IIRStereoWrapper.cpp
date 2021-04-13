
#include "IIRStereoWrapperHeader.h"

StereoIIR::StereoIIR()
{
	IIRFilters[0].reset();
	IIRFilters[1].reset();
	currentFreq = 440.f;
	sampleRate = 0.0;
	CoeffsType = 0;
}

StereoIIR::~StereoIIR()
{
	frequencySmoothed = nullptr;
}

void StereoIIR::reset()
{
	IIRFilters[0].reset();
	IIRFilters[1].reset();
}

void StereoIIR::prepare(dsp::ProcessSpec& spec)
{
	for (uint8 i = 0; i < 2; ++i)
	{
		IIRFilters[i].prepare(spec);
	}
	sampleRate = spec.sampleRate;
}

void StereoIIR::setType(int type)
{
	CoeffsType = type;
}

void StereoIIR::processSample(AudioBuffer<float>& buffer)
{
	for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
	{
		buffer.setSample(0, sample, IIRFilters[0].processSample(buffer.getSample(0, sample)));
		buffer.setSample(1, sample, IIRFilters[1].processSample(buffer.getSample(1, sample)));
		updateParams();
	}
}

void StereoIIR::updateParams()
{
	if (currentFreq != frequencySmoothed->getNextValue())
	{
		currentFreq = frequencySmoothed->getCurrentValue();
		switch (CoeffsType)
		{
		case 0:
			IIRFilters[0].coefficients = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, currentFreq);
			IIRFilters[1].coefficients = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, currentFreq);
			break;

		case 1:
			IIRFilters[0].coefficients = dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, currentFreq);
			IIRFilters[1].coefficients = dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, currentFreq);
			break;
		}
	}
}

void StereoIIR::setSmoothedVals(LinearSmoothedValue<float>* inFreq, LinearSmoothedValue<float>* inQ, LinearSmoothedValue<float>* inGain,
								float* inRatio, float* inThresh, float* inDecay)
{
	frequencySmoothed = inFreq;
	qSmoothed = inQ;
	gainSmoothed = inGain;

	ratioPtr = inRatio;
	threshPtr = inThresh;
	decayPtr = inDecay;
}

void StereoIIR::setSmoothedVals(LinearSmoothedValue<float>* inFreq)
{
	frequencySmoothed = inFreq;
}
