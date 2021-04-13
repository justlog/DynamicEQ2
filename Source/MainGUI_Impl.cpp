#include "MainGUI.h"
//#include "BinaryData.h"
GraphSection::GraphSection(volatile bool* fftBlockBool, float (&fftProcessData)[ (fftSize * 2) ], double inputSampleRate) : forwardFFT(fftOrder),
								window(fftSize, dsp::WindowingFunction<float>::blackmanHarris), nextFFTBlockReady(fftBlockBool), fftData(fftProcessData), 
								sampleRate(inputSampleRate)
{
	setOpaque(true);
	startTimerHz(30);
}

void GraphSection::timerCallback()
{

	if (*nextFFTBlockReady)
	{
		drawNextFrameOfSpectrum();
		*nextFFTBlockReady = false;
		repaint();
	}
}

void GraphSection::drawNextFrameOfSpectrum()
{
    window.multiplyWithWindowingTable (fftData, fftSize);      // [1]
    forwardFFT.performFrequencyOnlyForwardTransform (fftData); // [2]
    auto mindB = -100.0f;
    auto maxdB =    0.0f;
    for (int i = 0; i < scopeSize; ++i)                        // [3]
    {
        auto skewedProportionX = 1.0f - std::exp (std::log (1.0f - i / (float) scopeSize) * 0.2f);
        auto fftDataIndex = jlimit (0, fftSize / 2, (int) (skewedProportionX * fftSize / 2));
        auto level = jmap (jlimit (mindB, maxdB, Decibels::gainToDecibels (fftData[fftDataIndex])
                                               - Decibels::gainToDecibels ((float) fftSize)),
                           mindB, maxdB, 0.0f, 1.0f);
        scopeData[i] = level;                                  // [4]
    }
}

void GraphSection::drawFrame(Graphics& g)
{
	auto width  = getLocalBounds().getWidth();
	auto height = getLocalBounds().getHeight();

	for (int i = 1; i < scopeSize; ++i)
	{
		g.setColour(Colours::white);
		g.drawLine ({ (float) jmap (i - 1, 0, scopeSize - 1, 0, width),
							  jmap (scopeData[i - 1], 0.0f, 1.0f, (float) height, 0.0f),
					  (float) jmap (i,     0, scopeSize - 1, 0, width),
							  jmap (scopeData[i],     0.0f, 1.0f, (float) height, 0.0f)});
	}

}

void GraphSection::paint(Graphics& g) 
{
	g.fillAll(Colours::black);

	g.setColour(Colours::darkred);
	g.setOpacity(0.1f);
	g.fillAll();


	g.setColour(Colours::white);
	g.setOpacity(1.0f);
	drawFrame(g);
}


//============================================================================================
//============================================================================================
ParameterSection::ParameterSection()
{
	for (uint8 area = 0; area < TOTAL_FILTER_NUM; ++area)
	{
		if (area < CONST_FILTER_NUM)
		{
			fArea_Owned.add(new FilterArea(1, 1, &guiImgs));
		}
		else
		{
			fArea_Owned.add(new FilterArea(5, 2, &guiImgs));
		}
		addAndMakeVisible(*fArea_Owned[area]);
	}
}

void ParameterSection::resized()
{
	auto r = getLocalBounds();
	int areaWidth = getWidth();
	for (uint8 area = 0; area < TOTAL_FILTER_NUM; ++area)
	{
		if (area == 1)
		{
			continue;
		}
		fArea_Owned[area]->setBounds(r.removeFromLeft(areaWidth / TOTAL_FILTER_NUM));
	}
	fArea_Owned[1]->setBounds(r);
}

void ParameterSection::paint(Graphics& g)
{
	g.setGradientFill(ColourGradient::vertical(Colours::violet.darker(0.4f), Colours::dimgrey, getLocalBounds()));
	g.fillAll();
	g.beginTransparencyLayer(0.6f);
	g.setColour(Colours::black);
	g.fillAll();
	g.endTransparencyLayer();
	g.beginTransparencyLayer(0.2f);
	g.setColour(Colours::red.darker(0.6f));
	g.fillAll();
	g.endTransparencyLayer();

}



//============================================================================================
//============================================================================================

FilterArea::FilterArea()
{
	for (uint8 i = 0; i < sliderNum; ++i)
	{
		sliderArray.add(new Slider());
		addAndMakeVisible(*sliderArray[i]);
		sliderArray[i]->setTextBoxStyle(Slider::TextBoxBelow, false, sliderArray[i]->getWidth(), sliderArray[i]->getHeight() / 8);
		sliderArray[i]->setPopupDisplayEnabled(true, true, getParentComponent());
		sliderArray[i]->setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
	}
}

