#define MAX_DELAY 10000

/* INPUT: *inData, pointer to inport data at the current timestamp, to be 
  	      consumed by the function
   OUTPUT: *outData, pointer to outport data at current time + baserate, to be
  	       produced by the function
   INPUT: timestamp, current simulation time */
int32_t USER_TakeOneStep(double *inData, double *outData, double timestamp) 
{
	int32_t valid_delay, idx;
	
	for (idx = MAX_DELAY - 1; idx > 0; idx--)
		rtState[idx] = rtState[idx - 1];

	if (inData) {
		rtInport.In1 = inData[0];
		rtState[0] = rtInport.In1;	
	}
							
	if (outData) {
		if (readParam.delay >= MAX_DELAY)
			valid_delay = MAX_DELAY - 1;
		else
			valid_delay = readParam.delay;
			
		outData[0] = rtState[valid_delay];	
	}
	
	return NI_OK;
}
