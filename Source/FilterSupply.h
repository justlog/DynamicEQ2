#pragma once
#include "JuceHeader.h"
#include "RegularIIR.h"
#include "IIRStereoWrapperHeader.h"
//#define CONST_FILTER_NUM 2
//#define VAR_FILTER_NUM 7 
//#define TOTAL_FILTER_NUM (CONST_FILTER_NUM + VAR_FILTER_NUM)
constexpr int CONST_FILTER_NUM = 2;
constexpr int VAR_FILTER_NUM   = 7;
constexpr int TOTAL_FILTER_NUM = (CONST_FILTER_NUM + VAR_FILTER_NUM);


enum filterPlace
{
	lowpass,
	highpass,
	varFilter1,
	varFilter2,
	varFilter3,
};

struct PointerStore
{
	PointerStore();

	~PointerStore();
	
	void passPointers(std::array<LinearSmoothedValue<float>, TOTAL_FILTER_NUM>& SmoothedFreq, std::array<LinearSmoothedValue<float>, TOTAL_FILTER_NUM>& SmoothedQ,
					  std::array<LinearSmoothedValue<float>, VAR_FILTER_NUM>& SmoothedGain, std::array<float*, VAR_FILTER_NUM>& ThresholdLevel, std::array<float*, VAR_FILTER_NUM>& ThresholdRatio,
					  std::array<float*, VAR_FILTER_NUM>& ThresholdDecay, std::array<float*, VAR_FILTER_NUM>& DynamicBoolArray);

	std::array<LinearSmoothedValue<float>*, CONST_FILTER_NUM> constFreqPtr;
	std::array<LinearSmoothedValue<float>*, CONST_FILTER_NUM> constQPtr;
	std::array<LinearSmoothedValue<float>*, VAR_FILTER_NUM> varFreqPtr;
	std::array<LinearSmoothedValue<float>*, VAR_FILTER_NUM> varGainPtr;
	std::array<LinearSmoothedValue<float>*, VAR_FILTER_NUM> varQPtr;
	std::array<float*, VAR_FILTER_NUM> varThreshPtr;
	std::array<float*, VAR_FILTER_NUM> varRatioPtr;
	std::array<float*, VAR_FILTER_NUM> varDecayPtr;
	std::array<float*, TOTAL_FILTER_NUM> togglePtr;
	std::array<float*, VAR_FILTER_NUM> varDynamicBoolPtr;
};

class FilterStore
{
public:

	FilterStore();

	void reset();

	void prepare(dsp::ProcessSpec);

	void toggleFilter(int);

	void setFilterType(int, int);

	void process(AudioBuffer<float>&, dsp::ProcessContextReplacing<float>&);

	void passSmoothedVars(PointerStore&);


private:
	std::array<StereoIIR, CONST_FILTER_NUM> constTypeFilters;
	std::array<DynamicEQ::stereoFilter, VAR_FILTER_NUM> varTypeFilters;
	std::array<int, TOTAL_FILTER_NUM> activeFilters;
};

