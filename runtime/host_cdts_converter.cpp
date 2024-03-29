#include "host_cdts_converter.h"
#include "cdts_python3.h"
#include "utils.h"
#include <runtime/cdt_capi_loader.h>

//--------------------------------------------------------------------
[[maybe_unused]] cdts* convert_host_params_to_cdts(PyObject* params, metaffi_type_info* param_metaffi_types, int params_count, metaffi_size return_values_size)
{
	try
	{

		pyscope();

		Py_ssize_t params_size = Py_IsNone(params) ? 0 : PyTuple_Size(params);
		if(params_size == 0)
		{
			if(return_values_size == 0){
				return nullptr;
			}
			else{
				return xllr_alloc_cdts_buffer(0, return_values_size);
			}
		}

		// get the data from the local objects
		cdts* cdts_buf = xllr_alloc_cdts_buffer(PyTuple_Size(params), return_values_size);
		cdts_python3 pycdts(cdts_buf[0].pcdt, cdts_buf[0].len );

		pycdts.to_cdts(params, param_metaffi_types, params_count);

		return cdts_buf;
	}
	catch(std::exception& e)
	{
		std::stringstream ss;
		ss << "Failed convert_host_params_to_cdts: " << e.what();

		PyErr_SetString(PyExc_ValueError, ss.str().c_str());
		return nullptr;
	}
}
//--------------------------------------------------------------------
[[maybe_unused]] PyObject* convert_host_return_values_from_cdts(cdts* pcdts, metaffi_size index)
{
	try
	{
		pyscope();
		cdts_python3 cdts(pcdts[index].pcdt, pcdts[index].len);
		PyObject* o = cdts.to_py_tuple().detach();

		return o;
	}
	catch(std::exception& e)
	{
		std::stringstream ss;
		ss << "Failed convert_host_params_to_cdts: " << e.what();
		PyErr_SetString(PyExc_ValueError, ss.str().c_str());
		return Py_None;
	}
}
//--------------------------------------------------------------------