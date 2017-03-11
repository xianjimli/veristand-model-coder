/*========================================================================*
 * NI VeriStand Model Framework 
 * Core interface
 *
 * Abstract:
 *      The ni_modelframework.c file implements the NI VeriStand Model Framework interface 
 *      to a model. The intent of this source is to serve as an external
 *      module to a user implemented model. 
 *	
 *      Function names prefixed with "NIRT_" are Model Framework entries and are 
 *      used to communicate in and out of the model. Customizations to these functions 
 *      should be implemented in their similarly named functions prefixed with "USER_".
 *
 *      For an example in using this module with a user-implemented model source,  
 *      refer to the application between Template.c and ni_modelframework.c 
 *
 *========================================================================*/
 
 /* Model Framework API */
#include "ni_modelframework.h"

/* model.h is user generated and declares the Parameters type  */
#include "model.h"

/*
 * NI VeriStand Model Framework API version
 * Use NIRT_GetModelFrameworkVersion() instead to retrieve
 * version information.
*/
NI_Version NIVS_APIversion DataSection(".NIVS.APIVersion") = {NIMF_VER_MAJOR, NIMF_VER_MINOR, NIMF_VER_FIX, NIMF_VER_BUILD};
															  
#ifdef VXWORKS
	# include <ctype.h>
	
	HANDLE CreateSemaphore(void* lpSemaphoreAttributes, int lInitialCount, int lMaximumCount, char* lpName)
	{
		UNUSED_PARAMETER(lpSemaphoreAttributes);
		UNUSED_PARAMETER(lInitialCount);
		UNUSED_PARAMETER(lMaximumCount);
		UNUSED_PARAMETER(lpName);
		
		return semBCreate(SEM_Q_PRIORITY, SEM_FULL);
	}
	
	void WaitForSingleObject(HANDLE hHandle, int dwMilliseconds)
	{
		int ticks = INFINITE; 
		
		if (dwMilliseconds != INFINITE) 
		{
			ticks = (CLOCKS_PER_SEC * (dwMilliseconds/1000)) + 1;
		}
		
		semTake(hHandle, ticks); 
	}
	
	void ReleaseSemaphore(HANDLE hSemaphore, int lReleaseCount, void* lpPreviousCount)
	{
		UNUSED_PARAMETER(lReleaseCount);
		UNUSED_PARAMETER(lpPreviousCount);
		
		semGive(hSemaphore);
	}
	
	void CloseHandle(HANDLE hObject)
	{
		semFlush(hObject);
		semDelete(hObject);
	}
	
#elif kNIOSLinux
	# include <ctype.h>
	
	HANDLE CreateSemaphore(void* lpSemaphoreAttributes, int lInitialCount, int lMaximumCount, char* lpName)
	{
		UNUSED_PARAMETER(lpSemaphoreAttributes);
		UNUSED_PARAMETER(lpName);
		UNUSED_PARAMETER(lMaximumCount);
		
		HANDLE sem = (HANDLE) malloc(sizeof(sem_t));
		int err = sem_init(sem, 0, lInitialCount);
		
		if(err == 0)
		{
			return sem;
		}
		else
		{
			free(sem);
			return NULL;
		}
	}
	
	void WaitForSingleObject(HANDLE hHandle, int dwMilliseconds)
	{
		if (dwMilliseconds == INFINITE)
		{
			sem_wait(hHandle);
		}
		else
		{
			struct timespec ts;
			ts.tv_nsec = dwMilliseconds * 10^6;
			sem_timedwait(hHandle, &ts);
		}
	}
	
	void ReleaseSemaphore(HANDLE hSemaphore, int lReleaseCount, void* lpPreviousCount)
	{
		UNUSED_PARAMETER(lReleaseCount);
		UNUSED_PARAMETER(lpPreviousCount);
		sem_post(hSemaphore);
	}
	
	void CloseHandle(HANDLE hObject)
	{
		sem_destroy(hObject);
		free(hObject);
	}
#endif

#define EXT_IN		0
#define EXT_OUT		1

Parameters rtParameter[2];
int32_t READSIDE = 0;
unsigned char ReadSideDirtyFlag = 0, WriteSideDirtyFlag = 0;
int32_t NumTasks DataSection(".NIVS.numtasks") = 1;

struct { 
	int32_t stopExecutionFlag;
	const char *errmsg;
	HANDLE flip;
	uint32_t inCriticalSection;
	int32_t SetParamTxStatus;
	double timestamp;
} NIRT_system;

 /*========================================================================*
 * Model specifications
 * Defined externally by the user's model source.
 ========================================================================*/
