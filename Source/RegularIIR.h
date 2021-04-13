#pragma once
#include "JuceHeader.h"

constexpr int filterSlope = 8;

namespace DynamicEQ
{
	using namespace dsp;

	enum Type
	{
		Peak,
		Highshelf,
		Lowshelf,
		Lowpass,
		Highpass,
	};

	enum Channels
	{
		leftChannel,
		rightChannel
	};

	enum FilterDB
	{
		dB_12,
		dB_24,
		dB_36,
		dB_48,
		dB_72,
		dB_96
	};

	struct SmoothedVals
	{
		LinearSmoothedValue<float>* PeakFreq;
		LinearSmoothedValue<float>* PeakQ;
		LinearSmoothedValue<float>* PeakGain;
		float* threshRatio;
		float* threshLevel;
		float* threshDecay;
		float* dynamicBool;

		void prepare(LinearSmoothedValue<float>* inFreq, LinearSmoothedValue<float>* inQ, LinearSmoothedValue<float>* inGain,
			float* inRatio, float* inLevel, float* inDecay, float* dynamicBoolToggle)
		{
			PeakFreq = inFreq;
			PeakQ = inQ;
			PeakGain = inGain;

			threshRatio = inRatio;
			threshLevel = inLevel;
			threshDecay = inDecay;
			dynamicBool = dynamicBoolToggle;
		}
	};

	class Threshold
	{
		/*A class that returns a dynamic GainFactor(for a IIR::Filter) in proportion to a sample crossing a threshold.*/
		
	public:

		Threshold()
		{
			for (int filter = 0; filter < filterSlope / 2; ++filter)
			{
				lowPassButterworth[filter].reset();
				highPassButterworth[filter].reset();
			}
			threshBandPass.reset();
		}
		void prepare(const ProcessSpec& spec, ReferenceCountedArray<IIR::Coefficients<float>> &threshLow,  ReferenceCountedArray<IIR::Coefficients<float>> &threshHigh)
		{
			//Assigns the inner Bandpass filter a particular state and assigns values to the threshold level and the threshold ratio.
			for (int filter = 0; filter < filterSlope / 2; ++filter)
			{
				*(lowPassButterworth[filter].coefficients) = *(threshLow[filter]);
				*(highPassButterworth[filter].coefficients) = *(threshHigh[filter]);
			}
			threshBuffer.setSize(spec.numChannels, spec.maximumBlockSize);

		}

		void prepare(const ProcessSpec& spec, ReferenceCountedObjectPtr<IIR::Coefficients<float>> &bandPassCoeffs)
		{
			*threshBandPass.coefficients = *bandPassCoeffs;
			threshBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
		}

		void reset()
		{
			//Resets the inner Bandpass filter state.
			for (int filter = 0; filter < filterSlope / 2; ++filter)
			{
				lowPassButterworth[filter].reset();
				highPassButterworth[filter].reset();
			}
			threshBandPass.reset();
		}

		float getAttenuationSample()
		{
			//Calculates the GainFactor for a single sample.
			//TODO - Check incoming sample level in relation to threhshold level
			float difference = bandPassSample - thresholdLevelGain;
			difference *= thresholdRatio;
			float newLevel =  difference + thresholdLevelGain;
			return newLevel / bandPassSample;
		}

		float getAttenuationRMS()
		{
			//Calculates the GainFactor for RMS.
			float difference = bandPassRMS - thresholdLevelGain;
			difference *= thresholdRatio;
			float newLevel = difference + thresholdLevelGain;
			return newLevel / bandPassRMS;
		}


		void updateThreshParameters(float inputRatio, float thresholdLevel, float inputDecay)
		{
			//Updates Threshold parameters.
			setThreshold(thresholdLevel);
			setRatio(inputRatio);
			setDecay(inputDecay);
		}

		void updateFilter(ReferenceCountedArray<dsp::IIR::Coefficients<float>> &threshLow, ReferenceCountedArray<dsp::IIR::Coefficients<float>> &threshHigh)
		{
			//Updates the inner Bandpass filter state.
			for (int filter = 0; filter < filterSlope / 2; ++filter)
			{
				*(lowPassButterworth[filter].coefficients) = *(threshLow[filter]);
				*(highPassButterworth[filter].coefficients) = *(threshHigh[filter]);
			}
		}
		
