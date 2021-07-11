package main


const GuestHeaderTemplate = `
# Code generated by OpenFFI. Modify only in marked places.
# Guest code for {{.IDLFilenameWithExtension}}
`

const GuestImportsTemplate = `
import traceback
import sys
import platform
import os
from ctypes import *

{{range $mindex, $i := .Imports}}
import {{$i}}{{end}}
`

const GuestHelperFunctions = `
xllrHandle = None

def load_xllr():
	global xllrHandle
	
	if xllrHandle == None:
		xllrHandle = cdll.LoadLibrary(get_filename_to_load('xllr'))

def get_filename_to_load(fname):
	osname = platform.system()
	if osname == 'Windows':
		return os.getenv('OPENFFI_HOME')+'\\'+ fname + '.dll'
	elif osname == 'Darwin':
		return os.getenv('OPENFFI_HOME')+'/' + fname + '.dylib'
	else:
		return os.getenv('OPENFFI_HOME')+'/' + fname + '.so' # for everything that is not windows or mac, return .so
`

const GuestFunctionXLLRTemplate = `
{{range $mindex, $m := .Modules}}
# Code to call foreign functions in module {{$m.Name}}
{{range $findex, $f := $m.Functions}}
# Call to foreign {{$f.PathToForeignFunction.function}}
def EntryPoint_{{$f.PathToForeignFunction.function}}({{range $index, $elem := $f.Parameters}}{{if $index}},{{end}}{{$elem.Name}}{{end}}):
	global xllrHandle

	load_xllr()

	try:
		# call function
		{{range $index, $elem := $f.ReturnValues}}{{if $index}},{{end}}{{$elem.Name}}{{end}}{{if $f.ReturnValues}} = {{end}}{{$f.PathToForeignFunction.module}}.{{$f.PathToForeignFunction.function}}({{range $index, $elem := $f.Parameters}}{{if $index}},{{end}}{{$elem.Name}}{{end}})
		{{$retvalLength := len $f.ReturnValues}}
		ret_val_types = ({{range $index, $elem := $f.ReturnValues}}{{if $index}}, {{end}}{{GetOpenFFIType $elem}}{{end}}{{if eq $retvalLength 1}},{{end}})

		return ( None, ret_val_types {{range $index, $elem := $f.ReturnValues}}, {{$elem.Name}}{{end}})

	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata))

{{end}}
{{end}}
`
