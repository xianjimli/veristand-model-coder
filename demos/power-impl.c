/* INPUT: *inData, pointer to inport data at the current timestamp, to be 
  	      consumed by the function
   OUTPUT: *outData, pointer to outport data at current time + baserate, to be
  	       produced by the function
   INPUT: timestamp, current simulation time */
#include <math.h>

int32_t USER_TakeOneStep(double *inData, double *outData, double timestamp) 
{
	double r = 0.1;//(random()%100)/100.0;
	if (inData) {
		rtInport.power_on = (int)inData[0];
		rtInport.output_voltage = inData[1];
		rtInport.output_current = inData[2];
	}

	rtOutport.output_voltage = rtInport.output_voltage + r;
	rtOutport.output_current = rtInport.output_current + r;

	if (outData) {
		outData[0] = rtOutport.output_voltage;	
		outData[1] = rtOutport.output_current;	
	}

	return NI_OK;
}