		void updateFilter(ReferenceCountedObjectPtr<IIR::Coefficients<float>> &bandPassCoeffs)
		{
			*threshBandPass.coefficients = *bandPassCoeffs;
			//threshBandPass.coefficients = std::move(bandPassCoeffs);
			//*threshBandPass.coefficients = std::move(*bandPassCoeffs);
		}
		void updateFilter(ReferenceCountedObjectPtr<IIR::Coefficients<float>>&& bandPassCoeffs)
		{
			threshBandPass.coefficients = bandPassCoeffs;
			//threshBandPass.coefficients = std::move(bandPassCoeffs);
			//*threshBandPass.coefficients = std::move(*bandPassCoeffs);
		}

		bool aboveThresholdSample() 
			//Returns true if a sample level is above threshold.
			//Returns false if below.
		{
			return ( bandPassSample > thresholdLevelGain ? true : false);
		}

		bool aboveThresholdRMS()
		{
			//Return true if RMS level is above threshold
			//Returns false if below.
			return (bandPassRMS > thresholdLevelGain ? true : false);
		}

		void calcThresholdSample(float sample)
			//Calculates current sample level after being processed by the inner Bandpass.
		{
			bandPassSample = sample;
			for (int filter = 0; filter < filterSlope / 2; ++filter)
			{
				bandPassSample = lowPassButterworth[filter].processSample(bandPassSample);
			}
			for (int filter = 0; filter < filterSlope / 2; ++filter)
			{
				bandPassSample = highPassButterworth[filter].processSample(bandPassSample);
			}
		}

		void calcThresholdRMS_fullBuffer(int channel)
		{
			//Calculates RMS for a full buffer length.
			bandPassRMS = threshBuffer.getRMSLevel(channel, 0, threshBuffer.getNumSamples());
		}


		void calcThresholdRMS_Small(int channel, int startSample, int numSamps)
		{
			//Calculates RMS for some (buffer.getNumSamples() / N) buffer length.
			bandPassRMS = threshBuffer.getRMSLevel(channel, startSample, numSamps);
		}

		void calcThrehsoldRMS_Expo(int channel, int startSample, int numSamps)
		{
			if (firstBuffer)
			{
				bandPassRMS = threshBuffer.getRMSLevel(channel, startSample, numSamps);
				firstBuffer = false;
			}
			else
			{
				bandPassRMS *= 1.f - thresholdDecay;
				bandPassRMS += (threshBuffer.getRMSLevel(channel, startSample, numSamps) * thresholdDecay);

			}
		}

		void getFirstRMS(int channel, int startSample, int numSamps)
		{
			bandPassRMS = threshBuffer.getRMSLevel(channel, startSample, numSamps);
		}
		void calcThresholdRMS_ExpoSample(const float decayConst, float sample)
		{
			bandPassRMS *= 1.f - decayConst;
			bandPassRMS += std::sqrt( (sample * sample) * decayConst);
		}


		void CopyAndFilter(AudioBuffer<float>& loopBuffer, int channel)
		{
			//Copies processBlock's buffer and filters the copy with a bandpass.
			threshBuffer.makeCopyOf(loopBuffer);
			for (int sample = 0; sample < threshBuffer.getNumSamples(); ++sample)
			{
				float currentSample = threshBuffer.getSample(channel, sample);
				threshBuffer.setSample(channel, sample, threshBandPass.processSample(currentSample));
			}
		}

		float getSample(int channel, int sample)
		{
			return threshBuffer.getSample(channel, sample);
		}


		float bandPassSample;
		float bandPassRMS;

	private:

		IIR::Filter<float> lowPassButterworth[filterSlope / 2];
		IIR::Filter<float> highPassButterworth[filterSlope / 2];
		IIR::Filter<float> threshBandPass;
		AudioBuffer<float> threshBuffer;
		float thresholdRatio = 0.5f;
		float thresholdLevelGain = 0.5f;
		float thresholdDecay = 0.001f;
		bool firstBuffer = true;


		float getThresholdDb() const
		{
			return Decibels::gainToDecibels<float>(thresholdLevelGain);
		}

		void setThreshold(float thresholdLevel)
			//Sets the threshold level.
		{
			if (thresholdLevel != thresholdLevelGain)
			{
				thresholdLevelGain = thresholdLevel;
			}
		}

