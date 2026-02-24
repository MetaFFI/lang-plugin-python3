#include "python_compiler_plugin.h"

#include <runtime_manager/cpython3/python_api_wrapper.h>
#include <utils/safe_func.h>
#include <utils/logger.hpp>
#include <filesystem>
#include <stdexcept>
#include <cstdio>

static auto LOG = metaffi::get_logger("python3.compiler");

void PythonCompilerPlugin::initialize_python()
{
    fprintf(stderr, "+++ py_compiler_plugin: initialize_python enter (m_runtime=%s)\n", m_runtime ? "set" : "null"); fflush(stderr);
    if (m_runtime)
    {
        fprintf(stderr, "+++ py_compiler_plugin: initialize_python already done, returning\n"); fflush(stderr);
        return;
    }

    // Auto-detect Python version
    fprintf(stderr, "+++ py_compiler_plugin: detect_installed_python3 begin\n"); fflush(stderr);
    auto versions = cpython3_runtime_manager::detect_installed_python3();
    fprintf(stderr, "+++ py_compiler_plugin: detect_installed_python3 done (count=%zu)\n", versions.size()); fflush(stderr);
    if (versions.empty())
    {
        throw std::runtime_error("No Python 3.x installation found");
    }

    // Use latest available version (last in list)
    fprintf(stderr, "+++ py_compiler_plugin: cpython3_runtime_manager::create(%s) begin\n", versions.back().c_str()); fflush(stderr);
    m_runtime = cpython3_runtime_manager::create(versions.back());
    fprintf(stderr, "+++ py_compiler_plugin: cpython3_runtime_manager::create done\n"); fflush(stderr);

    // Add SDK path to sys.path for importing sdk.compiler.python3.host.*
    // SDK is expected to be in METAFFI_HOME or current working directory
    char* metaffi_home = metaffi_getenv_alloc("METAFFI_HOME");
    if (metaffi_home)
    {
        std::filesystem::path sdk_path = std::filesystem::path(metaffi_home);
        m_runtime->add_sys_path(sdk_path.string());

        // Also add the compiler subdirectory where Python dependencies are copied
        std::filesystem::path compiler_path = sdk_path / "python3" / "compiler";
        m_runtime->add_sys_path(compiler_path.string());

        metaffi_free_env(metaffi_home);
    }
    else
    {
        // Fallback to current directory
        std::filesystem::path sdk_path = std::filesystem::current_path();
        m_runtime->add_sys_path(sdk_path.string());
    }

    // If METAFFI_SOURCE_ROOT is set, add the Python API path for 'metaffi' module
    char* metaffi_source_root = metaffi_getenv_alloc("METAFFI_SOURCE_ROOT");
    if (metaffi_source_root)
    {
        std::filesystem::path api_path = std::filesystem::path(metaffi_source_root) / "sdk" / "api" / "python3";
        m_runtime->add_sys_path(api_path.string());
        metaffi_free_env(metaffi_source_root);
    }
    fprintf(stderr, "+++ py_compiler_plugin: initialize_python done\n"); fflush(stderr);
}