FilterArea::FilterArea(int numSliders, int numToggles, const GUI_Images* ImgPtr) : sliderNum(numSliders), toggleNum(numToggles), myImgPtr(ImgPtr)
{
	//Filter order is:
	/*1.varGain
	  2.freqnum
	  3.qValue
	  4.varThreshLevel
	  5.varThreshRatio
	  */
	/*If 2 sliders:
	  1. freqnum
	  2. qValue*/

	myLookAndFeel.setColour(Slider::ColourIds::thumbColourId, Colours::crimson.withMultipliedAlpha(0.85f));
	myLookAndFeel.setColour(Slider::ColourIds::rotarySliderFillColourId, Colours::firebrick.darker(0.5f));
	setLookAndFeel(&myLookAndFeel);
	
	//Adding sliders to sliderArray..
	for (uint8 i = 0; i < sliderNum; ++i)
	{
		if (sliderNum == 5 && (i == 0 || i == 3)) //Gain Sliders.
		{
			sliderArray.add(new DBSlider()); 
		}
		else if (sliderNum == 5 && i == 4)  //Ratio Sliders.
		{
			sliderArray.add(new RatioSlider()); 
			sliderArray[i]->setTooltip("Amount of attenuation as a precentage of the signal crosssing the threshold.");
		}
		else { sliderArray.add(new Slider()); }
		addAndMakeVisible(*sliderArray[i]);
		sliderArray[i]->setPopupDisplayEnabled(true, true, getParentComponent());
		sliderArray[i]->setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
	}

	//Setting ToggleButton for DynamicGain Toggle.
	for (uint8 i = 0; i < toggleNum; ++i)
	{
		toggleArray.add(new ToggleButton());
		addAndMakeVisible(*toggleArray[i]);
	}


	//Setting up tooltips and labmdas for toggles..
	switch (numToggles)
	{
	case 1:
		toggleArray[0]->setTooltip("ON\\OFF");
		toggleArray[0]->onStateChange = [this]()
		{
			repaint();
		};
		break;
	case 2:
		toggleArray[0]->setTooltip("Dynamic " + String(CharPointer_UTF8 ("\xe2\x9c\x93")) +
								   "\nStatic " + String(CharPointer_UTF8 ("\xe2\xa8\xaf ")));
		toggleArray[1]->setTooltip("ON\\OFF");

		toggleArray[0]->onClick = [this]()
		{
			repaint();
		};
		toggleArray[1]->onClick = [this]()
		{
			repaint();
		};
		break;
	default:
		break;
	}

	switch (numSliders)
	{
	case 1:
		sliderArray[0]->setTextValueSuffix(" Hz");
		break;
	case 2:
		sliderArray[0]->setTextValueSuffix(" Hz");
		break;

	case 5:
		sliderArray[1]->setTextValueSuffix(" Hz");
		sliderArray[3]->setTooltip("Determines the threshold from which the dynamic attenuation will start working.");
		DBG("All is well, 5 sliders.");
		break;
	default:
		DBG("sliderNum is not viable -- FilterArea constructor with numSliders.");
	}
}

FilterArea::~FilterArea()
{
	setLookAndFeel(nullptr);
}

