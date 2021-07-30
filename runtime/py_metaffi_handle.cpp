#include "py_metaffi_handle.h"
#include "utils.h"

PyObject* new_py_metaffi_handle(metaffi_handle h)
{
	PyObject* sys_mod_dict = PyImport_GetModuleDict();
	PyObject* main_mod = PyMapping_GetItemString(sys_mod_dict, "__main__");
	
	PyObject* instance = PyObject_CallMethod(main_mod, "metaffi_handle", "K", h);
	if(instance == NULL)
	{
		throw std::runtime_error("Failed to create pythonic metaffi_handle object");
	}
	
	return instance;
}