extern double USER_BaseRate;
extern const char * USER_ModelName;
extern const char * USER_Builder;
extern int32_t ParameterSize;
extern int32_t SignalSize;
extern int32_t ExtIOSize;
extern int32_t InportSize;
extern int32_t OutportSize;
extern NI_ExternalIO rtIOAttribs[];
extern NI_Parameter rtParamAttribs[];
extern int32_t ParamDimList[];
extern NI_Signal rtSignalAttribs[];
extern int32_t SigDimList[];
extern Parameters initParams;

 /*========================================================================*
 * Function: SetErrorMessage
 *
 * Abstract:
 *	Sets and prints the system's error or warning message to stderr. To clear the last message, you must first set ErrMsg to NULL.
 *
 * Parameters:
 *      ErrMsg : error or warning string. First set to NULL to clear the last error message
 *      isError : if true, the NIRT_system.stopExecutionFlag is enabled
 *
 * Returns:
 *      (void)
========================================================================*/
void SetErrorMessage(char *ErrMsg, int32_t isError)
{
	/* If a fatal error and one hasn't yet occurred */
	if (isError && (NIRT_system.stopExecutionFlag == 0)) 
	{
		/* Set the stop flag and forcefully set the error message by setting internal variable to NULL.*/
		NIRT_system.stopExecutionFlag = 1;
		NIRT_system.errmsg = NULL;
	}
	
	/* Only set the error message if it hasn't been set before. */
	if (NIRT_system.errmsg == NULL)
	{
		NIRT_system.errmsg = ErrMsg;
	}
	
	/* Print the error\warning message */
	if (NIRT_system.errmsg != NULL)
	{
		if(isError)
		{
			(void)fprintf(stderr,"VeriStand Error: %s\n", NIRT_system.errmsg);
		}
		else
		{
			(void)fprintf(stderr,"VeriStand Warning: %s\n", NIRT_system.errmsg);
		}
	}
}

 /*========================================================================*
 * Function: NIRT_GetModelFrameworkVersion
 *
 * Abstract:
 *	Returns the version of the NI VeriStand Model Framework
 *
 * Output Parameters
 *	major : major version number
 *	minor : minor version number
 *	fix   : fix version number
 *	build : build version number
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetModelFrameworkVersion(uint32_t* major, uint32_t* minor, uint32_t* fix, uint32_t* build)
{
	*major = NIVS_APIversion.major;
	*minor = NIVS_APIversion.minor;
	*fix = NIVS_APIversion.fix;
	*build = NIVS_APIversion.build;
	
	return NI_OK;
}

 /*========================================================================*
 * Function: NIRT_ModelStart
 *
 * Abstract:
 *	Executes before the first time step and only once within a model's execution.
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_ModelStart(void)
{
	return USER_ModelStart();
}

 /*========================================================================*
 * Function: NIRT_ModelError
 *
 * Abstract:
 *	Check for any simulation errors. 
 * 
 * Input/Output Parameters:
 *	msglen	: (in) length of input string (out) length of output string 
 * 
 * Output Parameters: 
 *	errmsg	: string containing error message (if any occurred). 
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_ModelError(char* Errmsg, int32_t* msglen)
{
	int32_t retVal = NI_OK;
	const char *simStoppedMsg = "The model simulation was stopped, but no reason was specified. This may be expected behavior.";
	
	if (NIRT_system.errmsg != NULL)
	{
		/* Set error condition */
		retVal = NI_ERROR;
		
		if (*msglen > 0) 
		{
			if (*msglen > (int32_t)strlen(NIRT_system.errmsg)) 
			{
				/* Get error message */
				*msglen = strlen(NIRT_system.errmsg);
			}
			
			/* Return error message */
			strncpy(Errmsg, NIRT_system.errmsg, *msglen);
		}
	}
	else if(NIRT_system.stopExecutionFlag == 1)
	{
		/* No error */
		retVal = NI_OK; 
		
		/* Execution stopped, but without a message. Return generic message. */
		if (*msglen > 0) 
		{
			if (*msglen > (int32_t)strlen(simStoppedMsg)) 
			{
				/* Get error message */
				*msglen = strlen(simStoppedMsg);
			}
			
			/* Return error message */
			strncpy(Errmsg, simStoppedMsg, *msglen);
		}
	}
	else
	{
		/* No error */
		retVal = NI_OK; 
		*msglen = 0;
	}
	
	return retVal;	
}

 /*========================================================================*
 * Function: NIRT_InitializeModel
 *
 * Abstract:
 *	Initializes internal structures and prepares model for execution. 
 * 
 * Input Parameters:
 *	finaltime	: the final time until which the model should run.
 * 
 * Output Parameters:
 *	outTimeStep	: the base time step of the model scheduler.
 *	num_in		: number of external inputs
 *	num_out		: number of external outputs
 *	num_tasks	: number of Tasks (for multirate models only). 
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_InitializeModel(double finaltime, double *outTimeStep, int32_t *num_in, int32_t *num_out, int32_t* num_tasks) 
{		
	NIRT_system.SetParamTxStatus = NI_OK;
	NIRT_system.timestamp = 0.0;
	
	/* Initialize parameter buffers */
	memcpy(&rtParameter[0], &initParams, sizeof(Parameters));
	memcpy(&rtParameter[1], &initParams, sizeof(Parameters));
	
	NIRT_system.flip = CreateSemaphore(NULL, 1, 1, NULL);
	if (NIRT_system.flip == NULL)
	{
		SetErrorMessage("Failed to create semaphore.", 1);
	}
	
	/* Return model specification */
	NIRT_GetModelSpec(NULL, 0, outTimeStep, num_in, num_out, num_tasks);
	
	/* Call custom initialization */
	return USER_Initialize();
}

 /*========================================================================*
 * Function: NI_ProbeOneSignal
 *
 * Abstract:
 *	Returns the value to a model's signal during it's execution.
 *
 * Parameters:
 *	idx : the signal's index to probe
 *	value : the buffer into where the signal's value is written
 *	len  : the signal's length in the "value" parameter.
 *	count : the number of elements to probe in a multidimensional signal
 *
 * Returns:
 *	the total number of probed signal elements
========================================================================*/
int32_t NI_ProbeOneSignal(int32_t idx, double *value, int32_t len, int32_t *count)
{
	int32_t subindex = 0;
  	int32_t sublength = 0;
	
	/*verify that index is within bounds*/
  	if (idx > SignalSize) 
	{
		SetErrorMessage("Signal index is out of bounds.", 1);
	    return NI_ERROR;
    }
	
	sublength = rtSignalAttribs[idx].width;
  	while ((subindex < sublength) && (*count < len))
	{
		/* Convert the signal's internal datatype to double and return its value */
    	value[(*count)++] = USER_GetValueByDataType((uintptr_t *)rtSignalAttribs[idx].addr, subindex++, rtSignalAttribs[idx].datatype);
	}
	
  	return *count;
}

 /*========================================================================*
 * Function: NIRT_ProbeSignals
 *
 * Abstract:
 *	returns the latest signal values. 
 *
 * Input Parameters: 
 *	sigindices	: list of signal indices to be probed.
 *	numsigs		: length of sigindices array.
 * 	
 *		NOTE: (Implementation detail) the sigindices array that is passed in is delimited by a value of -1. 
 *		Thus the # of valid indices in the sigindices table is min(numsigs, index of value -1) - 1. 
 *		Reason for subtracting 1, the first index in the array is used for bookkeeping and should be copied
 *		into the 0th index in the signal values buffer. 
 *		Also, the 1st index in the signal values buffer should contain the current timestamp. Hence valid 
 *		signal data should be filled in starting from index 2. Any non-scalar signals should be added to the 
 *		buffer in row-order. 
 * 	  
 * Input/Output Parameters
 *	num			: (in) length of sigvalues buffer (out) number of values returned in the buffer
 * 	  
 * Output Parameters: 
 *	value		: array of signal values
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_ProbeSignals(int32_t *sigindices, int32_t numsigs, double *value, int32_t* len)
{
	int32_t i = 0;
	int32_t count = 0;
	int32_t idx = 0;
	
	if (!NIRT_system.inCriticalSection)
	{
    	SetErrorMessage("SignalProbe should only be called between ScheduleTasks and PostOutputs", 1);
	}
	
	/* Get the index to the first signal */
  	if ((*len > 1)  && (numsigs > 0)) 
	{
	    value[count++] = sigindices[0];
	    value[count++] = 0;
  	}

	/* Get the second and other signals */
  	for (i = 1; (i < numsigs) && (count < *len); i++)
	{
	    idx = sigindices[i];
		
	    if (idx < 0)
		{
    	  	break;
		}
		
    	if (idx < SignalSize)
		{
      		NI_ProbeOneSignal(idx, value, *len, &count);
		}
  	}

  	*len = count;
	return count;	
}

 /*========================================================================*
 * Function: NIRT_GetSignalSpec
 *
 * Abstract:
 *	If index == -1, lookup parameter by the ID.
 *	If ID_len == 0 or if ID == null, return number of parameters in model.
 *	Otherwise, lookup parameter by index, and return the information. 
 *
 * Input/Output Parameters:
 *	index		: index of the signal (in/out) 
 *	ID			: ID of signal to be looked up (in), ID of signal at index (out)
 *	ID_len		: length of input ID(in), length of ID (out)
 *	bnlen		: length of buffer blkname (in), length of output blkname (out)
 *	snlen		: length of buffer signame (in), length of output signame (out)
 *
 * Output Parameters:  
 *	blkname		: Name of the source block for the signal
 *	portnum		: port number of the block that is the source of the signal (0 indexed)
 *	signame		: Name of the signal
 *	datatype	: same as with parameters. Currently assuming all data to be double. 
 *	dims		: The port's dimension of length 'numdims'. 
 *	numdims		: the port's number of dimensions. If a value of "-1" is specified, the minimum buffer size of "dims" is returned as its value. 
 *		
 * Returns:
 *	NI_OK if no error(if sigidx == -1, number of signals)
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetSignalSpec(int32_t* sidx, char* ID, int32_t* ID_len, char* blkname, int32_t* bnlen, int32_t *portnum, char* signame, int32_t* snlen, int32_t *dattype, int32_t* dims, int32_t* numdim)
{
	  int32_t sigidx = *sidx;
	  int32_t i = 0;
	  char *addr = NULL;
	  char *IDblk = 0;
	  int32_t IDport = 0;
	  
	if (sigidx < 0) 
	{
		/* check if ID has been specified. */
		if ((ID != NULL) && (ID_len != NULL) &&  (*ID_len > 0)) 
		{
			/* parse ID into blkname and port */
			addr = strrchr(ID, ':');
			if (addr == NULL)
			{
				return SignalSize;
			}
			
			/* Calculate the char offset into the string after the port */
			i = addr - ID;
	  
			/* malformed ID */
			if (i <= 0)
			{
				return SignalSize;
			}

			ID[i] = 0;
			IDblk = ID;
			IDport = atoi(ID+i+1);

			/* lookup the table for matching ID */
			for (i = 0; i < SignalSize; i++) 
			{
				if (!strcmp(IDblk,rtSignalAttribs[i].blockname) && IDport==(rtSignalAttribs[i]. portno+1))
				{
					break;
				}
			}

			if (i < SignalSize)
			{
				sigidx = *sidx = i;
			}
			else
			{
				return SignalSize;
			}
		} 
		else 
		{		
			/* no index or ID specified. */
			return SignalSize;
		}
	}

	if ( (sigidx >= 0) && (sigidx < SignalSize) ) 
	{
		if (ID != NULL) 
		{
			/* no need for return string to be null terminated! */
			/* 10 to accomodate ':', port number and null character */
			char *tempID = (char *)calloc(strlen(rtSignalAttribs[sigidx].blockname) + 10, sizeof(char));
			sprintf(tempID,"%s:%d",rtSignalAttribs[sigidx].blockname,rtSignalAttribs[sigidx]. portno+1);
		
				if ((int32_t)strlen(tempID) < *ID_len)
				{
					*ID_len = strlen(tempID);
				}

				strncpy(ID, tempID, *ID_len);
				free(tempID);
			}

		if (blkname != NULL) 
		{
			/* no need for return string to be null terminated! */
				if ((int32_t)strlen(rtSignalAttribs[sigidx].blockname) < *bnlen)
				{
					*bnlen = strlen(rtSignalAttribs[sigidx].blockname);
				}
			
				strncpy(blkname, rtSignalAttribs[sigidx].blockname, *bnlen);
			}

		if (signame != NULL) 
			{
				/* no need for return string to be null terminated! */
				if ((int32_t)strlen(rtSignalAttribs[sigidx].signalname)<*snlen)
				{
					*snlen = strlen(rtSignalAttribs[sigidx].signalname);
				}
				
				strncpy(signame, rtSignalAttribs[sigidx].signalname, *snlen);
			}

		if (portnum != NULL) 
		{
			*portnum = rtSignalAttribs[sigidx].portno;
		}
		
		if (dattype != NULL) 
		{
			*dattype = rtSignalAttribs[sigidx].datatype;
		}
				
		if (numdim != NULL)
		{
			if (*numdim == -1)
			{
				*numdim = rtSignalAttribs[sigidx].numofdims;
			}
			else if (dims != NULL) 
			{
				for (i = 0; i < *numdim; i++)
				{
					dims[i] = SigDimList[rtSignalAttribs[sigidx].dimListOffset + i];
				}
			}
		}

		return NI_OK;
	}

	return SignalSize;	
}

 /*========================================================================*
 * Function: NIRT_GetParameterIndices
 *
 * Abstract:
 *	Returns an array of indices to tunable parameters
 *
 * Output Parameters:
 *	indices	: buffer to store the parameter indices of length "len"
 *	len		: returns the number of indices. If a value of "-1" is specified, the minimum buffer size of "indices" is returned as its value. 
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetParameterIndices(int32_t* indices, int32_t* len)
{
  	int32_t i;
	
	if((len == NULL) || (indices == NULL))
	{
		return NI_ERROR;
	}
	
	if (*len == -1)
	{
		*len = ParameterSize;
	}
	else
	{
		for (i = 0; i < ParameterSize && i < *len; i++)
		{
			indices[i] = i;
		}
		
		*len = i;
	}

  	return NI_OK;	
}

 /*========================================================================*
 * Function: NIRT_GetParameterSpec
 *
 * Abstract:
 *	If index == -1, lookup parameter by the ID.
 *	If ID_len == 0 or if ID == null, return number of parameters in model.
 *	Otherwise, lookup parameter by index, and return the information. 
 *
 * Input/Output Parameters:
 *	paramidx	: index of the parameter(in/out). If a value of "-1" is specified, the parameter's ID is used instead
 *	ID			: ID of parameter to be looked up (in), ID of parameter at index (out)
 *	ID_len		: length of input ID (in), length of ID (out)
 *	pnlen		: length of buffer paramname(in), length of the returned paramname (out)
 *	numdim		: length of buffer dimension(in), length of output dimension (out). If a value of "-1" is specified, the minimum buffer size of "dims" is returned as its value
 *
 * Output Parameters: 
 *	paramname	: Name of the parameter
 *	datatype	: datatype of the parameter (currently assuming 0: double, .. )
 *	dims		: array of dimensions with length 'numdim'
 *		
 * Returns:
 *	NI_OK if no error (if paramidx == -1, number of tunable parameters)
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetParameterSpec(int32_t* pidx, char* ID, int32_t* ID_len, char* paramname, int32_t *pnlen, int32_t *dattype, int32_t* dims, int32_t* numdim)
{
	int32_t i= 0;
	int32_t paramidx = *pidx;

	if (paramidx < 0) 
	{
		/* check if ID has been specified. */
		if ( (ID != NULL) && (ID_len != NULL) && (*ID_len > 0) ) 
		{
			/* lookup the table for matching ID */
			for (i = 0; i < ParameterSize; i++) 
			{
				if (strcmp(ID, rtParamAttribs[i].paramname) == 0)
				{
					/* found matching string */
					break;
				}
			}

			if (i < ParameterSize)
			{
				/* note the index into the rtParamAttribs */
				paramidx = *pidx = i;
			}
			else
			{
				return ParameterSize;
			}
		} 
		else
		{
			/* no index or ID specified. */
			return ParameterSize;
		}
	}
	
	if ( (paramidx >= 0) && (paramidx < ParameterSize) ) 
	{
		if(ID != NULL) 
		{
			if ((int32_t)strlen(rtParamAttribs[paramidx].paramname) < *ID_len)
			{
				*ID_len = strlen(rtParamAttribs[paramidx].paramname);
			}
			strncpy(ID, rtParamAttribs[paramidx].paramname, *ID_len);
		}

		if(paramname != NULL) 
		{
			/* no need for return string to be null terminated! */
			if ((int32_t)strlen(rtParamAttribs[paramidx].paramname) < *pnlen)
			{
				*pnlen = strlen(rtParamAttribs[paramidx].paramname);
			}
			strncpy(paramname, rtParamAttribs[paramidx].paramname, *pnlen);
		}

		if (dattype != NULL)
		{
			*dattype = rtParamAttribs[paramidx].datatype;
		}
		
		if (numdim != NULL)
		{
			if (*numdim == -1)
			{
				*numdim = rtParamAttribs[paramidx].numofdims;
			}
			else if (dims != NULL)
			{
				for (i = 0; i < *numdim; i++)
				{
					dims[i] = ParamDimList[rtParamAttribs[paramidx].dimListOffset + i];
				}
			}
		}

		return NI_OK;
	}

	return ParameterSize;	
}

 /*========================================================================*
 * Function: NIRT_GetParameter
 *
 * Abstract:
 *	Get the current value of a parameter.
 * 
 * Input Parameters: 
 *	index	: index of the parameter
 *	subindex: element index into the flattened array if an array
 * 
 * Output Parameters:
 *	val		: value of the parameter 
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetParameter(int32_t index, int32_t subindex, double* val)
{
  	char* ptr = NULL;
	
	/* Check index boundaries */
  	if ( (index >= ParameterSize) || (index < 0) || (subindex >= rtParamAttribs[index].width) )
	{
	    return NI_ERROR;
	}
	
	/* Get the parameter's address into the Parameter struct 
	casting to char to perform pointer arithmetic using the byte offset */
  	ptr = (char*)&rtParameter[READSIDE] + rtParamAttribs[index].addr;
	
	/* Convert the parameter's internal datatype to double and return its value */
  	*val = USER_GetValueByDataType(ptr, subindex, rtParamAttribs[index].datatype);
	
  	return NI_OK;	
}

 /*========================================================================*
 * Function: NIRT_GetVectorParameter
 *
 * Abstract:
 *	Get the current value of a vector parameter.
 * 
 * Input Parameters: 
 *	index: index of the parameter
 *	paramLength: length of the parameter
 * 
 * Output Parameters:
 *	paramValues: an array of the parameter's value
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetVectorParameter(uint32_t index, double* paramValues, uint32_t paramLength)
{
  	char* ptr = NULL;
	uint32_t i = 0;
	
	/* Check index boundaries */
  	if ( (index >= ParameterSize) || (index < 0) || (paramLength != rtParamAttribs[index].width) )
	{
	    return NI_ERROR;
	}

	/* Get the parameter's address into the Parameter struct 
	casting to char to perform pointer arithmetic using the byte offset */
  	ptr = (char*)&rtParameter[READSIDE] + rtParamAttribs[index].addr;
	
  	while(i < paramLength)
	{
		/* Convert the parameter's internal datatype to double and return its value */
		paramValues[i] = USER_GetValueByDataType(ptr, i, rtParamAttribs[index].datatype);
		i++;
	}
	
  	return NI_OK;	
}

 /*========================================================================*
 * Function: NIRT_SetParameter
 *
 * Abstract:
 *	Set parameter value. Called either in scheduler loop or a background loop. 
 * 	
 * Input Parameters:
 *	index	: index of the parameter (as specified in Parameter Information)
 *	subindex: index in the flattened array, if parameter is n-D array
 *	val		: Value to set the parameter to
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_SetParameter(int32_t index, int32_t subindex, double val)
{
  	char* ptr = NULL;
	
	/* Check bounds */
  	if (index >= ParameterSize) 
	{
	  	NIRT_system.SetParamTxStatus = NI_ERROR;
		SetErrorMessage("Parameter index is out of bounds.", 1);
	    return NIRT_system.SetParamTxStatus;
    }
	
	/* Commit parameter values */
  	if (index < 0) 
	{
		/* Check if the read-side has since been modified. If it is, return an error and flush all changes to the write-side*/
		if(ReadSideDirtyFlag == 1)
		{
			memcpy(&rtParameter[1-READSIDE], &rtParameter[READSIDE], sizeof(Parameters));
			ReadSideDirtyFlag = 0;
			
			if(WriteSideDirtyFlag == 0)
			{
				/* No values to commit */
				return NI_OK;
			}
			else
			{
				SetErrorMessage("Parameters have been set inline and from the background loop at the same time. Parameters written from the background loop since the last commit have been lost.",1);
				WriteSideDirtyFlag = 0;
				return NI_ERROR;
			}
		}

		/* If an error occurred and we have values to commit, then save to the write side and return error */
	    if (NIRT_system.SetParamTxStatus == NI_ERROR) 
		{
	 		if(WriteSideDirtyFlag == 1)
			{
				memcpy(&rtParameter[READSIDE], &rtParameter[1-READSIDE], sizeof(Parameters));
			}

      		/* reset the status. */
      		NIRT_system.SetParamTxStatus = NI_OK;
			WriteSideDirtyFlag = 0;
			
      		return NI_ERROR;
	    }
		
		/* If we have values to commit, then save to the write-side */
		if(WriteSideDirtyFlag == 1)
		{
			/* commit changes */
			WaitForSingleObject(NIRT_system.flip, INFINITE);
			READSIDE = 1 - READSIDE;
			ReleaseSemaphore(NIRT_system.flip, 1, NULL);

			/* Copy back the newly set parameters to the write-side. */
			memcpy(&rtParameter[1-READSIDE], &rtParameter[READSIDE], sizeof(Parameters));
			WriteSideDirtyFlag = 0;
		}
		
    	return NI_OK;
  	}
	else
	{
		/* Update a parameter value in the pending buffer */

		/* verify that sub-index is within bounds. */
		if (subindex >= rtParamAttribs[index].width) 
		{
			NIRT_system.SetParamTxStatus = NI_ERROR;
			SetErrorMessage("Parameter subindex is out of bounds.",1);
			return NIRT_system.SetParamTxStatus;
		}

		/* If we have pending modified parameters, then copy to write-side */
		if(ReadSideDirtyFlag == 1)
		{
			memcpy(&rtParameter[1-READSIDE], &rtParameter[READSIDE], sizeof(Parameters));
			ReadSideDirtyFlag = 0;
		}
			
		/* Get the parameter's address into the Parameter struct 
		casting to char to perform pointer arithmetic using the byte offset */
		ptr = (char*)&rtParameter[1-READSIDE] + rtParamAttribs[index].addr;
		WriteSideDirtyFlag = 1;
		
		/* Convert the incoming double datatype to the parameter's internal datatype and update value */
		return USER_SetValueByDataType(ptr, subindex, val, rtParamAttribs[index].datatype);
	}
}

 /*========================================================================*
 * Function: NIRT_SetScalarParameterInline
 *
 * Abstract:
 *	Sets the value to a parameter immediately without the need of a commit request.
 *
 * Input Parameters:
 *	index		: index of the parameter as returned by NIRT_GetParameterSpec()
 *	subindex	: offset of the element within the parameter
 *	paramvalue	: Value to set the parameter to
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_SetScalarParameterInline( uint32_t index,  uint32_t subindex,  double paramvalue)
{
  	char* ptr = NULL;
	
	/*verify that index is within bounds*/
  	if (index >= ParameterSize) 
	{
	  	NIRT_system.SetParamTxStatus = NI_ERROR;
		SetErrorMessage("Parameter index is out of bounds.",1);
	    return NIRT_system.SetParamTxStatus;
    }

  	/* verify that parameter length is correct. */
  	if (subindex >= rtParamAttribs[index].width) 
	{
	  	NIRT_system.SetParamTxStatus = NI_ERROR;
		SetErrorMessage("Parameter subindex is out of bounds.",1);
	    return NIRT_system.SetParamTxStatus;
    }
	
	/* Get the parameter's address into the Parameter struct 
	casting to char to perform pointer arithmetic using the byte offset */
  	ptr = (char*)&rtParameter[READSIDE] + rtParamAttribs[index].addr;
	ReadSideDirtyFlag = 1;
	
	/* Convert the incoming double datatype to the parameter's internal datatype and update value */
	return USER_SetValueByDataType(ptr, subindex, paramvalue, rtParamAttribs[index].datatype);
}

 /*========================================================================*
 * Function: NIRT_SetVectorParameter
 *
 * Abstract:
 *	Sets the value to a parameter array.
 * 	
 * Input Parameters:
 *	index		: index of the parameter as returned by NIRT_GetParameterSpec()
 *	paramvalues	: array of values to set
 *	paramlength	: Length of parameter values.
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_SetVectorParameter( uint32_t index, const double* paramvalues,  uint32_t paramlength)
{
  	char* ptr = NULL;
	uint32_t i = 0;
	int32_t retval = NI_OK;
	
	/*verify that index is within bounds*/
  	if (index >= ParameterSize) 
	{
	  	NIRT_system.SetParamTxStatus = NI_ERROR;
		SetErrorMessage("Parameter index is out of bounds.",1);
	    return NIRT_system.SetParamTxStatus;
    }

  	/* verify that parameter length is correct. */
  	if (paramlength != rtParamAttribs[index].width) 
	{
	  	NIRT_system.SetParamTxStatus = NI_ERROR;
		SetErrorMessage("Parameter length is incorrect.",1);
	    return NIRT_system.SetParamTxStatus;
    }
	
	/* If we have pending modified parameters, then copy to write-side */
	if(ReadSideDirtyFlag == 1)
	{
		memcpy(&rtParameter[1-READSIDE], &rtParameter[READSIDE], sizeof(Parameters));
		ReadSideDirtyFlag = 0;
	}
	
	/* Get the parameter's address into the Parameter struct 
	casting to char to perform pointer arithmetic using the byte offset */
  	ptr = (char*)&rtParameter[1-READSIDE] + rtParamAttribs[index].addr;
	
	while(i < paramlength)
	{
		/* Convert the incoming double datatype to the parameter's internal datatype and update value */
		retval = retval & USER_SetValueByDataType(ptr, i, paramvalues[i], rtParamAttribs[index].datatype);
		i++;
	}
	
	WriteSideDirtyFlag = 1;
	return retval;
}

 /*========================================================================*
 * Function: NIRT_Schedule
 *
 * Abstract:
 *	Invokes the scheduler. This method is called every base time step. 
 *	The model receives external inputs for the current time step, computes the schedule, and posts the outputs.
 *	In addition, the model may choose to executes a base-rate task.
 * 
 * Input Parameters: 
 *	inData			: buffer with model external inputs values.
 * 
 * Output Parameters:
 *	outData			: model external outputs from last time step.
 *	outTime			: current simulation time.
 *	dispatchtasks	: list of tasks to be dispatched this time step (used for multirate models only)
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_Schedule(double *inData, double *outData, double *outTime, int32_t *dispatchtasks)
{
	int32_t retval = NI_ERROR;
	
	if (outTime)
	{
		*outTime = NIRT_system.timestamp;
	}
	
	if(NIRT_system.stopExecutionFlag)
	{
		return NI_ERROR;
	}
	
	WaitForSingleObject(NIRT_system.flip, INFINITE);
	if (NIRT_system.inCriticalSection > 0) 
	{
		SetErrorMessage("Each call to Schedule() MUST be followed by a call to ModelUpdate() before Schedule() is called again.", 1);
		ReleaseSemaphore(NIRT_system.flip, 1, NULL);
		retval = NI_ERROR;
	}
	else
	{
		retval = USER_TakeOneStep(inData, outData, NIRT_system.timestamp);
		NIRT_system.inCriticalSection++;
	}
	
	return retval;
}

 /*========================================================================*
 * Function: NIRT_ModelUpdate
 *
 * Abstract:
 *	Updates the internal state of the model. 
 *
 *	Each call to Schedule() or ScheduleTasks() must be followed by a call to NIRT_ModelUpdate before
 *	the scheduling call can be made again.
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_ModelUpdate(void)
{
	if (NIRT_system.inCriticalSection) 
	{
		NIRT_system.inCriticalSection--;
		NIRT_system.timestamp += USER_BaseRate;
		ReleaseSemaphore(NIRT_system.flip, 1, NULL);
	} 
	else 
	{
		SetErrorMessage("Model Update Failed", 1);
	}
	
	return NIRT_system.inCriticalSection;
}

 /*========================================================================*
 * Function: NIRT_GetExtIOSpec
 *
 * Abstract:
 *	Returns the model's inport or outport specification
 *	
 * Input Parameters: 
 *	index	: index of the task
 *
 * Output Parameters: 
 *	idx		: idx of the IO.
 *	name	: Name of the IO block
 *	tid		: task id
 *	type	: EXT_IN or EXT_OUT
 *	dims	: The port's dimension of length 'numdims'. 
 *	numdims : the port's number of dimensions. If a value of "-1" is specified, the minimum buffer size of "dims" is returned as its value. 
 *		
 * Returns
 *	NI_OK if no error. (if index == -1, return number of tasks in the model) 
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetExtIOSpec(int32_t index, int32_t *idx, char* name, int32_t* tid, int32_t *type, int32_t *dims, int32_t* numdims)
{
 	if (index == -1)
	{
		/* Return number of ports */
    	return ExtIOSize;
	}
	
  	if (idx != NULL)
	{
		/* Note the port index */
    	*idx = index;
	}
	
  	if (name != NULL)
	{
		/* Get the port name */
    	int32_t sz = strlen(name);
    	strncpy(name, rtIOAttribs[index].name, sz-1);
    	name[sz-1] = 0;
  	}

  	if (tid != NULL)
	{
		/* Get the task Id */
    	*tid = rtIOAttribs[index].TID;
	}
	
  	if (type != NULL)
	{
		/* Report if operating on an Inport or Outport */
    	*type = rtIOAttribs[index].type;
	}
	
	if (numdims != NULL)
	{
		if (*numdims == -1)
		{
			*numdims = 2;
		}
		else if(numdims != NULL) 
		{
			/* Return port dimensions */
			dims[0] = rtIOAttribs[index].dimX;
			dims[1] = rtIOAttribs[index].dimY;
		}
	}

  	return NI_OK;
}

 /*========================================================================*
 * Function: NIRT_FinalizeModel
 *
 * Abstract:
 *	Releases resources. Called after simulation stops.
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_FinalizeModel(void) 
{
	CloseHandle(NIRT_system.flip);
	return USER_Finalize();
}

 /*========================================================================*
 * Function: NIRT_GetModelSpec
 *
 * Abstract:
 *	Get the model information without initializing.
 *
 * Output Parameters: 
 *	name			: name of the model
 *	namelen			: the name string length. If a value of "-1" is specified, the minimum buffer size of "name" is returned
 *	baseTimeStep	: base time step of the model
 *	outNumInports	: number of external inputs
 *	outNumOutports	: number of external outputs
 *	numtasks		: number of Tasks (including the base rate task)
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetModelSpec(char* name, int32_t *namelen, double *baseTimeStep, int32_t *outNumInports, int32_t *outNumOutports, int32_t *numtasks)
{
	if (namelen != NULL) 
	{
		/* Return model name */
		int32_t i = 0;
		int32_t length = 0;
		
		length = (int32_t)strlen(USER_ModelName);
		
		if (*namelen == -1)
		{
			/* Return the required minimum buffer size */
			*namelen = length;
		}
		else if (name != NULL)
		{
			if (*namelen > length)
			{
				*namelen = length;
			}
			
			/* Return the model's name */
			strncpy(name, USER_ModelName, *namelen);

			for(i = 0; i < *namelen; i++)
			{
				/* Transform each character to lowercase*/
				name[i] = (char) tolower(name[i]);
			}
		}
	}
	
	if (baseTimeStep != NULL)
	{
		/* Return base time */
		*baseTimeStep = USER_BaseRate;
	}
	
	if (outNumInports != NULL)
	{
		/* Return number of inports */
		*outNumInports = InportSize;
	}
	
	if (outNumOutports != NULL)
	{
		/* Return number of outports */
		*outNumOutports = OutportSize;
	}
	
	if (numtasks != NULL) 
	{
		/* Return number of tasks */
		*numtasks = 1; 
	}
	
	return NI_OK;	
}

 /*========================================================================*
 * Function: NIRT_GetBuildInfo
 *
 * Abstract:
 *	Get the DLL versioning etc information.
 * 
 * Output Parameters:
 *	detail	: String containing model build information.
 *	len		: the build info string length. If a value of "-1" is specified, the minimum buffer size of "detail" is returned as its value. 
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetBuildInfo(char* detail, int32_t* len)
{
	int32_t msgLength = 0;
	char *msgBuffer = NULL;
	
	/* Message */
	const char msg[] = "%s\nModel Name: %s";
	
	/* Print to screen */
	printf("\n*******************************************************************************\n");
	msgLength = printf(msg, USER_Builder, USER_ModelName);
	printf("\n*******************************************************************************\n");

	if (*len == -1)
	{
		/* Return the required minimum buffer size */
		*len = msgLength;
	}
	else
	{
		/* allocate the buffer */
		msgBuffer = (char*)calloc(msgLength + 1, sizeof(char));
		sprintf (msgBuffer, msg, USER_Builder, USER_ModelName);
		
		if (*len >= msgLength) 
		{
			*len = msgLength + 1;
		}
		
		/* Copy into external buffer */
		strncpy(detail, msgBuffer, *len);
		
		/* Release memory */
		free(msgBuffer);
	}
	
	return NI_OK;
}

 /*========================================================================*
 * Function: NIRT_GetErrorMessageLength
 *
 * Abstract:
 *	Gets the length of the error message.
 *
 * Returns:
 *	Length of the error message.
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetErrorMessageLength(void)
{
	int32_t retval = 0;

	if (NIRT_system.errmsg != NULL)
	{
		retval = strlen(NIRT_system.errmsg); 
	}
	else
	{
		/* No error message length */
		retval =  0;
	}

	return retval;
}
