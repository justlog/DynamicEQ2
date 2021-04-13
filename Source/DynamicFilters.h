#pragma once
#include "../JuceLibraryCode/JuceHeader.h"

namespace CustomDSP
{
	using namespace dsp;

	typedef IIR::Filter<float> Filter;
	typedef IIR::Coefficients<float> Coeff;

	class filterChain
	{
	/*
	Need to do:
		- Implement needToUpdate():
			- Done.

		- Check how to avoid crackle and pops in signal when processing:
			- Need to ask Yair what is the best way to get proper smoothing on parameters while keeping latency to the minimum. 
    */
	public:
		//=============================================================================
		void prepare(const ProcessSpec& spec)
		{
			sampleRate = spec.sampleRate;
			auto& lowPass = mFilters.get<Lowpass>();
			auto& highPass = mFilters.get<Highpass>();
			auto& peakfilter = mFilters.get<Peak>();

			*lowPass.state = *Coeff::makeLowPass(sampleRate, lastFrequencies[Lowpass]);
			*highPass.state = *Coeff::makeHighPass(sampleRate, lastFrequencies[Highpass]);
			*peakfilter.state = *Coeff::makePeakFilter(sampleRate, lastFrequencies[Peak], lastQ[Peak], 1.0f);

			mFilters.prepare(spec);
		}

		void reset() noexcept
		{
			mFilters.reset();
		}

		void process(const ProcessContextReplacing<float>& context)
		{
			mFilters.process(context);
		}
		//=============================================================================

		void updateLowpass(float lowPassFreq, float lowPassQ)
		{
			//Updates Lowpass filter values.
			if (validChange(Lowpass, lowPassFreq, lowPassQ))
			{
				//DBG("Called updateLowpass");
				auto& lowPass = mFilters.get<Lowpass>();
				*lowPass.state = *Coeff::makeLowPass(sampleRate, lowPassFreq, lowPassQ);
				lastFrequencies[Lowpass] = lowPassFreq;
				lastQ[Lowpass] = lowPassQ;
			}
		}

		void updateHighpass(float highPassFreq, float highPassQ)
		{
			//Updates Highpass filter values.
			if (validChange(Highpass, highPassFreq, highPassQ))
			{
				//DBG("Called updateHighpass");
				auto& highPass = mFilters.get<Highpass>();
				*highPass.state = *Coeff::makeHighPass(sampleRate, highPassFreq, highPassQ);

				lastFrequencies[Highpass] = highPassFreq;
				lastQ[Highpass] = highPassQ;
			}
		}

		void updatePeakFilter(float peakFreq, float peakQ, float peakGain)
		{
			//Updates peak filter values.
			if (validChange(Peak, peakFreq, peakQ, peakGain))
			{
				//DBG("Called updatePeakFilter");
				auto& peakFilter = mFilters.get<Peak>();
				*peakFilter.state = *Coeff::makePeakFilter(sampleRate, peakFreq, peakQ, peakGain);
				lastFrequencies[Peak] = peakFreq;
				lastQ[Peak] = peakQ;
				lastGain[Peak] = peakGain;
			}
		}

		//=============================================================================

	private:

		//=============================================================================
		ProcessorChain<ProcessorDuplicator<Filter, Coeff>, ProcessorDuplicator<Filter, Coeff>, ProcessorDuplicator<Filter, Coeff>> mFilters;
		double sampleRate = 0.0;
		float lastFrequencies[3]{ 22000.f, 20.f, 4000.f };
		float lastQ[3]{ 1.0, 1.0, 1.0 };
		float lastGain[3]{ 1.0f, 1.0f, 1.0f };
		//=============================================================================

		bool validChange(int filterNum, float freq, float Q, float gain = 0.0f)
		{
			//Checks to see if call to update is a valid change.
			switch (filterNum)
			{
			case 0: case 1:
				if (validSampleRate() && (lastFrequencies[filterNum] != freq || lastQ[filterNum] != Q) )
				{
					return true;
				}
				else
				{
					return false;
				}
				break;

			case 2:
				if (validSampleRate() && (lastFrequencies[filterNum] != freq || lastQ[filterNum] != Q || lastGain[filterNum] != gain) )
				{
					return true;
				}
				else
				{
					return false;
				}
				break;
			}
		}

		bool validSampleRate() const
		{
			if (sampleRate != 0.0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		enum FilterIndex
		{
			Lowpass,
			Highpass,
			Peak
		};
	};

}

//class Threshold
//{
//
//	/*
//	In this class, the calculations are as follows:
//	Threshold - in dB
//	Attenuation - in Gain
//	
//	Need to do:
//		- Need to calculate level of input signal within the cutoff frequency of the filter.
//		- Check behaviour of filters without gain control(Lowpass, Highpas etc.) in other dynamic equalizer plugins and decide how to deal with them.
//
//	General class flow:
//		1. Calculate difference between threshold and input signal
//		2. Multiply that difference by ratio
//		3. Add threshold to the difference
//	*/
//public:
//	//=============================================================================
//	
//
//	bool aboveThreshold(float currentLevel) const
//	{
//		if (currentLevel > thresholdLevel)
//		{
//			return true;
//		}
//		else
//		{
//			return false;
//		}
//	}
//
//	//=============================================================================
//	void setThreshold(float threshold_Gain)
//	{
//		thresholdLevel = threshold_Gain;
//	}
//
//	void setRatio(float ratio)
//	{
//		thresholdRatio = ratio;
//	}
//	
//private:
//	float getLeveldB(float sampleLevel) const
//	{
//		return Decibels::gainToDecibels<float>(sampleLevel);
//	}
//
//	float thresholdRatio = 1.0f;
//	float thresholdLevel = 0.0f;
//};