		void setRatio(float inputRatio)
			//Demands input as a floating number or fraction.
		{
			if (inputRatio != thresholdRatio)
			{
				thresholdRatio = inputRatio;
			}
		}

		void setDecay(float inputDecay)
		{
			if (inputDecay != thresholdDecay)
			{
				thresholdDecay = inputDecay;
			}
		}


	};

	//==========================================================================================
	//==========================================================================================


	class monoFilter
	{
		/*A class that supplies a regular dsp::IIR::Filter along with parameter changing.*/
	public:
		monoFilter()
		{
			filter.reset();
		}
		void prepare(const ProcessSpec& spec, int FilterType)
		{
			//Set DynamicBool Ptr
			dynamicToggle = innerSmoothedValues.dynamicBool;
			//Prepares the filter for processing.
			sampleRate = spec.sampleRate;
			gainSmoothing.reset(sampleRate, 0.001f);
			gainSmoothing = 1.0f;

			//Preparing..
			filter.prepare(spec);

			//Setting correct filter type.
			switch (FilterType)
			{
			case Peak:
				filterFunc = IIR::Coefficients<float>::makePeakFilter;
				break;
				
			case Highshelf:
				filterFunc = IIR::Coefficients<float>::makeHighShelf;
				break;

			case Lowshelf:
				filterFunc = IIR::Coefficients<float>::makeLowShelf;
				break;
			}
			
			//Setting up filters..
			auto setFilter = filterFunc(sampleRate, currentFreq, currentQ, currentGain); 
			*filter.coefficients = *setFilter;

			frequencyThreshold.prepare(spec, IIR::Coefficients<float>::makeBandPass(sampleRate, currentFreq, currentQ));
		}

		void reset() noexcept
		{
			//Resets the filter's and threshold's state.
			filter.reset(); 
			frequencyThreshold.reset();
		}

		void setInnerSmoothedVals(LinearSmoothedValue<float>* inFreq, LinearSmoothedValue<float>* inQ, LinearSmoothedValue<float>* inGain,
							     float* inRatio, float* inLevel, float* inDecay, float* dynamicBoolToggle)
		{
			innerSmoothedValues.prepare(inFreq, inQ, inGain, inRatio, inLevel, inDecay, dynamicBoolToggle);
		}

		//==========================================================================================

		void updateParams(float frequency, float qFactor, float gain)
		{
			//Changes the parameters of the filter. Makes sure that the change is valid first.
			if (validChange(frequency, qFactor, gain))
			{
				currentFreq = frequency;
				currentQ = qFactor;
				currentGain = gain;


				setCoeffs();
				frequencyThreshold.updateFilter(IIR::Coefficients<float>::makeBandPass(sampleRate, currentFreq, currentQ));
			}
		}

		void processContext_smallerRMS(ProcessContextReplacing<float>& innerContext, AudioBuffer<float>& innerBuffer, int channel)
		{
			/*Calculates the incoming sample with a bandpass to get the proper sample level for the threshold
			  and then asks if that sample level is above the threshold set.
			  if so, calculates the amount of attenuation and makes new coefficients with that attenuation.*/

			int RMSWindow = innerBuffer.getNumSamples() / 4;
			frequencyThreshold.CopyAndFilter(innerBuffer, channel);
			float funcGain = currentGain;

			for (int sample = 0; sample < innerBuffer.getNumSamples(); ++sample)
			{
				UI_Gain = innerSmoothedValues.PeakGain->getNextValue();
				if(*dynamicToggle == 1.0f)
				{
					if (sample % RMSWindow == 0)
					{
						frequencyThreshold.calcThrehsoldRMS_Expo(channel, sample, RMSWindow);

						if (frequencyThreshold.aboveThresholdRMS())
						{
							float attenuation = frequencyThreshold.getAttenuationRMS();
							gainSmoothing.setTargetValue(attenuation);
							funcGain = gainSmoothing.getNextValue();
						}

						else
						{
							gainSmoothing.setTargetValue(1.0f);
							funcGain = gainSmoothing.getNextValue();
						}
					}
				}
				else
				{
					gainSmoothing.setTargetValue(UI_Gain);
					funcGain = gainSmoothing.getNextValue();
				}

				frequencyThreshold.updateThreshParameters(*innerSmoothedValues.threshRatio, *innerSmoothedValues.threshLevel, *innerSmoothedValues.threshDecay);
				updateParams(innerSmoothedValues.PeakFreq->getNextValue(), innerSmoothedValues.PeakQ->getNextValue(), funcGain);

				innerBuffer.setSample(channel, sample, filter.processSample(innerBuffer.getSample(channel, sample)));

			}
		}
		

