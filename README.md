# DynamicEQ2

## What is this?
A musical equalizer with dynamic processing written using C++ and the JUCE framework.

In short, Each frequency band is being attenuated according to a volume threshold set by the 
user. The equalizer is equipped with a 24dB/oct lowpass/highpass and 7  24dB/oct bell filters.

Example running inside a REAPER DAW:
[![IMAGE ALT TEXT HERE](https://i.imgur.com/3ah6uFo.png)](https://streamable.com/a3b2n7)

## How to try it out?
To try out the plugin, copy the .vst3 file located at the root of the repo into the plugin folder of your favorite DAW. 
