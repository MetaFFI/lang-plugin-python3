#include <runtime/runtime_plugin_api.h>
#include <cstdlib>
#include <cstring>
#include <utils/scope_guard.hpp>
#include <utils/function_path_parser.h>
#include <utils/foreign_function.h>
#include "utils.h"
#include <boost/filesystem.hpp>
#include <Python.h>
#include <sstream>
#include <map>

using namespace openffi::utils;

#define handle_err(err, err_len, desc) \
	*err_len = strlen( desc ); \
	*err = (char*)malloc(*err_len + 1); \
	strcpy(*err, desc ); \
	memset((*err+*err_len), 0, 1);

#define handle_err_str(err, err_len, descstr) \
	*err_len = descstr.length(); \
	*err = (char*)malloc(*err_len + 1); \
	descstr.copy(*err, *err_len, 0); \
	memset((*err+*err_len), 0, 1);

#define handle_py_err(err, err_len) \
	std::string pyerr(get_py_error()); \
	handle_err_str(err, err_len, pyerr);

#define TRUE 1
#define FALSE 0

#define pyscope()\
	PyGILState_STATE gstate = PyGILState_Ensure(); \
	scope_guard sggstate([&]() \
	{ \
		PyGILState_Release(gstate); \
	});
	
std::map<int64_t, PyObject*> loaded_functions;

