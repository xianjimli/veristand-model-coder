
/* evaluates a transfer function with numerator [1] and denominator [1 2 3]*/
static double engine_RPM_function(double input);

#define MAXIMUM( x, y) ((x)>(y)?(x):(y))

static double engine_RPM_function(double input)
{
	double *x = &(rtSignal.state1);
	double out, a11, a12, a21, b11, c12;

	a11 = readParam.a11;
	a12 = readParam.a12;
	a21 = readParam.a21;
	b11 = readParam.b11;
	c12 = readParam.c12;

	/* this is an Euler ODE solver at dt = 0.01 */
	x[0] += 0.01 * (a11 * x[0] + a12 * x[1] + b11 * input); 
	x[1] += 0.01 * a21 * x[0];

	out = c12 * x[1];

	if (!rtInport.command_EngineOn && out <= 0.0)
	{
		/* if engine is off and the RPM gets to zero (or less), 
		   then zero out the states so the engine will "stop"; 
		   otherwise, let the RPM gradually reach zero. */
		x[0] = 0.0;
		x[1] = 0.0;
	}
	
	/* Update the engineOn signal */
	rtSignal.engineOn = (int32_t)rtInport.command_EngineOn;

	/* never return an rpm value less than zero */
	rtSignal.RPM = MAXIMUM(out, 0.0);
	return rtSignal.RPM;
}

/* evaluates a first order model num = [175] den = [1 100] */
static double engine_temperature_function(double setPoint);
static double engine_temperature_function(double setPoint)
{
	double t1;

	if (readParam.temperature_timeConstant > 0)
		t1 = 1.0/readParam.temperature_timeConstant;
	else
		t1 = 0.0;
	
	rtSignal.engineTemperature += 0.01*(-t1*rtSignal.engineTemperature + setPoint); /* Euler ODE solver at dt = 0.01 */

	return t1 * rtSignal.engineTemperature; 
}

/* INPUT: *inData, pointer to inport data at the current timestamp, to be 
  	      consumed by the function
   OUTPUT: *outData, pointer to outport data at current time + baserate, to be
  	       produced by the function
   INPUT: timestamp, current simulation time */
int32_t USER_TakeOneStep(double *inData, double *outData, double timestamp) 
{
	double rpm_command, idleRPM = readParam.idleRPM, redlineRPM = readParam.redlineRPM, temperature_command;
	if (inData)
	{
		rtInport.command_RPM = inData[0];
		rtInport.command_EngineOn = (int32_t)inData[1];
	}
	else
	{
		rtInport.command_RPM = 0.0;
		rtInport.command_EngineOn = 0;
	}

	
	temperature_command = readParam.temperature_roomTemp;

	if (rtInport.command_EngineOn)
	{
		/* this simulates an "idle", i.e. a minimum rpm command when the engine is running */
		rpm_command = (rtInport.command_RPM > idleRPM ? rtInport.command_RPM : idleRPM); 

		/* determine if the temperature should move toward normal operating temp or redline temp */
		if (rtOutport.RPM < redlineRPM)
			temperature_command += readParam.temperature_operatingTempDelta;
		else
			temperature_command += readParam.temperature_redlineTempDelta;
	}
	else
	{
		/* when the engine is off, send the rpm_command to zero, letting the engine TF continue */
		rpm_command = 0.0; 
	}

	rtOutport.RPM = engine_RPM_function(rpm_command); /* don't let the RPM be less than zero */

	
	if (rtInport.command_EngineOn)
	{
	}

	rtOutport.engineTemperature = engine_temperature_function(temperature_command);
	
	if (outData)
	{
		outData[0] = rtOutport.RPM;	
		outData[1] = rtOutport.engineTemperature ;	
	}
	
	return NI_OK;
}