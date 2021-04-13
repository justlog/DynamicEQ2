/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DynamicEq2AudioProcessor::DynamicEq2AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", AudioChannelSet::stereo(), true)
#endif
	), ProcessorVTS(*this, nullptr, Identifier("DynamicProcessing"), createParameterLayout())
	
#endif
{
	//Setting parameter pointer arrays.
	for (uint8 i = 0; i < (TOTAL_FILTER_NUM); ++i)
	{
		mVTS_FreqPointers[i] = ProcessorVTS.getRawParameterValue("freqnum" + String(i+1));
		mVTS_QPointers[i] = ProcessorVTS.getRawParameterValue("qValue" + String(i+1));
		mVTS_FilterState[i] = ProcessorVTS.getRawParameterValue("filterState" + String(i + 1));
		previousFilterState[i] = *mVTS_FilterState[i];

		//Setting smoothers to be equal to pointer's inital value.
		mVTS_FreqSmooth[i] = *(mVTS_FreqPointers[i]);
		mVTS_QSmooth[i] = *(mVTS_QPointers[i]);

		if (i < VAR_FILTER_NUM)
		{
			mVTS_ThresholdLevelPointers[i] = ProcessorVTS.getRawParameterValue(
				"varThreshLevel" + String(i+1));
			mVTS_ThresholdRatioPointers[i] = ProcessorVTS.getRawParameterValue(
				"varThreshRatio" + String(i+1));
			mVTS_ThresholdDecayPointers[i] = ProcessorVTS.getRawParameterValue(
				"varThreshDecay" + String(i+1));
			mVTS_DyanmicBools[i] = ProcessorVTS.getRawParameterValue(
				"dynGain" + String(i + 1));



			mVTS_GainPointers[i] = ProcessorVTS.getRawParameterValue("varGain" + String(i+1));
			mVTS_GainSmooth[i] = *(mVTS_GainPointers[i]);
		}

	}

	//Filter Choices pointers.
	pFilterType = ProcessorVTS.getRawParameterValue("filterType");
	previousFilterType = *pFilterType;
	pFilterSlope = ProcessorVTS.getRawParameterValue("filterSlope"); //Connect functionallity in processblock.
	previousFilterSlope = *pFilterSlope;
}

DynamicEq2AudioProcessor::~DynamicEq2AudioProcessor()
{
}

//==============================================================================
const String DynamicEq2AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DynamicEq2AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DynamicEq2AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DynamicEq2AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DynamicEq2AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DynamicEq2AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DynamicEq2AudioProcessor::getCurrentProgram()
{
    return 0;
}

void DynamicEq2AudioProcessor::setCurrentProgram (int index)
{
}

const String DynamicEq2AudioProcessor::getProgramName (int index)
{
    return {};
}

void DynamicEq2AudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void DynamicEq2AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
	dsp::ProcessSpec spec;
	processSampleRate = sampleRate;
	spec.sampleRate = sampleRate;
	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = getTotalNumInputChannels();

	for (uint8 param = 0; param < TOTAL_FILTER_NUM; ++param)//Setting up smoothing variables.
	{
		mVTS_FreqSmooth[param].reset(sampleRate, 0.05);
		mVTS_QSmooth[param].reset(sampleRate, 0.05);
		if (param < VAR_FILTER_NUM)
		{
			mVTS_GainSmooth[param].reset(sampleRate, 0.05);
		}
	}

	PtrStore.passPointers(mVTS_FreqSmooth, mVTS_QSmooth, mVTS_GainSmooth,
		mVTS_ThresholdLevelPointers, mVTS_ThresholdRatioPointers, mVTS_ThresholdDecayPointers, mVTS_DyanmicBools);//Passing pointers to filters.
	FilterS.passSmoothedVars(PtrStore);//Passing smoothed variables refernces to filters.
	FilterS.reset();//Resetting filter states.
	FilterS.prepare(spec);//Perparing filters.
}

void DynamicEq2AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DynamicEq2AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DynamicEq2AudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	dsp::ProcessContextReplacing<float> currentContext((dsp::AudioBlock<float>)buffer);//Context for filters that need it for processing.

	for (uint8 i = 0; i < TOTAL_FILTER_NUM; ++i)//Setting target values for smoothing variables.
	{
		mVTS_FreqSmooth[i].setTargetValue(*mVTS_FreqPointers[i]);
		mVTS_QSmooth[i].setTargetValue(*mVTS_QPointers[i]);
		if (*mVTS_FilterState[i] != previousFilterState[i])
		{
			FilterS.toggleFilter(i);
			previousFilterState[i] = *mVTS_FilterState[i];
		}

		if (i < VAR_FILTER_NUM)
		{
			mVTS_GainSmooth[i].setTargetValue(*mVTS_GainPointers[i]);
		}
	}

	FilterS.process(buffer, currentContext);//Filter Processing.

	
	for (int i = 0; i < buffer.getNumSamples(); ++i)//Pushing samples in the buffer into FIFO struct for FFT operation.
	{
		pushNextSampleIntoFifo(buffer.getSample(0, i));
	}

}

