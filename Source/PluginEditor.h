/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "MainGUI.h"

typedef AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
typedef AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
typedef AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;
//==============================================================================
/**
*/
class DynamicEq2AudioProcessorEditor  : public AudioProcessorEditor
{
public:
    DynamicEq2AudioProcessorEditor (DynamicEq2AudioProcessor&, AudioProcessorValueTreeState& vts, volatile bool* nextFFT, float (&fftProcData)[(GraphSection::fftSize * 2)]);
    ~DynamicEq2AudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
	void paintOverChildren(Graphics& g) override;

	void setFilterAreaAttach();


private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
	OwnedArray<Slider> sliderArray;
    DynamicEq2AudioProcessor& processor;
	AudioProcessorValueTreeState& editorVTS;




	ComboBox filterTypeBox;
	std::unique_ptr<ComboBoxAttachment> filterTypeAttach;
	OwnedArray<SliderAttachment> sliderAttachmentArray;

	ComboBox filterSlopeBox;
	std::unique_ptr<ComboBoxAttachment> filterSlopeAttach;



	ParameterSection ParamArea;
	GraphSection graphArea;
	ComponentBoundsConstrainer constrainer;
	TooltipWindow editorToolTip;
	const std::array<float, GraphSection::fftSize>* fifoPtr;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicEq2AudioProcessorEditor)
};
