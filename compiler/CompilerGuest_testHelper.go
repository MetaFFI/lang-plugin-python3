//+build GuestTest

package main

/*
#cgo !windows LDFLAGS: -L. -ldl -L/home/tcs/src/github.com/OpenFFI/openffi-core/cmake-build-debug

#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>

int call_guest_test()
{
	const char* openffi_home = getenv("OPENFFI_HOME");
	char lib_dir[100] = {0};
	sprintf(lib_dir, "%s/xllr.test.so", openffi_home);

	void* lib_handle = dlopen(lib_dir, RTLD_NOW);
	if(!lib_handle)
	{
		printf("Failed loading library - %s\n", dlerror());
		return -1;
	}

	void* res = dlsym(lib_handle, "test_guest");
	if(!res)
	{
		printf("Failed loading symbol - %s\n", dlerror());
		return -1;
	}

	return ((int (*) (const char*, const char*))res)("xllr.python3", "package=GuestCode,function=f1,openffi_guest_lib=test_OpenFFIGuest,entrypoint_function=EntryPoint_f1");
}
*/
import "C"

func CallHostMock() int{
	return int(C.call_guest_test())
}