		void setFilterType(int FilterType)
		{
			//Sets the filter's type using std::function.
			//std::function is used to avoid having a switch block nested in each update call.
			switch (FilterType)
			{
			case Peak:
				filterFunc = IIR::Coefficients<float>::makePeakFilter;
				setCoeffs();
				break;

			case Highshelf:
				filterFunc = IIR::Coefficients<float>::makeHighShelf;
				setCoeffs();
				break;

			case Lowshelf:
				filterFunc = IIR::Coefficients<float>::makeLowShelf;
				setCoeffs();
				break;
			}
		}

		void setCoeffs()
		{
			filter.coefficients = filterFunc(sampleRate, currentFreq, currentQ, currentGain);
		}

		void setCoeffs(float externalGain)
		{
			//Threshold Version
			auto setFilter = filterFunc(sampleRate, currentFreq, currentQ, externalGain);
			*filter.coefficients = *setFilter;
		}

		void setCoeffs(const Array<float>& lastState)
		{
			//Copy Version
			filter.coefficients->coefficients = lastState;
		}


		Threshold frequencyThreshold;
		SmoothedVals innerSmoothedValues;
	private:
		IIR::Filter<float> filter;

		std::function <ReferenceCountedObjectPtr<IIR::Coefficients<float>>(double, float, float, float)> filterFunc = nullptr;
		double sampleRate = 0.0;
		float currentFreq = 440.f;
		float currentQ = 1.f;
		float currentGain = 1.f;
		float UI_Gain = 1.f;
		LinearSmoothedValue<float> gainSmoothing;
		float* dynamicToggle = nullptr;




		bool validSampleRate()
		{
			//Checks if sampleRate is valid.
			if (sampleRate != 0.0)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		bool validChange(float inputFreq, float inputQ, float inputGain)
		{
			//Checks if a parameter change is valid.
			if (validSampleRate() && ((inputFreq != currentFreq) || (inputQ != currentQ) || (inputGain != currentGain)))
			{
				return true;
			}
			else
			{
				return false;
			}
		}

	};


	//==========================================================================================
	//==========================================================================================

	struct stereoFilter
	{

		monoFilter innerFilters[2];

		//===================================================================================

		void prepare(const ProcessSpec& spec, int FilterType)
		{
			innerFilters[leftChannel].prepare(spec, FilterType);
			innerFilters[rightChannel].prepare(spec, FilterType);
		}

		void reset()
		{
			innerFilters[leftChannel].reset();
			innerFilters[rightChannel].reset();
		}

		void processContext_Small(ProcessContextReplacing<float>& context, AudioBuffer<float>& buffer)
		{
			innerFilters[leftChannel].processContext_smallerRMS(context, buffer, leftChannel);
			innerFilters[rightChannel].processContext_smallerRMS(context, buffer, rightChannel);
		}

		void updateFilterParams(float Freq, float Q, float gain = 1.f)
		{
			innerFilters[leftChannel].updateParams(Freq, Q, gain);
			innerFilters[rightChannel].updateParams(Freq, Q, gain);
		}

		void setFilterType(int FilterType)
		{
			innerFilters[leftChannel].setFilterType(FilterType);
			innerFilters[rightChannel].setFilterType(FilterType);
		}

		void setInnerSmoothed(LinearSmoothedValue<float>* freq, LinearSmoothedValue<float>* q, LinearSmoothedValue<float>* gain,
			float* inRatio, float* inLevel, float* inDecay, float* DynamicBoolPtr)
		{
			innerFilters[leftChannel].setInnerSmoothedVals(freq, q, gain, inRatio, inLevel, inDecay, DynamicBoolPtr);
			innerFilters[rightChannel].setInnerSmoothedVals(freq, q, gain, inRatio, inLevel, inDecay, DynamicBoolPtr);
		}

	};

}
