/* Include headers */
#include "ni_modelframework.h"
#include "model.h"
#include <stddef.h>
#include <math.h>

/* User defined datatypes and constants */
#define rtDBL	0
#define rtINT	2

/* Pull in the parameters structure from codegen.c */
extern Parameters rtParameter[2];
extern int32_t READSIDE;

/* !!!! IMPORTANT !!!!
   Accessing parameters values must be done through rtParameter[READSIDE]
   The macro readParam is defined for you as a simple way to access parameters
   !!!! IMPORTANT !!!! */
#define readParam rtParameter[READSIDE]

/* Define IO and Signals structs */
typedef struct {
@Inports-Decl@
} Inports;

typedef struct {
@Outports-Decl@
} Outports;

typedef struct {
@Signals-Decl@
} Signals;

/* Instantiate IO and Signals */
Inports rtInport;
Outports rtOutport;
Signals rtSignal;


/* INPUT: ptr, base address of where value should be set.
   INPUT: subindex, offset into ptr where value should be set.
   INPUT: value, the value to be set
   INPUT: type, the user defined type of the parameter being set, as defined in
  		  rtParamAttributes
   RETURN: status, NI_ERROR on error, NI_OK otherwise */
int32_t USER_SetValueByDataType(void* ptr, int32_t subindex, double value, int32_t type)
{
	switch (type) {
		case rtDBL: {
    		((double *)ptr)[subindex] = (double)value;
    		return NI_OK;
    } 
    case rtINT:{
    	((int32_t *)ptr)[subindex] = (int32_t)value;
    	return NI_OK;
    } 
    default: {
      return NI_ERROR;
    }
	}
}

/* INPUT: ptr, base address of value to be retrieved.
   INPUT: subindex, offset into ptr where value should be retrieved.
   INPUT: type, the user defined type of the parameter or signal being 
  		  retrieved, as defined in rtParamAttributes or rtSignalAttributes
   RETURN: value of user-defined type cast to a double */
double USER_GetValueByDataType(void* ptr, int32_t subindex, int32_t type)
{
	switch (type) {
    case rtDBL:{
      return (double)(((double *)ptr)[subindex]);
    }
    case rtINT: {
      return (double)(((int32_t *)ptr)[subindex]);
    }
    default: {
        /* return NaN, ok for vxworks and pharlap */
      uint32_t nan[2] = {0xFFFFFFFF, 0xFFFFFFFF};
      return *(double*)nan;
    }
  }
}

/*
// When a model has parameters of the form: "modelname/parameter", these model parameters are considered global parameters (target scoped) in NI VeriStand
// When a model has parameters of the form: "modelname/block/paramter" these model parameters are NOT considered global parameters (model scoped) in NI VeriStand
typedef struct {
  int32_t idx;			// not used
  char* paramname;	// name of the parameter, e.g., "Amplitude"
  uintptr_t addr;// offset of the parameter in the Parameters struct
  int32_t datatype;		// integer describing a user defined datatype. must have a corresponding entry in GetValueByDataType and SetValueByDataType
  int32_t width;		// size of parameter
  int32_t numofdims; 	// number of dimensions
  int32_t dimListOffset;// offset into dimensions array
  int32_t IsComplex;	// not used
} NI_Parameter;

*/
/* Define parameter attributes */
int32_t ParameterSize DataSection(".NIVS.paramlistsize") = @ParameterSize@;
NI_Parameter rtParamAttribs[] DataSection(".NIVS.paramlist") = {
@rtParamAttribs@
};
int32_t ParamDimList[] DataSection(".NIVS.paramdimlist") =
{
@ParamDimList@
};

/* Initialize parameters */
Parameters initParams DataSection(".NIVS.defaultparams") = {
@initParams@
};


/*
   This data structure is used to retrieve the size, width, and datatype of the default parameters.
      
   ParamSizeWidth Parameters_sizes[] DataSection(".NIVS.defaultparamsizes") = {
    { sizeof(initParams), 0, 0},  The first element in this array uses only the first field in the typedef.  It is used to specify the size of the default parameters structure.
    { sizeof(double), 1, 0 }, Subsequent elements in the array use all 3 fields, they are: the size (num of bytes per element), the width (num of elements) (2x2 array would have 4 elements), and the datatype of each parameter (which is handled by Get/SetValueByType)
   };  
*/
ParamSizeWidth Parameters_sizes[] DataSection(".NIVS.defaultparamsizes") = {
  { sizeof(initParams)},
  @Parameters_sizes@
};

