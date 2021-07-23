#include "host_cdts_converter.h"
#include "cdts_python3.h"
#include "utils.h"

//--------------------------------------------------------------------
cdt* convert_host_params_to_cdts(PyObject* params, PyObject* params_types)
{
	pyscope();
	
	if(!PyTuple_Check(params))
	{
		PyErr_SetString(PyExc_ValueError, "params_names is not a tuple as expected");
		return nullptr;
	}
	
	if(!PyTuple_Check(params_types))
	{
		PyErr_SetString(PyExc_ValueError, "params_types is not a tuple as expected");
		return nullptr;
	}
	
	// get the data from the local objects
	cdts_python3 cdts(PyTuple_Size(params));
	
	cdts.build(params, params_types, 0);
	
	return cdts.get_cdts();
}
//--------------------------------------------------------------------
PyObject* convert_host_return_values_from_cdts(cdt* cdt_return_values, openffi_size return_values_count)
{
	pyscope();
	
	cdts_python3 cdts(cdt_return_values, return_values_count);
	PyObject* o = cdts.parse();
	return o;
}
//--------------------------------------------------------------------