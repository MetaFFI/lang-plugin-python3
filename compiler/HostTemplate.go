package main

const HostHeaderTemplate = `
# Code generated by MetaFFI. DO NOT EDIT.
# Guest code for {{.IDLFilenameWithExtension}}
`

const HostImports = `
from ctypes import *
import ctypes.util
from typing import List
from typing import Any
from typing import Tuple
import platform
import os
from enum import Enum
`

const HostHelperFunctions = `
xllr_handle = None
python_plugin_handle = None

def free_metaffi():
	global xllr_handle
	global runtime_plugin

	err = pointer((c_char * 1)(0))
	err_len = (c_ulonglong)(0)
	xllr_handle.free_runtime_plugin(runtime_plugin, len(runtime_plugin), byref(err), byref(err_len))

def load_xllr_and_python_plugin():
	global xllr_handle
	global python_plugin_handle
	
	if xllr_handle == None:
		xllr_handle = cdll.LoadLibrary(get_filename_to_load('xllr'))

	if python_plugin_handle == None:
		python_plugin_handle = cdll.LoadLibrary(get_filename_to_load('xllr.python3'))

		err = pointer((c_char * 1)(0))
		err_len = (c_ulonglong)(0)
		python_plugin_handle.load_runtime(byref(err), byref(err_len)) # in order to initialize python environment (e.g. define metaffi_handle class)

	# set restypes
	python_plugin_handle.convert_host_params_to_cdts.argstype = [py_object, py_object]
	python_plugin_handle.convert_host_params_to_cdts.restype = c_void_p
	python_plugin_handle.convert_host_return_values_from_cdts.argstype = [c_void_p, c_uint64]
	python_plugin_handle.convert_host_return_values_from_cdts.restype = py_object
	xllr_handle.alloc_cdts_buffer.restype = c_void_p
	xllr_handle.load_function.restype = CFUNCTYPE(None)

def get_filename_to_load(fname):
	osname = platform.system()
	if os.getenv('METAFFI_HOME') is None:
		raise RuntimeError('No METAFFI_HOME environment variable')
	elif fname is None:
		raise RuntimeError('fname is None')

	if osname == 'Windows':
		return os.getenv('METAFFI_HOME')+'\\'+ fname + '.dll'
	elif osname == 'Darwin':
		return os.getenv('METAFFI_HOME')+'/' + fname + '.dylib'
	else:
		return os.getenv('METAFFI_HOME')+'/' + fname + '.so' # for everything that is not windows or mac, return .so

`