//==============================================================================
bool DynamicEq2AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* DynamicEq2AudioProcessor::createEditor()
{
	return new DynamicEq2AudioProcessorEditor(*this, ProcessorVTS, &nextFFTBlockReady, fftData);
}

//==============================================================================
void DynamicEq2AudioProcessor::getStateInformation (MemoryBlock& destData)
{
	//Storing plugin state into binary in XML form.
	auto state = ProcessorVTS.copyState();
	std::unique_ptr<XmlElement> xml(state.createXml());
	copyXmlToBinary(*xml, destData);
}

void DynamicEq2AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	//Loading plugin state from binary.
	std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

	if (xmlState.get() != nullptr &&
		xmlState->hasTagName(ProcessorVTS.state.getType()))
	{
		ProcessorVTS.replaceState(ValueTree::fromXml(*xmlState));
	}
}



AudioProcessorValueTreeState::ParameterLayout DynamicEq2AudioProcessor::createParameterLayout()
{
	std::vector<std::unique_ptr<AudioParameterFloat>> params;

	//Lowpass filter.
	params.push_back(std::make_unique<AudioParameterFloat>("freqnum1",
															"Frequency 1",
															NormalisableRange<float>(20.f, 22000.f, 0.01, 0.5),
															20.f));

	//Highpass filter.
	params.push_back(std::make_unique<AudioParameterFloat>("freqnum2",
															"Frequency 2",
															NormalisableRange<float>(20.f, 22000.f, 0.01, 0.5),
															22000.f));
	for (int i = 3; i < (TOTAL_FILTER_NUM + 1); ++i)//Frequency params.
	{
		{
			params.push_back(std::make_unique<AudioParameterFloat>("freqnum" + String(i),
																	"Frequency " + String(i),
																	NormalisableRange<float>(20.f, 22000.f, 0.01, 0.5),
																	440.f));
		}
	}

	for (int i = 1; i < (TOTAL_FILTER_NUM + 1); ++i)//Q params.
	{
		{
			params.push_back(std::make_unique<AudioParameterFloat>("qValue" + String(i),
																	"Q " + String(i),
																	NormalisableRange<float>(0.01f, 40.f, 0.01f, 0.4f),
																	1.0f));
		}
	}

	for (uint8 i = 1; i < (VAR_FILTER_NUM + 1); ++i)//Dynamics params.
	{
		params.push_back(std::make_unique<AudioParameterFloat>("varGain" + String(i),
			"Peak Gain " + String(i),
			NormalisableRange<float>(0.0001f, 1.0f, 0.001f),
			1.3f));
		params.push_back(std::make_unique<AudioParameterFloat>("varThreshLevel" + String(i),
			"Threshold Level " + String(i),
			NormalisableRange<float>(0.0001f, 1.0f, 0.0001f, 0.28f),
			1.0f));
		params.push_back(std::make_unique<AudioParameterFloat>("varThreshRatio" + String(i),
			"Threshold Ratio " + String(i),
			NormalisableRange<float>(0.001f, 1.0f, 0.001f, 1.0f),
			1.0f));
		params.push_back(std::make_unique<AudioParameterFloat>("varThreshDecay" + String(i),
			"Threshold Decay " + String(i),
			NormalisableRange<float>(0.0001f, 1.0f, 0.0001f),
			0.0010f));
	}

	auto parameterLayout = AudioProcessorValueTreeState::ParameterLayout(params.begin(), params.end());

	parameterLayout.add(
		std::make_unique<AudioParameterChoice>("filterType",
			"Filter Type",
			StringArray{ "Peak", "Highshelf", "Lowshelf" },
			0)
						);

	parameterLayout.add(
		std::make_unique<AudioParameterChoice>("filterSlope",
			"Filter Slope",
			StringArray{ "12 dB/Octave",
						"24 dB/Octave",
						"36 dB/Octave",
						"48 dB/Octave",
						"72 dB/Octave",
						"96 dB/Octave" },
			0)
						);

	for (uint8 i = 1; i < (TOTAL_FILTER_NUM + 1); ++i)
	{
		parameterLayout.add(std::make_unique<AudioParameterBool>("filterState" + String(i),
			"Filter " + String(i) + " ON/OFF",
			false));
	}
	for (uint8 i = 1; i < (VAR_FILTER_NUM + 1); ++i)
	{
		parameterLayout.add(std::make_unique<AudioParameterBool>("dynGain" + String(i),
			"Dynamic Gain " + String(i) + " ON/OFF",
			false));
	}

	return parameterLayout;

}



void DynamicEq2AudioProcessor::pushNextSampleIntoFifo(float sample)
//Pushes samples into a FIFO structure to perform FFT on.
{
	if (fifoIndex == (GraphSection::fftSize))
	{
		if (!nextFFTBlockReady)
		{
			zeromem(fftData, sizeof(fftData));
			memcpy(fftData, fifo, sizeof(fifo));
			nextFFTBlockReady = true;
		}
		fifoIndex = 0;
	}

	fifo[fifoIndex++] = sample;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DynamicEq2AudioProcessor();
}