void PythonCompilerPlugin::execute_host_compiler(
    const std::string& idl_def_json,
    const std::string& output_path,
    const std::string& host_options)
{
    fprintf(stderr, "+++ py_compiler_plugin: execute_host_compiler enter\n"); fflush(stderr);
    initialize_python();

    fprintf(stderr, "+++ py_compiler_plugin: acquire_gil begin\n"); fflush(stderr);
    // Acquire GIL for Python operations
    auto gil = m_runtime->acquire_gil();
    fprintf(stderr, "+++ py_compiler_plugin: acquire_gil done\n"); fflush(stderr);

    // Import host_compiler module
    PyObject* host_compiler_module = pPyImport_ImportModule("sdk.compiler.python3.host.host_compiler");
    if (!host_compiler_module)
    {
        pPyErr_Print();
        throw std::runtime_error("Failed to import sdk.compiler.python3.host.host_compiler");
    }

    // Import compiler_context module
    PyObject* context_module = pPyImport_ImportModule("sdk.compiler.python3.host.compiler_context");
    if (!context_module)
    {
        pPyErr_Print();
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to import sdk.compiler.python3.host.compiler_context");
    }

    // Import idl_entities.model module
    PyObject* idl_entities_module = pPyImport_ImportModule("sdk.idl_entities.python3.idl_entities.model");
    if (!idl_entities_module)
    {
        pPyErr_Print();
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to import sdk.idl_entities.python3.idl_entities.model");
    }

    // Import metaffi_types module
    PyObject* metaffi_types_module = pPyImport_ImportModule("metaffi.metaffi_types");
    if (!metaffi_types_module)
    {
        pPyErr_Print();
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to import metaffi.metaffi_types");
    }

    // Get CompilerContext class
    PyObject* context_class = pPyObject_GetAttrString(context_module, "CompilerContext");
    if (!context_class)
    {
        pPyErr_Print();
        Py_DECREF(metaffi_types_module);
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to get CompilerContext class");
    }

    // Create CompilerContext instance: CompilerContext(idl_entities_module, metaffi_types_module)
    PyObject* context_args = pPyTuple_New(2);
    Py_INCREF(idl_entities_module);  // PyTuple_SetItem steals reference
    pPyTuple_SetItem(context_args, 0, idl_entities_module);
    Py_INCREF(metaffi_types_module);
    pPyTuple_SetItem(context_args, 1, metaffi_types_module);

    PyObject* context = pPyObject_CallObject(context_class, context_args);
    Py_DECREF(context_args);
    Py_DECREF(context_class);

    if (!context)
    {
        pPyErr_Print();
        Py_DECREF(metaffi_types_module);
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to create CompilerContext instance");
    }

    // Get IDLDefinition class
    PyObject* idl_def_class = pPyObject_GetAttrString(idl_entities_module, "IDLDefinition");
    if (!idl_def_class)
    {
        pPyErr_Print();
        Py_DECREF(context);
        Py_DECREF(metaffi_types_module);
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to get IDLDefinition class");
    }

    // Call IDLDefinition.from_json(idl_def_json)
    PyObject* from_json_method = pPyObject_GetAttrString(idl_def_class, "from_json");
    if (!from_json_method)
    {
        pPyErr_Print();
        Py_DECREF(idl_def_class);
        Py_DECREF(context);
        Py_DECREF(metaffi_types_module);
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to get IDLDefinition.from_json method");
    }

    PyObject* json_str = pPyUnicode_FromStringAndSize(idl_def_json.c_str(), idl_def_json.length());
    PyObject* json_args = pPyTuple_New(1);
    pPyTuple_SetItem(json_args, 0, json_str);

    PyObject* definition = pPyObject_CallObject(from_json_method, json_args);
    Py_DECREF(json_args);
    Py_DECREF(from_json_method);
    Py_DECREF(idl_def_class);

    if (!definition)
    {
        pPyErr_Print();
        Py_DECREF(context);
        Py_DECREF(metaffi_types_module);
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to parse IDL JSON to IDLDefinition");
    }

    // Get HostCompiler class
    PyObject* compiler_class = pPyObject_GetAttrString(host_compiler_module, "HostCompiler");
    if (!compiler_class)
    {
        pPyErr_Print();
        Py_DECREF(definition);
        Py_DECREF(context);
        Py_DECREF(metaffi_types_module);
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to get HostCompiler class");
    }

    // Create HostCompiler instance: HostCompiler(context)
    PyObject* compiler_args = pPyTuple_New(1);
    Py_INCREF(context);
    pPyTuple_SetItem(compiler_args, 0, context);

    PyObject* compiler = pPyObject_CallObject(compiler_class, compiler_args);
    Py_DECREF(compiler_args);
    Py_DECREF(compiler_class);

    if (!compiler)
    {
        pPyErr_Print();
        Py_DECREF(definition);
        Py_DECREF(context);
        Py_DECREF(metaffi_types_module);
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to create HostCompiler instance");
    }

    // Extract output_dir and output_filename from output_path
    std::filesystem::path p(output_path);
    std::string output_dir = p.parent_path().string();
    std::string output_filename = p.stem().string();

    // If output_path is just a directory (no filename), use it as output_dir
    if (std::filesystem::is_directory(p) || output_filename.empty())
    {
        output_dir = output_path;
        output_filename = "host";
    }

    // Call compiler.compile(definition, output_dir, output_filename, host_options)
    PyObject* compile_method = pPyObject_GetAttrString(compiler, "compile");
    if (!compile_method)
    {
        pPyErr_Print();
        Py_DECREF(compiler);
        Py_DECREF(definition);
        Py_DECREF(context);
        Py_DECREF(metaffi_types_module);
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Failed to get HostCompiler.compile method");
    }

    // Build compile arguments: (definition, output_dir, output_filename, host_options)
    PyObject* compile_args = pPyTuple_New(4);
    Py_INCREF(definition);
    pPyTuple_SetItem(compile_args, 0, definition);
    pPyTuple_SetItem(compile_args, 1, pPyUnicode_FromString(output_dir.c_str()));
    pPyTuple_SetItem(compile_args, 2, pPyUnicode_FromString(output_filename.c_str()));

    // Parse host_options as key=value comma-separated string into dict
    PyObject* opts_dict = nullptr;
    if (!host_options.empty())
    {
        opts_dict = pPyDict_New();
        if (!opts_dict)
        {
            Py_DECREF(compiler);
            Py_DECREF(definition);
            Py_DECREF(context);
            Py_DECREF(metaffi_types_module);
            Py_DECREF(idl_entities_module);
            Py_DECREF(context_module);
            Py_DECREF(host_compiler_module);
            throw std::runtime_error("Failed to create host options dict");
        }

        auto trim = [](const std::string& s) -> std::string {
            size_t start = s.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) return "";
            size_t end = s.find_last_not_of(" \t\r\n");
            return s.substr(start, end - start + 1);
        };

        size_t pos = 0;
        while (pos < host_options.size())
        {
            size_t comma = host_options.find(',', pos);
            std::string token = host_options.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
            token = trim(token);
            if (!token.empty())
            {
                size_t eq = token.find('=');
                if (eq == std::string::npos || eq == 0 || eq == token.size() - 1)
                {
                    Py_DECREF(opts_dict);
                    Py_DECREF(compiler);
                    Py_DECREF(definition);
                    Py_DECREF(context);
                    Py_DECREF(metaffi_types_module);
                    Py_DECREF(idl_entities_module);
                    Py_DECREF(context_module);
                    Py_DECREF(host_compiler_module);
                    throw std::runtime_error("Host options are invalid (expected key=value[,key=value...])");
                }

                std::string key = trim(token.substr(0, eq));
                std::string val = trim(token.substr(eq + 1));
                PyObject* py_key = pPyUnicode_FromString(key.c_str());
                PyObject* py_val = pPyUnicode_FromString(val.c_str());
                if (!py_key || !py_val)
                {
                    Py_XDECREF(py_key);
                    Py_XDECREF(py_val);
                    Py_DECREF(opts_dict);
                    Py_DECREF(compiler);
                    Py_DECREF(definition);
                    Py_DECREF(context);
                    Py_DECREF(metaffi_types_module);
                    Py_DECREF(idl_entities_module);
                    Py_DECREF(context_module);
                    Py_DECREF(host_compiler_module);
                    throw std::runtime_error("Failed to create host option strings");
                }
                pPyDict_SetItem(opts_dict, py_key, py_val);
                Py_DECREF(py_key);
                Py_DECREF(py_val);
            }
            if (comma == std::string::npos)
            {
                break;
            }
            pos = comma + 1;
        }
    }

    if (opts_dict)
    {
        pPyTuple_SetItem(compile_args, 3, opts_dict);
    }
    else
    {
        Py_INCREF(pPy_None);
        pPyTuple_SetItem(compile_args, 3, pPy_None);
    }

    fprintf(stderr, "+++ py_compiler_plugin: calling HostCompiler.compile\n"); fflush(stderr);
    PyObject* result = pPyObject_CallObject(compile_method, compile_args);
    fprintf(stderr, "+++ py_compiler_plugin: HostCompiler.compile returned (result=%s)\n", result ? "ok" : "null"); fflush(stderr);
    Py_DECREF(compile_args);
    Py_DECREF(compile_method);

    if (!result)
    {
        pPyErr_Print();
        Py_DECREF(compiler);
        Py_DECREF(definition);
        Py_DECREF(context);
        Py_DECREF(metaffi_types_module);
        Py_DECREF(idl_entities_module);
        Py_DECREF(context_module);
        Py_DECREF(host_compiler_module);
        throw std::runtime_error("Compilation failed");
    }

    // Cleanup all PyObject references
    fprintf(stderr, "+++ py_compiler_plugin: Py_DECREF cleanup begin\n"); fflush(stderr);
    Py_DECREF(result);
    Py_DECREF(compiler);
    Py_DECREF(definition);
    Py_DECREF(context);
    Py_DECREF(metaffi_types_module);
    Py_DECREF(idl_entities_module);
    Py_DECREF(context_module);
    Py_DECREF(host_compiler_module);
    fprintf(stderr, "+++ py_compiler_plugin: execute_host_compiler done\n"); fflush(stderr);
}

void PythonCompilerPlugin::init()
{
    initialize_python();
    METAFFI_INFO(LOG, "initialized");
}

void PythonCompilerPlugin::compile_to_guest(
    const std::string& idl_def_json,
    const std::string& output_path,
    const std::string& guest_options)
{
    throw std::runtime_error("compile_to_guest is NOT IMPLEMENTED for Python3 compiler plugin");
}

void PythonCompilerPlugin::compile_from_host(
    const std::string& idl_def_json,
    const std::string& output_path,
    const std::string& host_options)
{
    execute_host_compiler(idl_def_json, output_path, host_options);
}

PythonCompilerPlugin::~PythonCompilerPlugin()
{
    fprintf(stderr, "+++ py_compiler_plugin: ~PythonCompilerPlugin enter (m_runtime=%s)\n", m_runtime ? "set" : "null"); fflush(stderr);
    // Runtime manager handles cleanup automatically via RAII
    m_runtime.reset();
    fprintf(stderr, "+++ py_compiler_plugin: ~PythonCompilerPlugin m_runtime.reset() done\n"); fflush(stderr);
}