const (
	HostFunctionStubsTemplate = `
{{ $idl := . }}
cfunctype_params_ret = CFUNCTYPE(None)
cfunctype_params_ret.argtypes = [c_void_p, POINTER(POINTER(c_ubyte)), POINTER(c_ulonglong)]
cfunctype_params_no_ret = CFUNCTYPE(None)
cfunctype_params_no_ret.argtypes = [c_void_p, POINTER(POINTER(c_ubyte)), POINTER(c_ulonglong)]
cfunctype_no_params_ret = CFUNCTYPE(None)
cfunctype_no_params_ret.argtypes = [c_void_p, POINTER(POINTER(c_ubyte)), POINTER(c_ulonglong)]
cfunctype_no_params_no_ret = CFUNCTYPE(None)
cfunctype_no_params_no_ret.argtypes = [POINTER(POINTER(c_ubyte)), POINTER(c_ulonglong)]

runtime_plugin = """xllr.{{$.TargetLanguage}}""".encode("utf-8")
{{range $mindex, $m := .Modules}}

{{range $gindex, $g := $m.Globals}}
{{if $g.Getter}}{{$g.Getter.Name}}_id = c_void_p(0){{end}}
{{if $g.Setter}}{{$g.Setter.Name}}_id = c_void_p(0){{end}}
{{end}}

{{range $findex, $f := $m.Functions}}
{{$f.Name}}_id = c_void_p(0)
{{end}}

{{range $cindex, $c := $m.Classes}}

{{range $findex, $f := $c.Fields}}
{{if $f.Getter}}{{$c.Name}}_{{$f.Getter.Name}}_id = c_void_p(0){{end}}
{{if $f.Setter}}{{$c.Name}}_{{$f.Setter.Name}}_id = c_void_p(0){{end}}
{{end}}

{{range $cstrindex, $cstr := $c.Constructors}}
{{$c.Name}}_{{$cstr.Name}}_id = c_void_p(0)
{{end}}

{{range $methindex, $meth := $c.Methods}}
{{$c.Name}}_{{$meth.Name}}_id = c_void_p(0)
{{end}}

{{if $c.Releaser}}{{$c.Name}}_{{$c.Releaser.Name}}_id = c_void_p(0){{end}}

{{end}}
{{end}}
def load(module_path: str)->None:
	# load foreign functions
	load_xllr_and_python_plugin()

	{{range $mindex, $m := .Modules}}
	{{range $gindex, $g := $m.Globals}}
	{{if $g.Getter}}global {{$g.Getter.Name}}_id{{end}}
	{{if $g.Setter}}global {{$g.Setter.Name}}_id{{end}}
	{{end}}

	{{range $findex, $f := $m.Functions}}
	global {{$f.Name}}_id
	{{end}}

	{{range $cindex, $c := $m.Classes}}

	{{range $findex, $f := $c.Fields}}
	{{if $f.Getter}}global {{$c.Name}}_{{$f.Getter.Name}}_id{{end}}
	{{if $f.Setter}}global {{$c.Name}}_{{$f.Setter.Name}}_id{{end}}
	{{end}}

	{{range $cstrindex, $cstr := $c.Constructors}}
	global {{$c.Name}}_{{$cstr.Name}}_id
	{{end}}

	{{range $methindex, $meth := $c.Methods}}
	global {{$c.Name}}_{{$meth.Name}}_id
	{{end}}


	{{if $c.Releaser}}global {{$c.Name}}_{{$c.Releaser.Name}}_id{{end}}

	{{end}}
	{{end}}

	err = POINTER(c_ubyte)()
	out_err = POINTER(POINTER(c_ubyte))(c_void_p(addressof(err)))
	err_len = c_uint32()
	out_err_len = POINTER(c_uint32)(c_void_p(addressof(err_len)))

	{{ $idl := . }}
	{{range $mindex, $m := .Modules}}

	{{range $findex, $f := $m.Globals}}
	{{if $f.Getter}}
	{{$f.Getter.Name}}_id = cast(xllr_handle.load_function(runtime_plugin, len(runtime_plugin), module_path.encode("utf-8"), len(module_path.encode("utf-8")), '{{$f.Getter.FunctionPathAsString $idl}}'.encode("utf-8"), len('{{$f.Getter.FunctionPathAsString $idl}}'.encode("utf-8")), {{$f.Getter.Name}}_id, {{len $f.Getter.Parameters}}, {{len $f.Getter.ReturnValues}}, out_err, out_err_len), {{GetCFuncType $f.Getter.Parameters $f.Getter.ReturnValues}})
	if not bool({{$f.Getter.Name}}_id): # failed to load function
		err_text = string_at(out_err.contents, out_err_len.contents.value)
		raise RuntimeError('\n'+str(err_text).replace("\\n", "\n"))
	{{end}}
	{{if $f.Setter}}
	{{$f.Setter.Name}}_id = cast(xllr_handle.load_function(runtime_plugin, len(runtime_plugin), module_path.encode("utf-8"), len(module_path.encode("utf-8")), '{{$f.Setter.FunctionPathAsString $idl}}'.encode("utf-8"), len('{{$f.Setter.FunctionPathAsString $idl}}'.encode("utf-8")), {{$f.Setter.Name}}_id, {{len $f.Setter.Parameters}}, {{len $f.Setter.ReturnValues}}, out_err, out_err_len), {{GetCFuncType $f.Setter.Parameters $f.Setter.ReturnValues}})
	if not bool({{$f.Setter.Name}}_id): # failed to load function
		err_text = string_at(out_err.contents, out_err_len.contents.value)
		raise RuntimeError('\n'+str(err_text).replace("\\n", "\n"))
	{{end}}
	{{end}} {{/* end globals */}}

	{{range $findex, $f := $m.Functions}}
	{{$f.Name}}_id = cast(xllr_handle.load_function(runtime_plugin, len(runtime_plugin), module_path.encode("utf-8"), len(module_path.encode("utf-8")), '{{$f.FunctionPathAsString $idl}}'.encode("utf-8"), len('{{$f.FunctionPathAsString $idl}}'.encode("utf-8")), {{$f.Name}}_id, {{len $f.Parameters}}, {{len $f.ReturnValues}}, out_err, out_err_len), {{GetCFuncType $f.Parameters $f.ReturnValues}})
	if not bool({{$f.Name}}_id): # failed to load function
		err_text = string_at(out_err.contents, out_err_len.contents.value)
		raise RuntimeError('\n'+str(err_text).replace("\\n", "\n"))
	{{end}}

	{{range $cindex, $c := $m.Classes}}
	{{range $cstrindex, $cstr := $c.Constructors}}
	{{$c.Name}}_{{$cstr.Name}}_id = cast(xllr_handle.load_function(runtime_plugin, len(runtime_plugin), module_path.encode("utf-8"), len(module_path.encode("utf-8")), '{{$cstr.FunctionPathAsString $idl}}'.encode("utf-8"), len('{{$cstr.FunctionPathAsString $idl}}'.encode("utf-8")), {{$c.Name}}_{{$cstr.Name}}_id, {{len $cstr.Parameters}}, {{len $cstr.ReturnValues}}, out_err, out_err_len), {{GetCFuncType $cstr.Parameters $cstr.ReturnValues}})
	if not bool({{$c.Name}}_{{$cstr.Name}}_id): # failed to load function
		err_text = string_at(out_err.contents, out_err_len.contents.value)
		raise RuntimeError('\n'+str(err_text).replace("\\n", "\n"))
	{{end}}

	{{range $findex, $f := $c.Fields}}
	{{if $f.Getter}}
	{{$c.Name}}_{{$f.Getter.Name}}_id = cast(xllr_handle.load_function(runtime_plugin, len(runtime_plugin), module_path.encode("utf-8"), len(module_path.encode("utf-8")), '{{$f.Getter.FunctionPathAsString $idl}}'.encode("utf-8"), len('{{$f.Getter.FunctionPathAsString $idl}}'.encode("utf-8")), {{$c.Name}}_{{$f.Getter.Name}}_id, {{len $f.Getter.Parameters}}, {{len $f.Getter.ReturnValues}}, out_err, out_err_len), {{GetCFuncType $f.Getter.Parameters $f.Getter.ReturnValues}})
	if not bool({{$c.Name}}_{{$f.Getter.Name}}_id): # failed to load function
		err_text = string_at(out_err.contents, out_err_len.contents.value)
		raise RuntimeError('\n'+str(err_text).replace("\\n", "\n"))
	{{end}}
	{{if $f.Setter}}
	{{$c.Name}}_{{$f.Setter.Name}}_id = cast(xllr_handle.load_function(runtime_plugin, len(runtime_plugin), module_path.encode("utf-8"), len(module_path.encode("utf-8")), '{{$f.Setter.FunctionPathAsString $idl}}'.encode("utf-8"), len('{{$f.Setter.FunctionPathAsString $idl}}'.encode("utf-8")), {{$c.Name}}_{{$f.Setter.Name}}_id, {{len $f.Setter.Parameters}}, {{len $f.Setter.ReturnValues}}, out_err, out_err_len), {{GetCFuncType $f.Setter.Parameters $f.Setter.ReturnValues}})
	if not bool({{$c.Name}}_{{$f.Setter.Name}}_id): # failed to load function
		err_text = string_at(out_err.contents, out_err_len.contents.value)
		raise RuntimeError('\n'+str(err_text).replace("\\n", "\n"))
	{{end}}
	{{end}} {{/* End fields */}}

	{{range $methindex, $meth := $c.Methods}}
	{{$c.Name}}_{{$meth.Name}}_id = cast(xllr_handle.load_function(runtime_plugin, len(runtime_plugin), module_path.encode("utf-8"), len(module_path.encode("utf-8")), '{{$meth.FunctionPathAsString $idl}}'.encode("utf-8"), len('{{$meth.FunctionPathAsString $idl}}'.encode("utf-8")), {{$c.Name}}_{{$meth.Name}}_id, {{len $meth.Parameters}}, {{len $meth.ReturnValues}}, out_err, out_err_len), {{GetCFuncType $meth.Parameters $meth.ReturnValues}})
	if not bool({{$c.Name}}_{{$meth.Name}}_id): # failed to load function
		err_text = string_at(out_err.contents, out_err_len.contents.value)
		raise RuntimeError('\n'+str(err_text).replace("\\n", "\n"))
	{{end}}

	{{if $c.Releaser}}
	{{$c.Name}}_{{$c.Releaser.Name}}_id = cast(xllr_handle.load_function(runtime_plugin, len(runtime_plugin), module_path.encode("utf-8"), len(module_path.encode("utf-8")), '{{$c.Releaser.FunctionPathAsString $idl}}'.encode("utf-8"), len('{{$c.Releaser.FunctionPathAsString $idl}}'.encode("utf-8")), {{$c.Name}}_{{$c.Releaser.Name}}_id, {{len $c.Releaser.Parameters}}, {{len $c.Releaser.ReturnValues}}, out_err, out_err_len), {{GetCFuncType $c.Releaser.Parameters $c.Releaser.ReturnValues}})
	if not bool({{$c.Name}}_{{$c.Releaser.Name}}_id): # failed to load function
		err_text = string_at(out_err.contents, out_err_len.contents.value)
		raise RuntimeError('\n'+str(err_text).replace("\\n", "\n"))
	{{end}}

	{{end}}
	{{end}}
	

{{range $mindex, $m := .Modules}}
# Code to call foreign functions in module {{$m.Name}} via XLLR

# globals
{{range $findex, $f := $m.Globals}}
{{if $f.Getter}}
def {{$f.Getter.Name}}():

	{{GenerateCodeGlobals $f.Getter.Name 1}}

	{{GenerateCodeAllocateCDTS $f.Getter.Parameters $f.Getter.ReturnValues false}}

	# xcall function
	{{GenerateCodeXCall "" $f.Getter.Name $f.Getter.Parameters $f.Getter.ReturnValues 1}}


	{{GenerateCodeReturnValues $f.Getter.Parameters $f.Getter.ReturnValues}}
	{{GenerateCodeReturn $f.Getter.ReturnValues}}

{{end}} {{/* end getter */}}
{{if $f.Setter}}
def set_{{$f.Setter.Name}}():
	{{GenerateCodeGlobals $f.Setter.Name 1}}

	{{GenerateCodeAllocateCDTS $f.Setter.Parameters $f.Setter.ReturnValues false}}

	# xcall function
	{{GenerateCodeXCall "" $f.Setter.Name $f.Setter.Parameters $f.Setter.ReturnValues 1}}

	{{GenerateCodeReturnValues $f.Setter.Parameters $f.Setter.ReturnValues}}
	{{GenerateCodeReturn $f.ReturnValues}}

{{end}}{{/* end setter */}}
{{end}}{{/* end fields */}}

{{range $findex, $f := $m.Functions}}
# Call to foreign {{$f.Name}}
def {{$f.Name}}({{range $index, $elem := $f.Parameters}}{{if $index}},{{end}} {{$elem.Name}}:{{ConvertToPythonTypeFromField $elem}}{{end}}) -> ({{range $index, $elem := $f.ReturnValues}}{{if $index}},{{end}}{{ConvertToPythonTypeFromField $elem}}{{end}}):

	{{GenerateCodeGlobals $f.Name 1}}

	{{GenerateCodeAllocateCDTS $f.Parameters $f.ReturnValues false}}

	# xcall function
	{{GenerateCodeXCall "" $f.Name $f.Parameters $f.ReturnValues 1}}

	{{GenerateCodeReturnValues $f.Parameters $f.ReturnValues}}
	{{GenerateCodeReturn $f.ReturnValues}}
{{end}}

{{range $classindex, $c := $m.Classes}}
# Class to call methods of foreign class {{$c.Name}}
class {{$c.Name}}:
	
	{{range $cstrindex, $f := $c.Constructors}}
	def __init__(self {{range $index, $elem := $f.Parameters}}, {{$elem.Name}}:{{ConvertToPythonTypeFromField $elem}}{{end}}):
		self.obj_handle = None
		{{$fullName := (print $c.Name "_" $f.Name)}}
		{{GenerateCodeGlobals $fullName 2}}

		{{GenerateCodeAllocateCDTS $f.Parameters $f.ReturnValues true}}
	
		# xcall function
		{{GenerateCodeXCall $c.Name $f.Name $f.Parameters $f.ReturnValues 2}}

		# unpack results
	
		{{GenerateCodeReturnValues $f.Parameters $f.ReturnValues}}

		self.obj_handle = ret_vals[0] # NOTICE: assuming first ret_val is the handle
	{{end}}

	{{range $findex, $f := $c.Fields}}
	{{if $f.Getter}}
	def {{$f.Getter.Name}}(self):
		{{$fullName := (print $c.Name "_" $f.Getter.Name)}}
		{{GenerateCodeGlobals $fullName 2}}
	
		{{GenerateCodeAllocateCDTS $f.Getter.Parameters $f.Getter.ReturnValues true}}
	
		# xcall function
		{{GenerateCodeXCall $c.Name $f.Getter.Name $f.Getter.Parameters $f.Getter.ReturnValues 2}}

		{{GenerateCodeReturnValues $f.Getter.Parameters $f.Getter.ReturnValues}}
		{{GenerateCodeReturn $f.Getter.ReturnValues}}


	{{end}} {{/* end getter */}}
	{{if $f.Setter}}{{ $p := index $f.Setter.Parameters 1 }}
	def {{$f.Setter.Name}}(self, {{$p.Name}} ):
		{{$fullName := (print $c.Name "_" $f.Setter.Name)}}
		{{GenerateCodeGlobals $fullName 2}}

		{{GenerateCodeAllocateCDTS $f.Setter.Parameters $f.Setter.ReturnValues true}}

		# xcall function
		{{GenerateCodeXCall $c.Name $f.Setter.Name $f.Setter.Parameters $f.Setter.ReturnValues 2}}

		{{GenerateCodeReturnValues $f.Setter.Parameters $f.Setter.ReturnValues}}
		{{GenerateCodeReturn $f.Setter.ReturnValues}}
	{{end}}{{/* end setter */}}
	{{end}}{{/* end fields */}}

	{{if $c.Releaser}}{{$f := $c.Releaser}}
	# released foreign object handle
	def __del__(self):
		{{$fullName := (print $c.Name "_" $f.Name)}}
		{{GenerateCodeGlobals $fullName 2}}

		{{$paramsLength := len $f.Parameters}}{{$h := index $f.Parameters 0}}
		params = (self.obj_handle,)
		params_types = ({{GetMetaFFIType $h}},)
		xcall_params = python_plugin_handle.convert_host_params_to_cdts(py_object(params), py_object(params_types), 0)

		# xcall function
		{{GenerateCodeXCall "" $fullName $f.Parameters $f.ReturnValues 2}}
	{{end}}

	{{range $methindex, $f := $c.Methods}}
	def {{$f.Name}}{{GenerateMethodSignature $f}}:
		{{$fullName := (print $c.Name "_" $f.Name)}}
		{{GenerateCodeGlobals $fullName 2}}
	
		{{GenerateCodeAllocateCDTS $f.Parameters $f.ReturnValues true}}

		# xcall function
		{{GenerateCodeXCall "" $fullName $f.Parameters $f.ReturnValues 2}}
	
		# unpack results
		{{GenerateCodeReturnValues $f.Parameters $f.ReturnValues}}
		{{GenerateCodeReturn $f.ReturnValues}}
	{{end}}

{{/* declare static methods */}}
{{range $methindex, $f := $c.Methods}}
{{if not $f.InstanceRequired}}
{{$c.Name}}.{{$f.Name}} = staticmethod({{$c.Name}}.{{$f.Name}})
{{end}}
{{end}}

{{end}} {{/* End Class */}}

{{end}}
`
)
