/*========================================================================*
 * NI VeriStand Model Framework 
 * Core interface
 *
 * Abstract:
 *	Function prototypes and type definitions to the NI VeriStand Model Framework
 *
 *========================================================================*/
 
#ifndef NI_MODELFRAMEWORK_H
#define NI_MODELFRAMEWORK_H

/* NI VeriStand Model Framework API version */
#define NIMF_VER_MAJOR 2016
#define NIMF_VER_MINOR 0
#define NIMF_VER_FIX 0
#define NIMF_VER_BUILD 72

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <ctype.h>

#if defined (VXWORKS) || defined (kNIOSLinux)
	#ifdef __cplusplus
		#define DLL_EXPORT extern "C"
	#else
		#define DLL_EXPORT 
	#endif
	#define DataSection(t) __attribute__ ((section((t))))
#else
	/* Windows macros */
    #ifdef __cplusplus
		#define DLL_EXPORT extern "C" __declspec(dllexport)
	#else
		#define DLL_EXPORT __declspec(dllexport)
	#endif
	#define DataSection(t)
#endif

/* Define C99 fixed-sized types */
#if defined (VXWORKS) || defined (kNIOSLinux) || _MSC_VER > 1500
	/* VxWorks, Linux or MSVC10+ */
	#include <stdint.h>
#else
	/* MSVC9 and earlier */
	typedef __int32 int32_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;
	typedef __int64 int64_t;
	
	#ifndef _UINTPTR_T_DEFINED
		/* Windows SDK defines uintptr_t in vadefs.h*/
		typedef unsigned int uintptr_t;
	#endif
#endif

#ifdef VXWORKS 
	/* VxWorks includes */
	# include <vxWorks.h>
	# include <sysLib.h>
	# include <math.h>
	# include <semLib.h>
	# include <time.h>
	# include <private/mathP.h>

	/* VxWorks macros */
	# define HANDLE SEM_ID
	# define INFINITE WAIT_FOREVER
#elif kNIOSLinux
	/* Linux includes */
	# include <semaphore.h>
	# include <time.h>
	# include <math.h>

	/* Linux macros */
	# define HANDLE sem_t *
	# define INFINITE -1
#else
	/* Windows includes */
	# include <windows.h>
#endif

/* UNUSED_PARAMETER(x)
 * Used to specify that a function parameter is required but not
 * accessed by the function body */
#ifndef UNUSED_PARAMETER
	#define UNUSED_PARAMETER(x) (void)(x)
#endif 

#define NI_OK		0
#define NI_ERROR	1

typedef struct {
  int32_t idx;			/* not used */
  const char* name;		/* name of the external IO, e.g., "In1" */
  int32_t TID;			/* = 0 */
  int32_t type;			/* Ext Input: 0, Ext Output: 1 */
  int32_t width;		/* not used */
  int32_t dimX;			/* 1st dimension size */
  int32_t dimY;			/* 2nd dimension size */
} NI_ExternalIO;

typedef struct {
  int32_t idx;			/* not used */
  const char* paramname;/* name of the parameter, e.g., "Amplitude" */
  uintptr_t addr;		/* address or offset of the parameter in the Parameters struct */
  int32_t datatype;		/* integer describing a user defined datatype. Must have a corresponding entry in GetValueByDataType and SetValueByDataType */
  int32_t width;		/* size of parameter */
  int32_t numofdims;	/* number of dimensions */
  int32_t dimListOffset;/* offset into dimensions array */
  int32_t IsComplex;	/* not used */
} NI_Parameter;

typedef struct {
  int32_t idx;			/* not used */
  const char* blockname;/* name of the block where the signals originates, e.g., "sinewave/sine" */
  int32_t portno;		/* the port number of the block */
  const char* signalname;/* name of the signal, e.g., "Sinewave + In1" */
  uintptr_t addr;		/* address or offset of the storage for the signal */
  uintptr_t baseaddr;	/* not used */
  int32_t datatype;		/* integer describing a user defined datatype. Must have a corresponding entry in GetValueByDataType */
  int32_t width;		/* size of signal */
  int32_t numofdims;	/* number of dimensions */
  int32_t dimListOffset;/* offset into dimensions array */
  int32_t IsComplex;	/* not used */
} NI_Signal;

