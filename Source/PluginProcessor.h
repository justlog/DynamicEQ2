#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "FilterSupply.h"
#include "MainGUI.h"

//REMEMBER! YOU'VE CHANGED THE FFT BOOL VARIABLE TO BE VOLATILE SINCE YOU WEREN'T SURE ABOUT THE COMPILER OPTIMIZATION THING
//IT'S PROBABLY OKAY SINCE YOU'VE RAN IT AND IT SEEMS TO BE WORKING FINE
//BUT YOU PROBABLY WANT TO MAKE SURE THAT IT IS SO SO YOU CAN SHIP IT PROPERLY.
//==============================================================================
/**
*/
class DynamicEq2AudioProcessor : public AudioProcessor
{
public:
	//==============================================================================
	DynamicEq2AudioProcessor();
	~DynamicEq2AudioProcessor();

	//==============================================================================
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

	void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

	//==============================================================================
	AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	//==============================================================================
	const String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const String getProgramName(int index) override;
	void changeProgramName(int index, const String& newName) override;

	//==============================================================================
	void getStateInformation(MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;

	void pushNextSampleIntoFifo(float sample);

	AudioProcessorValueTreeState::ParameterLayout createParameterLayout();


	//Frequency Analyzer variables.
	float fifo[GraphSection::fftSize];
	float fftData[GraphSection::fftSize * 2];
	int fifoIndex = 0;
	//Maybe try std::atomic<bool>?
	volatile bool nextFFTBlockReady = false;
	bool whichFifo = true;

private:
	AudioProcessorValueTreeState ProcessorVTS;
	//Smoothers
	std::array<LinearSmoothedValue<float>, TOTAL_FILTER_NUM> mVTS_FreqSmooth;
	std::array<LinearSmoothedValue<float>, TOTAL_FILTER_NUM> mVTS_QSmooth;
	std::array<LinearSmoothedValue<float>, VAR_FILTER_NUM> mVTS_GainSmooth;

	//Stores
	FilterStore FilterS;
	PointerStore PtrStore;

	//SampleRate.
	double processSampleRate;

	//Filter slopes
	float previousFilterType;
	float previousFilterSlope;

	//Pointers to params.
	std::array<float*, TOTAL_FILTER_NUM> mVTS_FreqPointers;
	std::array<float*, TOTAL_FILTER_NUM> mVTS_QPointers;
	std::array<float*, VAR_FILTER_NUM> mVTS_GainPointers;
	std::array<float*, VAR_FILTER_NUM> mVTS_DyanmicBools;

	//Pointers to threshold params.
	std::array<float*, VAR_FILTER_NUM> mVTS_ThresholdLevelPointers;
	std::array<float*, VAR_FILTER_NUM> mVTS_ThresholdRatioPointers;
	std::array<float*, VAR_FILTER_NUM> mVTS_ThresholdDecayPointers;

	//Filter choices pointers.
	float* pFilterType = nullptr;
	float* pFilterSlope = nullptr;

	//Filter toggle pointers
	std::array<float*, TOTAL_FILTER_NUM> mVTS_FilterState;
	std::array<int, TOTAL_FILTER_NUM> previousFilterState;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicEq2AudioProcessor)
};
