#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "FilterSupply.h"
#include "BinaryData.h"

typedef AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

//Struct containing binary resources images.
struct GUI_Images
{
public:
	Image freqImage{ImageFileFormat::loadFrom(BinaryData::freq_png, BinaryData::freq_pngSize)};
	Image gainImage{ImageFileFormat::loadFrom(BinaryData::Audioicon_png, BinaryData::Audioicon_pngSize)};
	Image qImage{ ImageFileFormat::loadFrom(BinaryData::QIcon_png, BinaryData::QIcon_pngSize) };
	Image PowerImage{ ImageFileFormat::loadFrom(BinaryData::PowerIcon_png, BinaryData::PowerIcon_pngSize) };
};

//A custom LookAndFeel class for color changes.
class CustomLookAndFeel : public LookAndFeel_V4
{

public:
	Label* createSliderTextBox(Slider& slider) override;

};

//Class for a gain slider, presenting values in dB.
class DBSlider : public Slider
{
	virtual String getTextFromValue(double value) override
	{
		if (value + 0.00001  >= 1.0)
		{
			//DBG("IN 1 == " + (String)value);
			return String{ "0.0 dB" };
		}
		else if (value - 0.0001 <= 0.0)
		{
			return String{ "-INF" };
		}
		return String{ Decibels::gainToDecibels(value) } << " dB";
	}

	virtual double getValueFromText(const String& value) override
	{
		String decibelText = value.upToFirstOccurrenceOf("dB", false, false).trim();
		double minusInfdB = -80.0;
		return decibelText.equalsIgnoreCase("-INF") ? minusInfdB : Decibels::decibelsToGain(decibelText.getDoubleValue());
	}
};

//Class for the Threshold ratio slider. presents values in precentages.
class RatioSlider : public Slider
{
	virtual String getTextFromValue(double value) override
	{
		return String{ value * 100 } + String{ " \%" };
	}

	virtual double getValueFromText(const String& value) override
	{
		String precentText = value.upToFirstOccurrenceOf("%", false, false).trim();
		return precentText.getDoubleValue() / 100.0;
	}
};

//Class for frequency analyzer.
class GraphSection : public Component, private Timer
{
public:
	enum
	{
		fftOrder = 12,
		fftSize = 1 << fftOrder,
		scopeSize = 1024
	};

	GraphSection(volatile bool* fftBlockBool, float (&fftProcessData)[ (fftSize * 2) ], double inputSampleRate);

	void paint(Graphics& g) override;

	void timerCallback() override;

	void drawNextFrameOfSpectrum();

	void drawFrame(Graphics& g);



private:
	dsp::FFT forwardFFT;
	dsp::WindowingFunction<float> window;
	double sampleRate;
	float* fftData;
	volatile bool* nextFFTBlockReady;
	float scopeData[scopeSize];

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphSection)

};


//Each FilterArea object represents a "block" of Sliders for a specific filter.
class FilterArea : public Component
{

public:
	FilterArea();

	FilterArea(int numSliders, int numToggles, const GUI_Images* ImgPtr);

	~FilterArea();

	void paint(Graphics& g) override;

	void drawDividingLine(Graphics& g, const Point<float>& startPoint, const Point<float>& endPoint, const int thickness, const Colour& c);
	
	void setOnOffColour(Graphics& g, const bool state);

	void setDynamicColour(Graphics& g, const bool state, Slider* sliderPtr);

	void resized() override;

	void setSliders();
	
	void setSlidersSize();

	void attachSliders_Var(const StringArray& paramNames, AudioProcessorValueTreeState* editVTS);

	void attachSliders_Const(const StringArray& ParamNames, AudioProcessorValueTreeState* editVTS);

	void attachToggle_Var(const StringArray& paramName, AudioProcessorValueTreeState* editVTS);

	


	uint8 sliderNum = 3;
	uint8 toggleNum = 1;

	OwnedArray<Slider> sliderArray;
private:
	OwnedArray<ToggleButton> toggleArray;
	OwnedArray<AudioProcessorValueTreeState::SliderAttachment> attachArray;
	OwnedArray<ButtonAttachment> buttonAttachArray;
	CustomLookAndFeel myLookAndFeel;
	const GUI_Images* myImgPtr;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterArea)
};

//This is the parent containing all of the FilterArea objects.
class ParameterSection : public Component
{
public:
	ParameterSection();

	void setFilterAreas();

	void resized() override;

	void paint(Graphics& g) override;

	//std::array<FilterArea, TOTAL_FILTER_NUM> fArea_Array;
	OwnedArray<FilterArea> fArea_Owned;
	GUI_Images guiImgs;
private:

};
