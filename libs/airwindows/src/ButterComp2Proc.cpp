/* ========================================
 *  ButterComp2 - ButterComp2.h
 *  Copyright (c) 2016 airwindows, All rights reserved
 * ======================================== */

#ifndef __ButterComp2_H
#include "ButterComp2.h"
#endif

namespace ButterComp2 {


void ButterComp2::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames) 
{
    float* in1  =  inputs[0];
    float* in2  =  inputs[1];
    float* out1 = outputs[0];
    float* out2 = outputs[1];

	double overallscale = 1.0;
	overallscale /= 44100.0;
	overallscale *= getSampleRate();

	double inputgain = pow(10.0,(A*14.0)/20.0);
	double compfactor = 0.012 * (A / 135.0);
	double output = B * 2.0;
	double wet = C;
	double dry = 1.0 - wet;
	double outputgain = inputgain;
	outputgain -= 1.0;
	outputgain /= 1.5;
	outputgain += 1.0;
    
    while (--sampleFrames >= 0)
    {
		long double inputSampleL = *in1;
		long double inputSampleR = *in2;
		double drySampleL = inputSampleL;
		double drySampleR = inputSampleR;
		
		inputSampleL *= inputgain;
		inputSampleR *= inputgain;
		
		long double divisor = compfactor / (1.0+fabs(lastOutputL));
		//this is slowing compressor recovery while output waveforms were high
		divisor /= overallscale;
		long double remainder = divisor;
		divisor = 1.0 - divisor;
		//recalculate divisor every sample		
		
		long double inputposL = inputSampleL + 1.0;
		if (inputposL < 0.0) inputposL = 0.0;
		long double outputposL = inputposL / 2.0;
		if (outputposL > 1.0) outputposL = 1.0;		
		inputposL *= inputposL;
		targetposL *= divisor;
		targetposL += (inputposL * remainder);
		long double calcposL = pow((1.0/targetposL),2);
		
		long double inputnegL = (-inputSampleL) + 1.0;
		if (inputnegL < 0.0) inputnegL = 0.0;
		long double outputnegL = inputnegL / 2.0;
		if (outputnegL > 1.0) outputnegL = 1.0;		
		inputnegL *= inputnegL;
		targetnegL *= divisor;
		targetnegL += (inputnegL * remainder);
		long double calcnegL = pow((1.0/targetnegL),2);
		//now we have mirrored targets for comp
		//outputpos and outputneg go from 0 to 1
		
		if (inputSampleL > 0)
		{ //working on pos
			if (flip)
			{
				controlAposL *= divisor;
				controlAposL += (calcposL*remainder);
				
			}
			else
			{
				controlBposL *= divisor;
				controlBposL += (calcposL*remainder);
			}
		}
		else
		{ //working on neg
			if (flip)
			{
				controlAnegL *= divisor;
				controlAnegL += (calcnegL*remainder);
			}
			else
			{
				controlBnegL *= divisor;
				controlBnegL += (calcnegL*remainder);
			}
		}
		//this causes each of the four to update only when active and in the correct 'flip'
		
		divisor = compfactor / (1.0+fabs(lastOutputR));
		//this is slowing compressor recovery while output waveforms were high
		divisor /= overallscale;
		remainder = divisor;
		divisor = 1.0 - divisor;
		//recalculate divisor every sample		
		
		long double inputposR = inputSampleR + 1.0;
		if (inputposR < 0.0) inputposR = 0.0;
		long double outputposR = inputposR / 2.0;
		if (outputposR > 1.0) outputposR = 1.0;		
		inputposR *= inputposR;
		targetposR *= divisor;
		targetposR += (inputposR * remainder);
		long double calcposR = pow((1.0/targetposR),2);
		
		long double inputnegR = (-inputSampleR) + 1.0;
		if (inputnegR < 0.0) inputnegR = 0.0;
		long double outputnegR = inputnegR / 2.0;
		if (outputnegR > 1.0) outputnegR = 1.0;		
		inputnegR *= inputnegR;
		targetnegR *= divisor;
		targetnegR += (inputnegR * remainder);
		long double calcnegR = pow((1.0/targetnegR),2);
		//now we have mirrored targets for comp
		//outputpos and outputneg go from 0 to 1
		
		if (inputSampleR > 0)
		{ //working on pos
			if (flip)
			{
				controlAposR *= divisor;
				controlAposR += (calcposR*remainder);
				
			}
			else
			{
				controlBposR *= divisor;
				controlBposR += (calcposR*remainder);
			}
		}
		else
		{ //working on neg
			if (flip)
			{
				controlAnegR *= divisor;
				controlAnegR += (calcnegR*remainder);
			}
			else
			{
				controlBnegR *= divisor;
				controlBnegR += (calcnegR*remainder);
			}
		}
		//this causes each of the four to update only when active and in the correct 'flip'
		
		long double totalmultiplierL;
		long double totalmultiplierR;
		if (flip)
		{
			totalmultiplierL = (controlAposL * outputposL) + (controlAnegL * outputnegL);
			totalmultiplierR = (controlAposR * outputposR) + (controlAnegR * outputnegR);
		}
		else
		{
			totalmultiplierL = (controlBposL * outputposL) + (controlBnegL * outputnegL);
			totalmultiplierR = (controlBposR * outputposR) + (controlBnegR * outputnegR);
		}
		//this combines the sides according to flip, blending relative to the input value
		
		inputSampleL *= totalmultiplierL;
		inputSampleL /= outputgain;
		
		inputSampleR *= totalmultiplierR;
		inputSampleR /= outputgain;
		
		if (output != 1.0) {
			inputSampleL *= output;
			inputSampleR *= output;
		}

		if (wet !=1.0) {
			inputSampleL = (inputSampleL * wet) + (drySampleL * dry);
			inputSampleR = (inputSampleR * wet) + (drySampleR * dry);
		}
		
		lastOutputL = inputSampleL;
		lastOutputR = inputSampleR;
		//we will make this factor respond to use of dry/wet
		
		flip = !flip;
		
		
		*out1 = inputSampleL;
		*out2 = inputSampleR;

		in1++;
		in2++;
		out1++;
		out2++;
    }
}

} // end namespace ButterComp2

