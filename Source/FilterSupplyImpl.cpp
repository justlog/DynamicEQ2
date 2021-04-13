
#include "FilterSupply.h"

PointerStore::PointerStore()
{

}
PointerStore::~PointerStore()
{
	for (uint8 i = 0; i < CONST_FILTER_NUM; ++i)
	{
		constFreqPtr[i] = nullptr;
	}

	for (uint8 i = 0; i < VAR_FILTER_NUM; ++i)
	{
		varFreqPtr[i] = nullptr;
		varGainPtr[i] = nullptr;
		varQPtr[i] = nullptr;
		varThreshPtr[i] = nullptr;
		varRatioPtr[i] = nullptr;
	}

	for (uint8 i = 0; i < TOTAL_FILTER_NUM; ++i)
	{
		togglePtr[i] = nullptr;
	}
}

void PointerStore::passPointers(std::array<LinearSmoothedValue<float>, TOTAL_FILTER_NUM>& SmoothedFreq, std::array<LinearSmoothedValue<float>, TOTAL_FILTER_NUM>& SmoothedQ,
	std::array<LinearSmoothedValue<float>, VAR_FILTER_NUM>& SmoothedGain, std::array<float*, VAR_FILTER_NUM>& ThresholdLevel, std::array<float*, VAR_FILTER_NUM>& ThresholdRatio,
	std::array<float*, VAR_FILTER_NUM>& ThresholdDecay, std::array<float*, VAR_FILTER_NUM>& DynamicBoolArray)
{
	for (uint8 i = 0; i < TOTAL_FILTER_NUM; ++i)
	{
		if (i < CONST_FILTER_NUM)
		{
			constFreqPtr[i] = &SmoothedFreq[i];
			constQPtr[i] = &SmoothedQ[i];
		}
		else
		{
			varFreqPtr[i - CONST_FILTER_NUM] = &SmoothedFreq[i];
			varQPtr[i - CONST_FILTER_NUM] = &SmoothedQ[i];
			varGainPtr[i - CONST_FILTER_NUM] = &SmoothedGain[i - CONST_FILTER_NUM];
			varThreshPtr[i - CONST_FILTER_NUM] = ThresholdLevel[i - CONST_FILTER_NUM];
			varRatioPtr[i - CONST_FILTER_NUM] = ThresholdRatio[i - CONST_FILTER_NUM];
			varDecayPtr[i - CONST_FILTER_NUM] = ThresholdDecay[i - CONST_FILTER_NUM];
			varDynamicBoolPtr[i - CONST_FILTER_NUM] = DynamicBoolArray[i - CONST_FILTER_NUM];

		}
	}
}

//==========================================================================================

FilterStore::FilterStore()
{
	for (int i = 0; i < TOTAL_FILTER_NUM; ++i)
	{
		activeFilters[i] = 0;
	}
}

void FilterStore::reset()
{
	for (int i = 0; i < CONST_FILTER_NUM; ++i)
	{
		constTypeFilters[i].reset();
	}
	constTypeFilters[0].setType(1);

	for (int i = 0; i < VAR_FILTER_NUM; ++i)
	{
		varTypeFilters[i].reset();
	}
}


void FilterStore::prepare(dsp::ProcessSpec inSpec)
{
	for (int i = 0; i < CONST_FILTER_NUM; ++i)
	{
		constTypeFilters[i].prepare(inSpec);
	}

	for (int i = 0; i < VAR_FILTER_NUM; ++i)
	{
		varTypeFilters[i].prepare(inSpec, DynamicEQ::Peak);
	}
}

void FilterStore::setFilterType(int filterNum, int filterType)
{
	if (filterNum > highpass)
	{
		varTypeFilters[filterNum - 2].setFilterType(filterType);
	}
	else
	{
		constTypeFilters[filterNum].setType(filterType);
	}
}

void FilterStore::toggleFilter(int filterNum)
{
	if (activeFilters[filterNum])
	{
		activeFilters[filterNum] = 0;
		DBG("Filter number " << filterNum << " deactivated.");
	}
	else
	{
		activeFilters[filterNum] = 1;
		DBG("Filter number " << filterNum << " activated.");
	}
}

void FilterStore::process(AudioBuffer<float>& inBuffer, dsp::ProcessContextReplacing<float>& context)
{
	for (uint8 filter = 0; filter < TOTAL_FILTER_NUM; ++filter)
	{
		if (filter < CONST_FILTER_NUM)
		{
			if (activeFilters[filter])
			{
				constTypeFilters[filter].processSample(inBuffer);
			}
			continue;
		}
		else
		{
			if (activeFilters[filter])
			{
				varTypeFilters[filter - CONST_FILTER_NUM].processContext_Small(context, inBuffer);
			}
		}
	}
}


void FilterStore::passSmoothedVars(PointerStore& p)
{
	for (uint8 i = 0; i < CONST_FILTER_NUM; ++i)
	{
		constTypeFilters[i].setSmoothedVals(p.constFreqPtr[i]);
	}

	for (uint8 i = 0; i < VAR_FILTER_NUM; ++i)
	{
		varTypeFilters[i].setInnerSmoothed(p.varFreqPtr[i], p.varQPtr[i], p.varGainPtr[i], p.varRatioPtr[i], p.varThreshPtr[i], p.varDecayPtr[i], p.varDynamicBoolPtr[i]);
	}

}
