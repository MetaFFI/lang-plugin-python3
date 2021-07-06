
# Code generated by OpenFFI. Modify only in marked places.
# Guest code for test.proto

import traceback
import sys
import platform
import os
from ctypes import *


import GuestCode

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


# Code to call foreign functions in module Service1

# Call to foreign f1
def EntryPoint_f1(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14):
	global xllrHandle

	load_xllr()

	try:
		# call function
		r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14 = GuestCode.f1(p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14)
		
		ret_val_types = (1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 67584, 65600)

		return ( None, ret_val_types , r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14)

	except Exception as e:
		errdata = traceback.format_exception(*sys.exc_info())
		return ('\n'.join(errdata))