typedef struct {
  int32_t tid;		
  double tstep;		
  double offset;
  int32_t priority;
} NI_Task;

typedef struct {
  int32_t size;
  int32_t width;
  int32_t basetype;
} ParamSizeWidth;

typedef struct {
	uint32_t major;
	uint32_t minor;
	uint32_t fix;
	uint32_t build;
} NI_Version;

extern NI_Version NIVS_APIversion;

/* Definition of user defined function for getting values of user defined types */
double USER_GetValueByDataType(void* ptr, int32_t subindex, int32_t type);

/* Definition of user defined function for setting values of user defined types */
int32_t USER_SetValueByDataType(void* ptr, int32_t subindex, double value, int32_t type);

/* Definition of user defined function for initializing the model. */
int32_t USER_Initialize(void);

/* Definition of user defined function for performing additional initialization after setting parameter values but before model execution starts */
int32_t USER_ModelStart(void);

/* Definition of user defined function to be executed on every iteration of the baserate */
int32_t USER_TakeOneStep(double *inData, double *outData, double timestamp);

/* Definition of user defined function for doing work after model execution has stopped */
int32_t USER_Finalize(void);

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
DLL_EXPORT int32_t NIRT_GetModelFrameworkVersion(uint32_t* major, uint32_t* minor, uint32_t* fix, uint32_t* build);

 /*========================================================================*
 * Function: NIRT_ModelStart
 *
 * Abstract:
 *	Executes before the first time step and only once within a model's execution.
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_ModelStart(void);

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
DLL_EXPORT int32_t NIRT_InitializeModel(double finaltime, double *outTimeStep, int32_t *num_in, int32_t *num_out, int32_t* num_tasks);					

 /*========================================================================*
 * Function: NIRT_PostOutputs
 *
 * Abstract:
 *		Pushes outport data out to LabVIEW for Multi-rate models.
 *
 * Output Parameters:
 *		outData: preallocated array of model outputs
 *
 * Returns:
 *     Return value: 0 if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_PostOutputs(double *outData);									

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
DLL_EXPORT int32_t NIRT_ModelUpdate(void);

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
DLL_EXPORT int32_t NIRT_Schedule(double *inData, double *outData, double *outTime, int32_t *dispatchtasks);

 /*========================================================================*
 * Function: NIRT_TaskTakeOneStep
 *
 * Abstract:
 *	Advance a task one step. Called at the rate of the taskid. (for multirate models only)
 * 
 * Input Parameters: 
 *	taskid	: Task ID of the task to be executed.
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_TaskTakeOneStep(int32_t taskid);

 /*========================================================================*
 * Function: NIRT_FinalizeModel
 *
 * Abstract:
 *	Releases resources. Called after simulation stops.
 *
 * Returns:
 *	NI_OK if no error
 *========================================================================*/
DLL_EXPORT int32_t NIRT_FinalizeModel(void);

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
DLL_EXPORT int32_t NIRT_ProbeSignals(int32_t *sigindices, int32_t numsigs, double *value, int32_t* num);

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
DLL_EXPORT int32_t NIRT_SetScalarParameterInline(uint32_t index,  uint32_t subindex, double paramvalue);

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
DLL_EXPORT int32_t NIRT_SetVectorParameter(uint32_t index, const double* paramvalues, uint32_t paramlength);

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
DLL_EXPORT int32_t NIRT_SetParameter(int32_t index, int32_t subindex, double val);

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
DLL_EXPORT int32_t NIRT_GetParameter(int32_t index, int32_t subindex, double* val);

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
DLL_EXPORT int32_t NIRT_GetVectorParameter(uint32_t index, double* paramValues, uint32_t paramLength);

 /*========================================================================*
 * Function: NIRT_GetErrorMessageLength
 *
 * Abstract:
 *	Gets the length of the error message.
 *
 * Returns:
 *	Length of the error message.
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetErrorMessageLength(void);

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
DLL_EXPORT int32_t NIRT_ModelError(char* errmsg, int32_t* msglen);

 /*========================================================================*
 * Function: NIRT_TaskRunTimeInfo
 *
 * Abstract:
 *		called in background loop. Returns number of overruns of tasks in the simulation overloading this function 
 *		to set the HALT ON TASK OVERRUN flag halt = 1: do not halt, 2: halt.
 *
 *========================================================================*/
