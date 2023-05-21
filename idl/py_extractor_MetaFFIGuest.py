
# Code generated by MetaFFI. Modify only in marked places.
# Guest code for py_extractor.json

import traceback
import sys
import platform
import os
from typing import Any
from ctypes import *


import py_extractor

python_plugin_handle = None
def load_python_plugin():
	global python_plugin_handle
	
	if python_plugin_handle == None:
		python_plugin_handle = cdll.LoadLibrary(get_filename_to_load('xllr.python3'))
		python_plugin_handle.set_entrypoint.argstype = [c_char_p, c_void_p]
		python_plugin_handle.xcall_params_ret.argstype = [py_object, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong)]
		python_plugin_handle.xcall_params_no_ret.argstype = [py_object, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong)]
		python_plugin_handle.xcall_no_params_ret.argstype = [py_object, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong)]
		python_plugin_handle.xcall_no_params_no_ret.argstype = [py_object, POINTER(c_char_p), POINTER(c_ulonglong)]

def get_filename_to_load(fname):
	osname = platform.system()
	if osname == 'Windows':
		return os.getenv('METAFFI_HOME')+'\\'+ fname + '.dll'
	elif osname == 'Darwin':
		return os.getenv('METAFFI_HOME')+'/' + fname + '.dylib'
	else:
		return os.getenv('METAFFI_HOME')+'/' + fname + '.so' # for everything that is not windows or mac, return .so

load_python_plugin()













@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_variable_info_get_name(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_variable_info_get_name), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_variable_info_get_name'.encode(), CEntryPoint_variable_info_get_name)

def EntryPoint_variable_info_get_name(obj):
	try:

		
		ret_val_types = (4096,)

		return (None, ret_val_types, obj.name)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_variable_info_get_type(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_variable_info_get_type), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_variable_info_get_type'.encode(), CEntryPoint_variable_info_get_type)

def EntryPoint_variable_info_get_type(obj):
	try:

		
		ret_val_types = (4096,)

		return (None, ret_val_types, obj.type)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)








@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_variable_info_Releasevariable_info(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_no_ret(py_object(EntryPoint_variable_info_Releasevariable_info), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_variable_info_Releasevariable_info'.encode(), CEntryPoint_variable_info_Releasevariable_info)

def EntryPoint_variable_info_Releasevariable_info(this_instance):
	try:
		# xcall release object
		
		python_plugin_handle.release_object(this_instance)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)







@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_parameter_info_get_name(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_parameter_info_get_name), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_parameter_info_get_name'.encode(), CEntryPoint_parameter_info_get_name)

def EntryPoint_parameter_info_get_name(obj):
	try:

		
		ret_val_types = (4096,)

		return (None, ret_val_types, obj.name)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_parameter_info_get_type(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_parameter_info_get_type), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_parameter_info_get_type'.encode(), CEntryPoint_parameter_info_get_type)

def EntryPoint_parameter_info_get_type(obj):
	try:

		
		ret_val_types = (4096,)

		return (None, ret_val_types, obj.type)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)








@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_parameter_info_Releaseparameter_info(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_no_ret(py_object(EntryPoint_parameter_info_Releaseparameter_info), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_parameter_info_Releaseparameter_info'.encode(), CEntryPoint_parameter_info_Releaseparameter_info)

def EntryPoint_parameter_info_Releaseparameter_info(this_instance):
	try:
		# xcall release object
		
		python_plugin_handle.release_object(this_instance)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)







@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_function_info_get_name(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_function_info_get_name), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_function_info_get_name'.encode(), CEntryPoint_function_info_get_name)

def EntryPoint_function_info_get_name(obj):
	try:

		
		ret_val_types = (4096,)

		return (None, ret_val_types, obj.name)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_function_info_get_comment(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_function_info_get_comment), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_function_info_get_comment'.encode(), CEntryPoint_function_info_get_comment)

def EntryPoint_function_info_get_comment(obj):
	try:

		
		ret_val_types = (4096,)

		return (None, ret_val_types, obj.comment)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_function_info_get_parameters(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_function_info_get_parameters), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_function_info_get_parameters'.encode(), CEntryPoint_function_info_get_parameters)

def EntryPoint_function_info_get_parameters(obj):
	try:

		
		ret_val_types = (98304,)

		return (None, ret_val_types, obj.parameters)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_function_info_get_return_values(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_function_info_get_return_values), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_function_info_get_return_values'.encode(), CEntryPoint_function_info_get_return_values)

def EntryPoint_function_info_get_return_values(obj):
	try:

		
		ret_val_types = (69632,)

		return (None, ret_val_types, obj.return_values)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)








