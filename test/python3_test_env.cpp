#include "python3_test_env.h"

#include <utils/env_utils.h>

#include <iostream>
#include <stdexcept>

namespace
{
std::string require_env(const char* name)
{
	std::string value = get_env_var(name);
	if(value.empty())
	{
		std::string msg = "Environment variable not set: ";
		msg += name;
		throw std::runtime_error(msg);
	}
	return value;
}

std::filesystem::path build_sdk_path(const std::filesystem::path& root,
                                     const std::initializer_list<const char*>& parts)
{
	std::filesystem::path result = root;
	for(const auto* part : parts)
	{
		result /= part;
	}
	return result;
}

std::string resolve_module_dir()
{
	auto root = std::filesystem::path(require_env("METAFFI_SOURCE_ROOT"));
	auto path = build_sdk_path(root, {
		"sdk", "test_modules", "guest_modules", "python3", "module"
	});
	return path.string();
}

std::string resolve_module_file()
{
	auto root = std::filesystem::path(require_env("METAFFI_SOURCE_ROOT"));
	auto path = build_sdk_path(root, {
		"sdk", "test_modules", "guest_modules", "python3", "single_file_module.py"
	});
	return path.string();
}
} // namespace

PythonTestEnv::PythonTestEnv()
	: runtime("python3"),
	  module_dir(runtime.runtime_plugin(), resolve_module_dir()),
	  module_file(runtime.runtime_plugin(), resolve_module_file()),
	  builtins(runtime.runtime_plugin(), "builtins"),
	  importlib(runtime.runtime_plugin(), "importlib"),
	  asyncio(runtime.runtime_plugin(), "asyncio")
{
	std::cerr << "+++ python3_test_env METAFFI_SOURCE_ROOT=" << get_env_var("METAFFI_SOURCE_ROOT") << std::endl;
	std::cerr << "+++ python3_test_env METAFFI_HOME=" << get_env_var("METAFFI_HOME") << std::endl;
	std::cerr << "+++ python3_test_env module_dir=" << resolve_module_dir() << std::endl;
	std::cerr << "+++ python3_test_env module_file=" << resolve_module_file() << std::endl;
	runtime.load_runtime_plugin();
	std::cerr << "+++ python3_test_env runtime_loaded=python3" << std::endl;
}

PythonTestEnv::~PythonTestEnv()
{
	runtime.release_runtime_plugin();
}

PythonTestEnv& python_test_env()
{
	static PythonTestEnv env;
	return env;
}