void FilterArea::paint(Graphics& g)
{
	g.setColour(Colours::black);
	g.setOpacity(0.8f);
	g.drawRect(getLocalBounds(), 1.6f);


	//Painting according to number of Sliders.
	if (sliderNum == 1)
	{
		g.setOpacity(0.4f);
		g.drawImage(myImgPtr->freqImage, sliderArray[0]->getBounds().toFloat().reduced(10));
		setOnOffColour(g, toggleArray[0]->getToggleState());
	}
	else if (sliderNum == 2)
	{
		//Drawing dividing line between sliders
		auto localR = getLocalBounds();
		auto localP = Point<float>(localR.getX(), localR.getY() + (localR.getHeight() / 2));
		drawDividingLine(g, localP, localP.withX(localP.getX() + localR.getWidth()), 5.0f, Colours::black.withAlpha(0.5f));


		//Drawing slider images..
		g.setOpacity(0.2f);
		g.drawImage(myImgPtr->freqImage, sliderArray[0]->getBounds().toFloat().reduced(10));
		setOnOffColour(g, (toggleArray[0]->getToggleState()));

	}
	else if(sliderNum == 5)
	{
		//Drawing slider images..
		g.setOpacity(0.2f);
		g.drawImage(myImgPtr->freqImage, sliderArray[1]->getBounds().toFloat().reduced(10));

		g.setOpacity(0.4f);
		auto gainRect = sliderArray[0]->getBounds().toFloat().reduced(30);
		auto gainRectCenter = gainRect.getCentre();
		g.drawImage(myImgPtr->gainImage, gainRect.withCentre(gainRectCenter.withY(gainRectCenter.getY() - 5.0f)));

		auto qRect = sliderArray[2]->getBounds().toFloat().reduced(20);
		auto qRectCenter = qRect.getCentre();
		g.drawImage(myImgPtr->qImage, qRect.withCentre(qRectCenter.withY(qRectCenter.getY() - 6.5f)));

		//Setting colours according to the filter's current state..
		setOnOffColour(g, (toggleArray[1]->getToggleState()));
		setDynamicColour(g, toggleArray[0]->getToggleState(), sliderArray[3]);
		setDynamicColour(g, toggleArray[0]->getToggleState(), sliderArray[4]);
		setDynamicColour(g, !(toggleArray[0]->getToggleState()), sliderArray[0]);
	}
	//Filter order is:
	/*1.varGain
	  2.freqnum
	  3.qValue
	  4.varThreshLevel
	  5.varThreshRatio
	  */
	/*if 2 sliders:
	  1. freqnum
	  2. qValue*/


}

void FilterArea::drawDividingLine(Graphics& g, const Point<float>& startPoint, const Point<float>& endPoint, const int thickness, const Colour& c)
{
	//Takes a starting point and end point and draws a line between them in accordance to the given thickness and colour.
	auto divLine = Line<float>(startPoint, endPoint);
	g.setColour(c);
	g.drawLine(divLine, thickness);
}

void FilterArea::setOnOffColour(Graphics& g, const bool state)
{
	//Setting LookAndFeel according to the filter's state.
	if (state)
	{
		myLookAndFeel.setColour(Slider::ColourIds::thumbColourId, Colours::crimson.withMultipliedAlpha(0.85f));
		myLookAndFeel.setColour(Slider::ColourIds::rotarySliderFillColourId, Colours::firebrick.darker(0.5f));
	}
	else
	{
		g.setColour(Colours::black);
		g.setOpacity(0.3f);
		auto localBounds = getLocalBounds();
		localBounds.expand(20, 20);
		g.fillRect(localBounds);
		myLookAndFeel.setColour(Slider::ColourIds::thumbColourId, Colours::dimgrey);
		myLookAndFeel.setColour(Slider::ColourIds::rotarySliderFillColourId, Colours::dimgrey);
	}
}
void FilterArea::setDynamicColour(Graphics& g, const bool state, Slider* sliderPtr)
{
	//Setting up specific sliders according to the filter's dynamic toggle..
	if (state)
	{
		sliderPtr->setColour(Slider::ColourIds::thumbColourId ,getLookAndFeel().findColour(Slider::ColourIds::thumbColourId));
		sliderPtr->setColour(Slider::ColourIds::rotarySliderFillColourId ,getLookAndFeel().findColour(Slider::ColourIds::rotarySliderFillColourId));
	}
	else
	{
		sliderPtr->setColour(Slider::ColourIds::thumbColourId, Colours::dimgrey);
		sliderPtr->setColour(Slider::ColourIds::rotarySliderFillColourId, Colours::dimgrey);
	}
}

void FilterArea::resized()
{
	setSlidersSize();
	for (uint8 slider = 0; slider < sliderNum; ++slider)
	{
		sliderArray[slider]->setTextBoxStyle(Slider::TextBoxBelow, false, sliderArray[slider]->getWidth(), (sliderArray[slider]->getHeight() / 6));
	}
}

void FilterArea::setSliders()
{
	for (uint8 i = 0; i < sliderNum; ++i)
	{
		sliderArray.add(new Slider());
		addAndMakeVisible(*sliderArray[i]);
		sliderArray[i]->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
	}

}