DLL_EXPORT int32_t NIRT_TaskRunTimeInfo(int32_t halt, int32_t* overruns, int32_t *numtasks);

 /*========================================================================*
 * Function: NIRT_GetSimState
 *
 * Abstract:
 *
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetSimState(int32_t* numContStates, char* contStatesNames, double* contStates, int32_t* numDiscStates, 
								char* discStatesNames, double* discStates, int32_t* numClockTicks, char* clockTicksNames, int32_t* clockTicks);

 /*========================================================================*
 * Function: NIRT_SetSimState
 *
 * Abstract:
 *
 *========================================================================*/
DLL_EXPORT int32_t NIRT_SetSimState(double* contStates, double* discStates, int32_t* clockTicks);

/*
 * Functions for getting Model Information:
 * These functions are not necessary for the model to run and used to provide a better user experience.
 * 
 * The following functions may be called either in initialization phase, 
 * or from a background process while the simulation is running.
 * 
 * To refer to a parameter or signal in the model, 3 unique entities are used:
 * 1. Name (full_pathname for parameters or Path_to_SourceBlock:PortNumber for signals)
 * 2. ID
 * 3. A 32-bit integer index
 * 
 * Name and ID are both character arrays and may be identical. The ID is not seen by the user and 
 * may include some coded information to make access faster.
 * 
 * An index is an integer that is used as a key into tables for constant time access in RT.
 * Index may have any value > 0. With -1 being the special invalid index.
 * 
 * On RT, the parameter/signal information is extracted from the DLL using their corresponding IDs.
 * The function should return in addition to datatype/dimension etc information, the index for the ID.
 * This index is used subsequently when probing a signal value or setting value of a parameter.
 * 
 * The list of IDs may be either read in from a file generated when building the DLL or another 
 * function may be exported to generate the list.
 */

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
DLL_EXPORT int32_t NIRT_GetBuildInfo(char* detail, int32_t* len);

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
DLL_EXPORT int32_t NIRT_GetModelSpec(char* name, int32_t *namelen, double *baseTimeStep, int32_t *outNumInports, 
										int32_t *outNumOutports, int32_t *numtasks);

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
DLL_EXPORT int32_t NIRT_GetParameterIndices(int32_t* indices, int32_t* len);

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
DLL_EXPORT int32_t NIRT_GetParameterSpec(int32_t* paramidx, char* ID, int32_t* ID_len, char* paramname, int32_t *pnlen, 
									  int32_t *datatype, int32_t* dims, int32_t* numdim);

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
DLL_EXPORT int32_t NIRT_GetSignalSpec(int32_t* sigidx, char* ID, int32_t* ID_len, char* blkname, int32_t* bnlen, int32_t *portnum, 
								   char* signame, int32_t* snlen, int32_t *datatype, int32_t* dims, int32_t* numdim);

 /*========================================================================*
 * Function: NIRT_GetTaskSpec
 *
 * Abstract:
 *	Returns a model's multirate task specifications
 *
 * Input Parameters: 
 *	index	: index of the task
 *		
 * Output Parameters: 
 *	tid		: task ID. This may not necessarily be the index of task in task list.
 *	tstep	: time step for this task
 *	offset	: 
 *		
 * Returns:
 *	NI_OK if no error. (if index == -1, return number of tasks in the model) 
 *========================================================================*/
DLL_EXPORT int32_t NIRT_GetTaskSpec(int32_t index, int32_t* tid, double *tstep, double *offset);


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
DLL_EXPORT int32_t NIRT_GetExtIOSpec(int32_t index, int32_t *idx, char* name, int32_t* tid, int32_t *type, int32_t *dims, int32_t* numdims);
#endif