@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_function_info_Releasefunction_info(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_no_ret(py_object(EntryPoint_function_info_Releasefunction_info), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_function_info_Releasefunction_info'.encode(), CEntryPoint_function_info_Releasefunction_info)

def EntryPoint_function_info_Releasefunction_info(this_instance):
	try:
		# xcall release object
		
		python_plugin_handle.release_object(this_instance)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)







@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_class_info_get_name(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_class_info_get_name), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_class_info_get_name'.encode(), CEntryPoint_class_info_get_name)

def EntryPoint_class_info_get_name(obj):
	try:

		
		ret_val_types = (4096,)

		return (None, ret_val_types, obj.name)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_class_info_get_comment(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_class_info_get_comment), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_class_info_get_comment'.encode(), CEntryPoint_class_info_get_comment)

def EntryPoint_class_info_get_comment(obj):
	try:

		
		ret_val_types = (4096,)

		return (None, ret_val_types, obj.comment)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_class_info_get_fields(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_class_info_get_fields), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_class_info_get_fields'.encode(), CEntryPoint_class_info_get_fields)

def EntryPoint_class_info_get_fields(obj):
	try:

		
		ret_val_types = (98304,)

		return (None, ret_val_types, obj.fields)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_class_info_get_methods(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_class_info_get_methods), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_class_info_get_methods'.encode(), CEntryPoint_class_info_get_methods)

def EntryPoint_class_info_get_methods(obj):
	try:

		
		ret_val_types = (98304,)

		return (None, ret_val_types, obj.methods)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)








@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_class_info_Releaseclass_info(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_no_ret(py_object(EntryPoint_class_info_Releaseclass_info), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_class_info_Releaseclass_info'.encode(), CEntryPoint_class_info_Releaseclass_info)

def EntryPoint_class_info_Releaseclass_info(this_instance):
	try:
		# xcall release object
		
		python_plugin_handle.release_object(this_instance)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)







@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_py_info_get_globals(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_py_info_get_globals), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_py_info_get_globals'.encode(), CEntryPoint_py_info_get_globals)

def EntryPoint_py_info_get_globals(obj):
	try:

		
		ret_val_types = (98304,)

		return (None, ret_val_types, obj.globals)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_py_info_get_functions(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_py_info_get_functions), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_py_info_get_functions'.encode(), CEntryPoint_py_info_get_functions)

def EntryPoint_py_info_get_functions(obj):
	try:

		
		ret_val_types = (98304,)

		return (None, ret_val_types, obj.functions)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_py_info_get_classes(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_py_info_get_classes), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_py_info_get_classes'.encode(), CEntryPoint_py_info_get_classes)

def EntryPoint_py_info_get_classes(obj):
	try:

		
		ret_val_types = (98304,)

		return (None, ret_val_types, obj.classes)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)








@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_py_info_Releasepy_info(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_no_ret(py_object(EntryPoint_py_info_Releasepy_info), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_py_info_Releasepy_info'.encode(), CEntryPoint_py_info_Releasepy_info)

def EntryPoint_py_info_Releasepy_info(this_instance):
	try:
		# xcall release object
		
		python_plugin_handle.release_object(this_instance)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)




@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_py_extractor_py_extractor(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_py_extractor_py_extractor), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_py_extractor_py_extractor'.encode(), CEntryPoint_py_extractor_py_extractor)

def EntryPoint_py_extractor_py_extractor(filename):
	try:
		# call constructor
		new_instance = py_extractor.py_extractor(filename)
		
		
		ret_val_types = (32768,)

		return ( None, ret_val_types , new_instance)

	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)





@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_py_extractor_extract(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_ret(py_object(EntryPoint_py_extractor_extract), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_py_extractor_extract'.encode(), CEntryPoint_py_extractor_extract)

def EntryPoint_py_extractor_extract(this_instance):
	try:
		# call method
		info = this_instance.extract()
		
		
		ret_val_types = (32768,)

		return ( None, ret_val_types , info)
		
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)
	



@CFUNCTYPE(None, c_void_p, POINTER(c_char_p), POINTER(c_ulonglong))
def CEntryPoint_py_extractor_Releasepy_extractor(cdts, out_err, out_err_len):
	global python_plugin_handle
	python_plugin_handle.xcall_params_no_ret(py_object(EntryPoint_py_extractor_Releasepy_extractor), c_void_p(cdts), out_err, out_err_len)
python_plugin_handle.set_entrypoint('EntryPoint_py_extractor_Releasepy_extractor'.encode(), CEntryPoint_py_extractor_Releasepy_extractor)

def EntryPoint_py_extractor_Releasepy_extractor(this_instance):
	try:
		# xcall release object
		
		python_plugin_handle.release_object(this_instance)
	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata),)




