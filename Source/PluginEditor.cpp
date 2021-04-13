/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
//==============================================================================
DynamicEq2AudioProcessorEditor::DynamicEq2AudioProcessorEditor(DynamicEq2AudioProcessor& p, AudioProcessorValueTreeState& vts, volatile bool* nextFFT,
																float (&fftProcData)[(GraphSection::fftSize * 2)])
	: AudioProcessorEditor(&p), editorVTS(vts), processor(p), graphArea(nextFFT, fftProcData, editorVTS.processor.getSampleRate()),
	editorToolTip(nullptr, 500)
{
	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.
	constrainer.setFixedAspectRatio(2.392);
	constrainer.setMinimumSize(1471, 615);
	addAndMakeVisible(ParamArea);
	addAndMakeVisible(graphArea);
	setFilterAreaAttach();





    setSize (1495, 625);
	//YOU CHANGED THE RESIZE METHOD INSIDE FILTERAREA AND YOU ADDED THE CODE ABOVE THIS. MAKE SURE TO BE AWARE OF THE CHANGES.

}

DynamicEq2AudioProcessorEditor::~DynamicEq2AudioProcessorEditor()
{
}

//==============================================================================
void DynamicEq2AudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
	paintOverChildren(g);
}

void DynamicEq2AudioProcessorEditor::paintOverChildren(Graphics& g)
{
	g.setColour(Colours::black);
	g.setOpacity(0.7f);
	const auto graphRect = graphArea.getLocalBounds();
	g.drawRect(graphRect, 3.0f);
	Point<int> overlapOrigin = graphRect.getBottomLeft();
	Rectangle<int> overlapRect(overlapOrigin.getX(),
							   overlapOrigin.getY(),
							   graphRect.getWidth(),
							   10);
	auto overlapGradient = ColourGradient::vertical(Colours::darkred.withMultipliedAlpha(0.1f),
													Colours::violet.darker(1.3f),
													overlapRect);
	overlapGradient.multiplyOpacity(0.4f);
	g.setGradientFill(overlapGradient);
	g.fillRect(overlapRect);
}

void DynamicEq2AudioProcessorEditor::resized()
{
	setResizable(true, true);

	constrainer.checkComponentBounds(this);//Constraining component size.
	auto localRect = getLocalBounds();
	//Divying up size of window for componenets.
	ParamArea.setBounds(localRect.removeFromBottom(getHeight() / 2.5));
	graphArea.setBounds(localRect);
}

void DynamicEq2AudioProcessorEditor::setFilterAreaAttach()
{
	//Passing down strings to create attachments for UI components.
	int ConstFilterOffset = CONST_FILTER_NUM - 1;
	for (uint8 area = 0; area < TOTAL_FILTER_NUM; ++area)
	{
		if (area < CONST_FILTER_NUM)
		{
			ParamArea.fArea_Owned[area]->attachSliders_Var(StringArray{ "freqnum" + String(area + 1)
																	 //  "qValue" + String(area + 1) 
																		},
																		&editorVTS);
			ParamArea.fArea_Owned[area]->attachToggle_Var(StringArray{"filterState" + String{area + 1} },
																	   &editorVTS);
		}
		else
		{
			ParamArea.fArea_Owned[area]->attachSliders_Var(StringArray{ "varGain"		 + String(area - ConstFilterOffset),
																		"freqnum"		 + String(area + 1),
																		"qValue"		 + String(area + 1),
																		"varThreshLevel" + String(area - ConstFilterOffset),
																		"varThreshRatio" + String(area - ConstFilterOffset) },
																		&editorVTS);
			ParamArea.fArea_Owned[area]->attachToggle_Var(StringArray{"dynGain" + String{area - ConstFilterOffset },
																	  "filterState" + String{area + 1} },
																	   &editorVTS);
		}
	}
}