void FilterArea::setSlidersSize()
{
	//Setting up the sizes of the elements inside of FilterArea.


	//Filter order is:
	/*1.varGain
	  2.freqnum
	  3.qValue
	  4.varThreshLevel
	  5.varThreshRatio
	  */
	/*if 2 sliders:
	  1. freqnum
	  2. qValue*/


	//Getting localBounds and width for reference.
	Rectangle<int> r = getLocalBounds();
	int areaWidth = r.getWidth();

	switch (sliderNum)
	{
		//Chopping up the area given from parent according to the number of sliders
		//and setting bounds for elements with chopped pieces.
	case 1:
	{
		Rectangle<int> onOffSection = r.removeFromBottom(r.getHeight() / 8);
		sliderArray[0]->setBounds(r);
		toggleArray[0]->setBounds(onOffSection);
	}
		break;

	case 2:
	{
		Rectangle<int> topFreqSliderArea = r.removeFromTop(r.getHeight() / 2);
		Rectangle<int> bottomQSliderArea = r;
		Rectangle<int> onOffSection = bottomQSliderArea.removeFromBottom(bottomQSliderArea.getHeight() / 4);



		toggleArray[0]->setBounds(onOffSection);
		sliderArray[0]->setBounds(topFreqSliderArea);
		sliderArray[1]->setBounds(bottomQSliderArea);
	}
		break;

	case 3:
	{
		Rectangle<int> topGainSliderArea = r.removeFromTop(r.getHeight() / 2);
		Rectangle<int> bottomFreqSliderArea = r.removeFromLeft(r.getWidth() / 2);
		Rectangle<int> bottomQSliderArea = r;

		sliderArray[0]->setBounds(topGainSliderArea);
		sliderArray[1]->setBounds(bottomFreqSliderArea);
		sliderArray[2]->setBounds(bottomQSliderArea);
	}
		break;

	case 5:
	{
		Rectangle<int> onOffSection = r.removeFromBottom(r.getHeight() / 8);
		Rectangle<int> topSection = r.removeFromTop(r.getHeight() / 2);
		Rectangle<int> topStaticGainSliderArea = topSection.removeFromLeft(getWidth() / 2);
		Rectangle<int> topRatioSliderArea = topSection.removeFromTop(topSection.getHeight() / 2);
		Rectangle<int> topThresholdSliderArea = topSection;
		Rectangle<int> bottomFreqSliderArea = r.removeFromLeft(r.getWidth() / 2);
		Rectangle<int> bottomQSliderArea = r;


		sliderArray[0]->setBounds(topStaticGainSliderArea);
		sliderArray[1]->setBounds(bottomFreqSliderArea);
		sliderArray[2]->setBounds(bottomQSliderArea);
		sliderArray[3]->setBounds(topThresholdSliderArea);
		sliderArray[4]->setBounds(topRatioSliderArea);

		Rectangle<int> buttonArea = onOffSection.removeFromLeft(onOffSection.getWidth() / 2);
		toggleArray[1]->setBounds(buttonArea.removeFromRight(buttonArea.getWidth() / 2));
		toggleArray[0]->setBounds(buttonArea);
	}

		
		break;

	default:
		DBG("ERROR! sliderNum is ill-defined.");
	}

}


void FilterArea::attachSliders_Var(const StringArray& paramNames, AudioProcessorValueTreeState* editVTS)
{
	//Attaching Sliders to their parameters.
	for (uint8 param = 0; param < paramNames.size(); ++param)
	{
		attachArray.add(new AudioProcessorValueTreeState::SliderAttachment(*editVTS, paramNames[param], *sliderArray[param]));
	}
}

void FilterArea::attachToggle_Var(const StringArray& paramNames, AudioProcessorValueTreeState* editVTS)
{
	//Attaching ToggleButtons to their parameters.
	for (uint8 param = 0; param < paramNames.size(); ++param)
	{
		buttonAttachArray.add(new AudioProcessorValueTreeState::ButtonAttachment(*editVTS,
																				 paramNames[param],
																				 *toggleArray[param]));
	}
}


//====================================================================================================================================
//====================================================================================================================================
//====================================================================================================================================


Label* CustomLookAndFeel::createSliderTextBox(Slider& slider)
{
	//Setting up Label with no borders for SliderTextBox.
		Label* l = LookAndFeel_V4::createSliderTextBox(slider);
		l->setBorderSize(BorderSize<int>(0));
		l->setColour(Label::outlineColourId, Colours::transparentWhite);
		l->setColour(Label::outlineWhenEditingColourId, Colours::transparentWhite);
		auto sliderPoint = slider.getPosition();
		l->setTopLeftPosition(sliderPoint.getX(), slider.getBottom());
		return l;
}
