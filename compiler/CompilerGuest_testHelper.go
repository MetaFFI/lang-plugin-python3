package main

/*
#cgo !windows LDFLAGS: -L. -ldl
#cgo windows LDFLAGS: -L.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef _WIN32
#include <dlfcn.h>
int call_guest_test()
{
	const char* metaffi_home = getenv("METAFFI_HOME");
	char lib_dir[260] = {0};
	sprintf(lib_dir, "%s/xllr.test.so", metaffi_home);

	void* lib_handle = dlopen(lib_dir, RTLD_NOW);
	if(!lib_handle)
	{
		printf("Failed loading library - %s\n", dlerror());
		return -1;
	}

	void* res = dlsym(lib_handle, "test_guest");
	if(!res)
	{
		printf("Failed loading symbol test_guest from xllr.test.so - %s\n", dlerror());
		return -1;
	}

	return ((int (*) (const char*, const char*))res)("xllr.python3", "package=GuestCode,function=f1,metaffi_guest_lib=test_MetaFFIGuest,entrypoint_function=EntryPoint_f1");
}
#else
#include <Windows.h>
int call_guest_test()
{
	const char* metaffi_home = getenv("METAFFI_HOME");
	char lib_dir[260] = {0};
	sprintf(lib_dir, "%s/xllr.test.dll", metaffi_home);

	void* lib_handle = LoadLibraryA(lib_dir);
	if(!lib_handle)
	{
		printf("Failed loading library %s - 0x%x\n", lib_dir, GetLastError());
		return -1;
	}

	void* res = GetProcAddress(lib_handle, "test_guest");
	if(!res)
	{
		printf("Failed loading symbol test_guest from xllr.test.dll - 0x%x\n", GetLastError());
		return -1;
	}

	return ((int (*) (const char*, const char*))res)("xllr.python3", "package=GuestCode,function=f1,metaffi_guest_lib=test_MetaFFIGuest,entrypoint_function=EntryPoint_f1");
}
#endif
*/
import "C"

func CallHostMock() int{
	return int(C.call_guest_test())
}