/*
typedef struct {
  int32_t    idx;		// not used
  char*  blockname; // name of the block where the signals originates, e.g., "sinewave/sine"
  int32_t    portno;	// the port number of the block
  char* signalname; // name of the signal, e.g., "Sinewave + In1"
  uintptr_t addr;// address of the storage for the signal
  uintptr_t baseaddr;		// not used
  int32_t	 datatype;	// integer describing a user defined datatype. must have a corresponding entry in GetValueByDataType
  int32_t width;		// size of signal
  int32_t numofdims; 	// number of dimensions
  int32_t dimListOffset;// offset into dimensions array
  int32_t IsComplex;	// not used
} NI_Signal;
*/

/* Define signal attributes */
int32_t SignalSize DataSection(".NIVS.siglistsize") = @SignalSize@;
/* must be careful to not get a pointer into .rela.NIVS.siglist */
/* the addr field for these signals is populated in USER_Initialize */
NI_Signal rtSignalAttribs[] DataSection(".NIVS.siglist") = {
@rtSignalAttribs@
};
int32_t SigDimList[] DataSection(".NIVS.sigdimlist") =
{
@SigDimList@
};

/*
typedef struct {
  int32_t	idx;	// not used
  char*	name;	// name of the external IO, e.g., "In1"
  int32_t	TID;	// = 0
  int32_t   type; 	// Ext Input: 0, Ext Output: 1
  int32_t  width; 	// not used
  int32_t	dimX;	// 1st dimension size
  int32_t	dimY; 	// 2nd dimension size
} NI_ExternalIO;
*/

/* Define I/O attributes */
int32_t ExtIOSize DataSection(".NIVS.extlistsize") = @ExtIOSize@;
int32_t InportSize = @InportSize@;
int32_t OutportSize = @OutportSize@;
NI_ExternalIO rtIOAttribs[] DataSection(".NIVS.extlist") = {
@rtINAttribs@
@rtOutAttribs@
	{ -1 },
};

/* Model name and build information */
const char * USER_ModelName DataSection(".NIVS.compiledmodelname") = "@model-name@";
const char * USER_Builder DataSection(".NIVS.builder") = "@model-desc@";

/* baserate is the rate at which the model runs */
double USER_BaseRate = @baserate@;

/*
typedef struct {
  int32_t    tid;		// = 0
  double tstep;		
  double offset;
  int32_t priority;
} NI_Task;
*/
NI_Task rtTaskAttribs DataSection(".NIVS.tasklist") = { 0 /* must be 0 */, @baserate@ /* must be equal to baserate */, 0, 0 };

/* RETURN: status, NI_ERROR on error, NI_OK otherwise */
int32_t USER_Initialize() {
	/*Initialize signal addresses*/
@USER_Initialize@
	return NI_OK;
}

@implementation@

/* RETURN: status, NI_ERROR on error, NI_OK otherwise */
int32_t USER_Finalize() {
	return NI_OK;
}

/* Non-supported API */

extern struct { 
	int32_t stopExecutionFlag;
	const char *errmsg;
	void* flip;
	uint32_t inCriticalSection;
	int32_t SetParamTxStatus;
	double timestamp;
} NIRT_system;

DLL_EXPORT int32_t NIRT_GetSimState(int32_t* numContStates, char* contStatesNames, double* contStates, int32_t* numDiscStates, char* discStatesNames, double* discStates, int32_t* numClockTicks, char* clockTicksNames, int32_t* clockTicks) 
{
	if (numContStates && numDiscStates && numClockTicks) {
		if (*numContStates < 0 || *numDiscStates < 0 || *numClockTicks < 0) {
			*numContStates = 0;
			*numDiscStates = 0;
			*numClockTicks = 1;
			return NI_OK;
		}
	}
	
	if (clockTicks && clockTicksNames) {
		clockTicks[0] = (int)NIRT_system.timestamp;
		strcpy(clockTicksNames, "clockTick0");
	}	
	return NI_OK;
}

DLL_EXPORT int32_t NIRT_SetSimState(double* contStates, double* discStates, int32_t* clockTicks)
{
	if (clockTicks) {
		NIRT_system.timestamp = clockTicks[0];
	}	
	return NI_OK;
}

int32_t USER_ModelStart() {
	return NI_OK;
}