//--------------------------------------------------------------------
void load_package_path()
{
	std::string curpath(boost::filesystem::current_path().string());
	
	std::stringstream ss;
	ss << "import sys" << std::endl;
	//ss << "import site" << std::endl;
	//ss << "sys.path.append(site.USER_SITE)" << std::endl;
	ss << "sys.path.append('" << curpath << "')" << std::endl;
	
	PyRun_SimpleString(ss.str().c_str());
}
//--------------------------------------------------------------------
void load_runtime(char** err, uint32_t* err_len)
{
	// load python runtime
	if(!Py_IsInitialized())
	{
		Py_InitializeEx(1); // 1 means register signal handlers
	}
	
	pyscope();
	
	load_package_path();
}
//--------------------------------------------------------------------
void free_runtime(char** err, uint32_t* err_len)
{
	if(!Py_IsInitialized())
	{
		return;
	}
		
	int res = Py_FinalizeEx();
	if(res == -1)
	{
		handle_err(err, err_len, "Python finalization has failed!");
		return;
	}
}
//--------------------------------------------------------------------
int64_t load_function(const char* function_path, uint32_t function_path_len, char** err, uint32_t* err_len)
{
	
	if(!Py_IsInitialized())
	{
		handle_err(err, err_len, "Runtime has not been loaded - module cannot be loaded!");
		return -1;
	}
	
	pyscope();
	
	openffi::utils::function_path_parser fp(function_path);
	
	// TODO: can pymod be released?!
	PyObject* pymod = PyImport_ImportModuleEx(fp[function_path_entry_openffi_guest_lib].c_str(), Py_None, Py_None, Py_None);

	if(!pymod)
	{
		// error has occurred
		handle_py_err(err, err_len);
		return -1;
	}
	
	// load function
	PyObject* pyfunc = PyObject_GetAttrString(pymod, fp[function_path_entry_entrypoint_function].c_str());
	
	if(!pyfunc)
	{
		handle_err((char**)err, err_len, "Cannot find function in module!");
		return -1;
	}
	
	if(!PyCallable_Check(pyfunc))
	{
		handle_err((char**)err, err_len, "Requested function found in module, but it is not a function!");
		return -1;
	}
	
	// place in loaded functions
	int64_t function_id = -1;
	for(auto& it : loaded_functions) // make function_id the highest number
	{
		if(function_id <= it.first)
		{
			function_id = it.first + 1;
		}
	}
	
	loaded_functions[function_id] = pyfunc;
	
	return function_id;
}
//--------------------------------------------------------------------
void free_function(int64_t function_id, char** err, uint32_t* err_len)
{
	// TODO: if all functions in a module are freed, module should be freed as well
}
//--------------------------------------------------------------------
void call(
	int64_t function_id,
	unsigned char* in_params, uint64_t in_params_len,
	unsigned char** out_params, uint64_t* out_params_len,
	unsigned char** out_ret, uint64_t* out_ret_len,
	uint8_t* is_error
)
{
	auto it = loaded_functions.find(function_id);
	if(it == loaded_functions.end())
	{
		handle_err((char**)out_ret, out_ret_len, "Requested function has not been loaded");
		*is_error = TRUE;
		return;
	}
	PyObject* pyfunc = it->second;
	
	// set parameters
	PyObject* pyParams = PyBytes_FromStringAndSize((const char*)in_params, in_params_len);
	if(!pyParams)
	{
		handle_err((char**)out_ret, out_ret_len, "Failed to create parameters byte array");
		*is_error = TRUE;
		return;
	}
	
	PyObject* paramsArray = PyTuple_New(1);
	if(!paramsArray)
	{
		handle_err((char**)out_ret, out_ret_len, "Failed to create new tuple");
		*is_error = TRUE;
		return;
	}
	
	scope_guard sgParams([&]()
	{
		Py_DecRef(paramsArray);
	});
	
	PyTuple_SetItem(paramsArray, 0, pyParams);
	PyObject* res = PyObject_CallObject(pyfunc, paramsArray);
	if(!res)
	{
		handle_err((char**)out_ret, out_ret_len, "OpenFFI Python guest code returned with an exception! The generated code must return error as str. (something is wrong...");
		*is_error = TRUE;
		return;
	}

	PyObject* ret = nullptr;
	PyObject* out = nullptr;
	PyObject* errmsg = nullptr;

	scope_guard sgrets([&]()
	{
		if(res){ Py_DecRef(res); res = nullptr; }
		
		// if(ret && res != ret && ret != Py_None){ Py_DecRef(ret); ret = nullptr; } // crashes examples - getting clean up but by whom?
		
		if(out && out != Py_None){ Py_DecRef(out); out = nullptr; }
		// if(errmsg && errmsg != Py_None){ Py_DecRef(errmsg); errmsg = nullptr; }
	});

	// if the return values is a tuple:
	// first parameter is the result - serialized
	// second parameter is the out parameters - serialized OR empty
	if(!PyTuple_Check(res))
	{
		handle_err((char**)out_ret, out_ret_len, "OpenFFI Python guest code did not return a tuple. Expects a tuple! (something is wrong...");
		*is_error = TRUE;
		return;
	}
	
	if(PyTuple_Size(res) == 2) // (serialized return value, error string)
	{
		ret = PyTuple_GetItem(res, 0);
		errmsg = PyTuple_GetItem(res, 1);
	}
	else if(PyTuple_Size(res) == 3) // (serialized return value, serialized parameters value (out params support), error string)
	{
		ret = PyTuple_GetItem(res, 0);
		errmsg = PyTuple_GetItem(res, 1);
		out = PyTuple_GetItem(res, 2);
	}
	else
	{
		handle_err((char**)out_ret, out_ret_len, "OpenFFI Python guest code expected a tuple of size 2 or 3 (something is wrong...)");
		*is_error = TRUE;
		return;
	}
	
	if(errmsg != nullptr && errmsg != Py_None) // error has occurred
	{
		const char* perrmsg = PyUnicode_AsUTF8(errmsg);
		handle_err((char**)out_ret, out_ret_len, perrmsg);
		*is_error = TRUE;
		return;
	}
	
	// get out parameters
	if(out && out != Py_None)
	{
		*out_params_len = PyBytes_Size(out);
		*out_params = (unsigned char*)malloc(*out_params_len);
		memcpy(*out_params, (unsigned char*)PyBytes_AsString(out), *out_params_len);
	}

	if(!PyBytes_Check(ret))
	{
		handle_err((char**)out_ret, out_ret_len, "OpenFFI Python guest code must did not return bytes type as expected (something is wrong...)");
		*is_error = TRUE;
		return;
	}
	
	// get return values
	// serialized proto results are in "str"
	*out_ret_len = PyBytes_Size(ret);
	if(*out_ret_len == 0) // void param
	{
		*out_ret = nullptr;
		*is_error = FALSE;
		return;
	}
	
	const char* pretarray = PyBytes_AsString(ret);
	
	*out_ret = (unsigned char*)malloc(*out_ret_len);
	
	memcpy(*out_ret, (unsigned char*)pretarray, *out_ret_len);
	
	*is_error = FALSE;
}
//--------------------------------------------------------------------
