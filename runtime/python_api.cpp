// SDK CDTS serializer (replaces cdts_python3.h)
#include <cdts_serializer/cpython3/cdts_python3_serializer.h>

#include <boost/filesystem.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <runtime/runtime_plugin_api.h>
#include <sstream>
#include <utility>
#include <utils/foreign_function.h>
#include <utils/entity_path_parser.h>
#include <utils/safe_func.h>

// SDK runtime manager and entity classes
#include <runtime_manager/cpython3/runtime_manager.h>
#include <runtime_manager/cpython3/python_api_wrapper.h>
#include <runtime_manager/cpython3/module.h>
#include <runtime_manager/cpython3/entity.h>
#include <runtime_manager/cpython3/gil_guard.h>
#include <runtime_manager/cpython3/python_utils.h>

// Local runtime globals
#include "runtime_globals.h"


using namespace metaffi::utils;
using metaffi::runtime::cpython3::check_python_error;

#define handle_err(err, desc)                           \
	{                                                   \
		auto err_len = strlen(desc);                    \
		*err = (char*)xllr_alloc_string(desc, err_len); \
	}

#define handle_err_str(err, descstr)                        \
	{                                                       \
		auto err_len = descstr.length();                    \
		*err = xllr_alloc_string(descstr.c_str(), err_len); \
	}

#define handle_py_err(err)                   \
	std::string pyerr(check_python_error()); \
	handle_err_str(err, pyerr);

#define TRUE 1
#define FALSE 0

std::vector<foreign_function_entrypoint_signature_params_ret_t> params_ret_functions;
std::vector<foreign_function_entrypoint_signature_params_no_ret_t> params_no_ret_functions;
std::vector<foreign_function_entrypoint_signature_no_params_ret_t> no_params_ret_functions;
std::vector<foreign_function_entrypoint_signature_no_params_no_ret_t> no_params_no_ret_functions;

enum attribute_action
{
	attribute_action_getter,
	attribute_action_setter
};

/**
 * Entity context - used by xcall functions
 *
 * For callables (functions, methods, constructors): uses SDK Entity
 * For attributes (getters, setters): uses current attribute logic
 */
struct entity_context {
	// SDK Entity for callables (null for attributes)
	std::shared_ptr<Module> module;
	std::shared_ptr<Entity> entity;

	// Type info for CDTS conversion (both callables and attributes)
	std::vector<metaffi_type_info> params_types;
	std::vector<metaffi_type_info> retvals_types;

	// Attribute-specific fields (only used when entity is null)
	attribute_action foreign_object_type;
	bool is_instance_required = false;
	std::vector<std::string> attribute_path;
	PyObject* entrypoint = nullptr;       // Attribute name as PyUnicode (for attributes)
	PyObject* attribute_holder = nullptr; // Object holding the attribute

	[[nodiscard]] uint8_t params_count() const { return (uint8_t)params_types.size(); }
	[[nodiscard]] uint8_t retvals_count() const { return (uint8_t)retvals_types.size(); }

	// Check if this context uses SDK Entity (callable) or attribute logic
	[[nodiscard]] bool is_callable() const { return entity != nullptr; }

	// For attributes: fills entrypoint and attribute_holder at call time
	// using the instance passed as the first parameter
	bool find_attribute_holder_and_attribute(PyObject* root)
	{
		if(attribute_path.empty())
		{
			return entrypoint && attribute_holder;
		}

		PyObject* parent = root;
		PyObject* pyobj = root;

		for(const std::string& step: attribute_path)
		{
			PyObject* temp = pPyObject_GetAttrString(pyobj, step.c_str());
			parent = pyobj;
			pyobj = temp;
			if(!pyobj)
			{
				return false;
			}
		}

		entrypoint = pPyUnicode_FromString(attribute_path[attribute_path.size() - 1].c_str());
		attribute_holder = parent;
		attribute_path.clear();

		return true;
	}
};

static PyObject* import_module_by_path(const std::string& module_path)
{
	std::filesystem::path p = std::filesystem::absolute(std::filesystem::path(module_path));

	if(std::filesystem::exists(p) && std::filesystem::is_regular_file(p))
	{
		std::filesystem::path dir = p.parent_path();
		std::string dir_str = dir.generic_string();

		PyObject* sysPath = pPySys_GetObject("path");
		PyObject* path = pPyUnicode_FromString(dir_str.c_str());
		if(path != nullptr)
		{
			if(pPySequence_Contains(sysPath, path) == 0)
			{
				pPyList_Append(sysPath, path);
			}
			Py_DECREF(path);
		}

		return pPyImport_ImportModuleLevel(p.stem().string().c_str(), pPy_None, pPy_None, pPy_None, 0);
	}

	if(std::filesystem::exists(p) && std::filesystem::is_directory(p))
	{
		std::filesystem::path dir = p.parent_path();
		std::string dir_str = dir.generic_string();

		PyObject* sysPath = pPySys_GetObject("path");
		PyObject* path = pPyUnicode_FromString(dir_str.c_str());
		if(path != nullptr)
		{
			if(pPySequence_Contains(sysPath, path) == 0)
			{
				pPyList_Append(sysPath, path);
			}
			Py_DECREF(path);
		}

		return pPyImport_ImportModule(p.filename().string().c_str());
	}

	return pPyImport_ImportModule(module_path.c_str());
}
//--------------------------------------------------------------------
std::unordered_map<std::string, void*> foreign_entities;
void set_entrypoint(const char* entrypoint_name, void* pfunction)
{
	foreign_entities[entrypoint_name] = pfunction;
}
// NOTE: initialize_environment() has been removed.
// The SDK's cpython3_runtime_manager handles sys.path initialization.
// METAFFI_SOURCE_ROOT and import_metaffi_package() are handled in load_runtime().
//--------------------------------------------------------------------
// SDK runtime manager - replaces global state variables
// g_runtime_manager is declared extern in runtime_globals.h for use by other files
std::shared_ptr<cpython3_runtime_manager> g_runtime_manager;
static std::mutex g_runtime_mutex;

void load_runtime(char** err)
{
	std::lock_guard<std::mutex> lock(g_runtime_mutex);

	if(g_runtime_manager && g_runtime_manager->is_runtime_loaded())
	{
		return;
	}

	// make sure g_runtime_manager is nullptr
	g_runtime_manager = nullptr;

	try
	{
		auto add_metaffi_sys_path = [](const std::shared_ptr<cpython3_runtime_manager>& manager)
		{
			char* source_root = metaffi_getenv_alloc("METAFFI_SOURCE_ROOT");
			if(source_root && *source_root)
			{
				std::filesystem::path dev_path = std::filesystem::path(source_root) / "sdk" / "api" / "python3";
				manager->add_sys_path(dev_path.string());
			}
			metaffi_free_env(source_root);
		};

		// First check if python is already loaded into the process
		auto already_loaded_python3 = cpython3_runtime_manager::load_loaded_cpython3();
		if(already_loaded_python3)
		{
			g_runtime_manager = already_loaded_python3;
			add_metaffi_sys_path(g_runtime_manager);
			g_runtime_manager->import_metaffi_package();
			return;
		}

		// Auto-detect Python version
		auto versions = cpython3_runtime_manager::detect_installed_python3();
		if(versions.empty())
		{
			handle_err(err, "No Python 3.8-3.13 installation found");
			return;
		}

		// Use first detected version - create() handles runtime loading internally
		g_runtime_manager = cpython3_runtime_manager::create(versions[0]);

		add_metaffi_sys_path(g_runtime_manager);

		// Import metaffi package using SDK runtime_manager (handles GIL internally)
		g_runtime_manager->import_metaffi_package();
	}
	catch(const std::exception& e)
	{
		handle_err_str(err, std::string(e.what()));
	}
}


//--------------------------------------------------------------------
void free_runtime(char** err)
{
	std::lock_guard<std::mutex> lock(g_runtime_mutex);

	if(!g_runtime_manager)
	{
		return;
	}

	try
	{
		g_runtime_manager->release_runtime();
		g_runtime_manager.reset();
	}
	catch(const std::exception& e)
	{
		handle_err_str(err, std::string(e.what()));
	}
}
//--------------------------------------------------------------------
void free_xcall(xcall* pxcall, char** err)
{
	void** pxcall_and_context = pxcall->pxcall_and_context;

	delete static_cast<entity_context*>(pxcall_and_context[1]);
	pxcall_and_context[1] = nullptr;
	pxcall_and_context[0] = nullptr;

	delete pxcall;

	*err = nullptr;
}
//--------------------------------------------------------------------
// NOTE: organize_arguments() has been removed.
// The SDK's CallableEntity::call() now handles varargs/kwargs internally.
//--------------------------------------------------------------------
void pyxcall_params_ret(
        entity_context* pctxt,
        cdts params_ret[2],
        char** out_err)
{
	try
	{
		gil_guard guard;

		// Convert CDTS to Python tuple using SDK serializer
		cdts_python3_serializer params_ser(*g_runtime_manager, params_ret[0]);
		PyObject* params = params_ser.extract_as_tuple();

		PyObject* res = nullptr;

		if(pctxt->is_callable())
		{
			// Use SDK Entity for callables
			auto* callable = dynamic_cast<CallableEntity*>(pctxt->entity.get());
			if(callable)
			{
				res = callable->call(params);
			}
			else
			{
				Py_DECREF(params);
				handle_err(out_err, "Entity is not callable");
				return;
			}
		}
		else
		{
			// Attribute logic (current implementation)
			if(!pctxt->attribute_path.empty())
			{
				if(!pctxt->is_instance_required)
				{
					Py_DECREF(params);
					handle_err(out_err, "non-object attribute should have been found");
					return;
				}

				if(!pctxt->find_attribute_holder_and_attribute(pPyTuple_GetItem(params, 0)))
				{
					Py_DECREF(params);
					handle_err(out_err, "attribute not found");
					return;
				}
			}

			if(pctxt->foreign_object_type == attribute_action_getter)
			{
				res = PyObject_GetAttr(pctxt->attribute_holder, pctxt->entrypoint);
			}
			else if(pctxt->foreign_object_type == attribute_action_setter)
			{
				PyObject_SetAttr(pctxt->attribute_holder, pctxt->entrypoint, pPyTuple_GetItem(params, 1));
			}
			else
			{
				Py_DECREF(params);
				handle_err(out_err, "Unknown foreign object type");
				return;
			}
		}

		std::string err_msg = std::move(check_python_error());
		if(!err_msg.empty())
		{
			Py_DECREF(params);
			handle_err_str((char**)out_err, err_msg);
			return;
		}

		// Convert result to CDTS using SDK serializer
		cdts_python3_serializer ret_ser(*g_runtime_manager, params_ret[1]);
		size_t retval_count = pctxt->retvals_types.size();

		if(retval_count == 1)
		{
			ret_ser.add(res, pctxt->retvals_types[0].type);
		}
		else if(retval_count > 1 && res && pPyTuple_Check(res))
		{
			for(size_t i = 0; i < retval_count; i++)
			{
				PyObject* item = pPyTuple_GetItem(res, i);
				ret_ser.add(item, pctxt->retvals_types[i].type);
			}
		}
		else if(retval_count > 1 && res)
		{
			ret_ser.add(res, pctxt->retvals_types[0].type);
		}

		Py_DECREF(params);

	} catch(const std::exception& err)
	{
		handle_err_str(out_err, std::string(err.what()));
	}
}
//--------------------------------------------------------------------
// IMPORTANT: 	the name of the function must be different from
//				xcall_params_ret, as "xcall_params_ret" is exported by xllr
//				which makes linux choose xllr's function instead of this one
void py_api_xcall_params_ret(void* context, cdts params_ret[2], char** out_err)
{
	entity_context* pctxt = static_cast<entity_context*>(context);
	pyxcall_params_ret(pctxt, params_ret, out_err);
}
//--------------------------------------------------------------------
void pyxcall_no_params_ret(
        entity_context* pctxt,
        cdts params_ret[2],
        char** out_err)
{
	try
	{
		gil_guard guard;

		PyObject* res = nullptr;

		if(pctxt->is_callable())
		{
			// Use SDK Entity for callables
			auto* callable = dynamic_cast<CallableEntity*>(pctxt->entity.get());
			if(callable)
			{
				res = callable->call(std::vector<PyObject*>{});
			}
			else
			{
				handle_err(out_err, "Entity is not callable");
				return;
			}
		}
		else
		{
			// Attribute logic
			if(!pctxt->attribute_path.empty())
			{
				handle_err(out_err, "non-object attribute should have been loaded");
				return;
			}

			if(pctxt->foreign_object_type == attribute_action_getter)
			{
				res = PyObject_GetAttr(pctxt->attribute_holder, pctxt->entrypoint);
			}
			else
			{
				handle_err(out_err, "Unexpected attribute action");
				return;
			}
		}

		// Check error
		std::string err_msg = std::move(check_python_error());
		if(!err_msg.empty())
		{
			handle_err_str((char**)out_err, err_msg);
			return;
		}

		if(!res)
		{
			std::stringstream ss;
			ss << "Expected return type. No return type returned";
			handle_err_str((char**)out_err, ss.str());
			return;
		}

		// Convention: params_ret[0]=params (unused), params_ret[1]=retvals
		cdts_python3_serializer ret_ser(*g_runtime_manager, params_ret[1]);
		size_t retval_count = pctxt->retvals_types.size();

		if(retval_count == 1)
		{
			ret_ser.add(res, pctxt->retvals_types[0].type);
		}
		else if(retval_count > 1 && res && pPyTuple_Check(res))
		{
			for(size_t i = 0; i < retval_count; i++)
			{
				PyObject* item = pPyTuple_GetItem(res, i);
				ret_ser.add(item, pctxt->retvals_types[i].type);
			}
		}

	} catch(const std::exception& err)
	{
		handle_err_str(out_err, std::string(err.what()));
	}
}
//--------------------------------------------------------------------
void py_api_xcall_no_params_ret(void* context, cdts params_ret[2], char** out_err)
{
	entity_context* pctxt = static_cast<entity_context*>(context);
	pyxcall_no_params_ret(pctxt, params_ret, out_err);
}
//--------------------------------------------------------------------
void pyxcall_params_no_ret(
        entity_context* pctxt,
        cdts params_ret[2],
        char** out_err)
{
	try
	{
		gil_guard guard;

		// Convention: params_ret[0]=params, params_ret[1]=retvals (unused)
		cdts_python3_serializer params_ser(*g_runtime_manager, params_ret[0]);
		PyObject* params = params_ser.extract_as_tuple();

		if(pctxt->is_callable())
		{
			// Use SDK Entity for callables
			auto* callable = dynamic_cast<CallableEntity*>(pctxt->entity.get());
			if(callable)
			{
				callable->call(params);
			}
			else
			{
				Py_DECREF(params);
				handle_err(out_err, "Entity is not callable");
				return;
			}
		}
		else
		{
			// Attribute logic
			if(!pctxt->attribute_path.empty())
			{
				if(!pctxt->is_instance_required)
				{
					Py_DECREF(params);
					handle_err(out_err, "non-object attribute should have been found");
					return;
				}

				if(!pctxt->find_attribute_holder_and_attribute(pPyTuple_GetItem(params, 0)))
				{
					Py_DECREF(params);
					handle_err(out_err, "attribute not found");
					return;
				}
			}

			if(pctxt->foreign_object_type == attribute_action_setter)
			{
				PyObject* value = pctxt->is_instance_required ? pPyTuple_GetItem(params, 1) : pPyTuple_GetItem(params, 0);
				PyObject_SetAttr(pctxt->attribute_holder, pctxt->entrypoint, value);
			}
			else
			{
				Py_DECREF(params);
				handle_err(out_err, "Unexpected attribute action");
				return;
			}
		}

		std::string err_msg = std::move(check_python_error());
		if(!err_msg.empty())
		{
			Py_DECREF(params);
			handle_err_str((char**)out_err, err_msg);
			return;
		}

		Py_DECREF(params);

	} catch(const std::exception& err)
	{
		handle_err_str(out_err, std::string(err.what()));
	}
}
//--------------------------------------------------------------------
void py_api_xcall_params_no_ret(void* context, cdts params_ret[2], char** out_err)
{
	entity_context* pctxt = static_cast<entity_context*>(context);
	pyxcall_params_no_ret(pctxt, params_ret, out_err);
}
//--------------------------------------------------------------------
void pyxcall_no_params_no_ret(
        entity_context* pctxt,
        char** out_err)
{
	try
	{
		gil_guard guard;

		if(pctxt->is_callable())
		{
			// Fast path: call PyObject_CallObject directly, bypassing
			// CallableEntity::call() which would redundantly acquire GIL,
			// lock a mutex, create an empty tuple, and run organize_arguments.
			PyObject* py_callable = pctxt->entity->get_py_callable();
			if(!py_callable)
			{
				handle_err(out_err, "Entity callable is null");
				return;
			}

			PyObject* result = pPyObject_CallObject(py_callable, nullptr);
			if(result)
			{
				Py_DECREF(result);
			}
			else
			{
				std::string err_msg = std::move(check_python_error());
				if(!err_msg.empty())
				{
					handle_err_str((char**)out_err, err_msg);
				}
				else
				{
					handle_err(out_err, "Failed to call Python callable (void)");
				}
				return;
			}
		}
		else
		{
			handle_err(out_err, "Expecting callable object");
			return;
		}

		if(pPyErr_Occurred())
		{
			std::string err_msg = std::move(check_python_error());
			if(!err_msg.empty())
			{
				handle_err_str((char**)out_err, err_msg);
			}
		}
	}
	catch(const std::exception& err)
	{
		handle_err_str(out_err, std::string(err.what()));
	}
}
//--------------------------------------------------------------------
void py_api_xcall_no_params_no_ret(void* context, char** out_err)
{
	entity_context* pctxt = static_cast<entity_context*>(context);
	pyxcall_no_params_no_ret(pctxt, out_err);
}
//--------------------------------------------------------------------
xcall* make_callable(void* py_callable_as_py_object, metaffi_type_info* params_types, int8_t params_count, metaffi_type_info* retvals_types, int8_t retval_count, char** err)
{
	std::unique_ptr<entity_context> ctxt = std::make_unique<entity_context>();
	if(params_types)
	{
		ctxt->params_types.insert(ctxt->params_types.end(), params_types, params_types + params_count);
	}
	if(retvals_types)
	{
		ctxt->retvals_types.insert(ctxt->retvals_types.end(), retvals_types, retvals_types + retval_count);
	}

	// Create SDK PythonFunction entity from raw PyObject*
	PyObject* py_callable = static_cast<PyObject*>(py_callable_as_py_object);
	ctxt->entity = std::make_shared<PythonFunction>(
		py_callable,
		std::vector<PyObject*>{},  // Empty type vectors - not used for dispatch
		std::vector<PyObject*>{},
		false,  // varargs
		false   // named_args
	);

	void* xcall_func = params_count > 0 && retval_count > 0 ? (void*)py_api_xcall_params_ret
	                 : params_count == 0 && retval_count > 0 ? (void*)py_api_xcall_no_params_ret
	                 : params_count > 0 && retval_count == 0 ? (void*)py_api_xcall_params_no_ret
	                 : (void*)py_api_xcall_no_params_no_ret;

	xcall* pxcall = new xcall(xcall_func, ctxt.release());
	return pxcall;
}
//--------------------------------------------------------------------
xcall* load_entity(const char* module_path, const char* entity_path, metaffi_type_info* param_types, int8_t params_count, metaffi_type_info* ret_types, int8_t retval_count, char** err)
{
	// Ensure runtime is loaded
	if(!g_runtime_manager || !g_runtime_manager->is_runtime_loaded())
	{
		load_runtime(err);
		if(*err) return nullptr;
	}

	try
	{
		gil_guard guard;

		// Create context
		std::unique_ptr<entity_context> ctxt = std::make_unique<entity_context>();
		if(param_types)
		{
			ctxt->params_types.insert(ctxt->params_types.end(), param_types, param_types + params_count);
		}
		if(ret_types)
		{
			ctxt->retvals_types.insert(ctxt->retvals_types.end(), ret_types, ret_types + retval_count);
		}

		// Load module using SDK
		ctxt->module = std::make_shared<Module>(g_runtime_manager.get(), module_path);

		metaffi::utils::entity_path_parser fp(entity_path);

		if(fp.contains("callable"))
		{
			// Use SDK Module::load_entity() for callables
			ctxt->entity = ctxt->module->load_entity(
				entity_path,
				std::vector<PyObject*>{},  // Empty type vectors - not used for dispatch
				std::vector<PyObject*>{}
			);
		}
		else if(fp.contains("attribute"))
		{
			// Keep current attribute logic (as per user request)
			std::string load_symbol = fp["attribute"];
			ctxt->is_instance_required = fp.contains("instance_required");

			if(fp.contains("getter"))
			{
				ctxt->foreign_object_type = attribute_action_getter;
			}
			else if(fp.contains("setter"))
			{
				ctxt->foreign_object_type = attribute_action_setter;
			}
			else
			{
				handle_err(err, "missing getter or setter attribute in function path");
				return nullptr;
			}

			std::vector<std::string> path_to_object;
			boost::split(path_to_object, load_symbol, boost::is_any_of("."));
			ctxt->attribute_path = path_to_object;

			// For non-instance attributes, resolve immediately
			if(!ctxt->is_instance_required)
			{
				PyObject* pymod = import_module_by_path(module_path);
				if(!pymod)
				{
					handle_py_err(err);
					return nullptr;
				}

				if(!ctxt->find_attribute_holder_and_attribute(pymod))
				{
					handle_err(err, "attribute not found");
					return nullptr;
				}
			}
		}
		else
		{
			handle_err(err, "expecting \"callable\" or \"attribute\" in entity path");
			return nullptr;
		}

		// Select xcall function
		void* xcall_func = nullptr;
		if(params_count > 0 && retval_count > 0)
		{
			xcall_func = (void*)py_api_xcall_params_ret;
		}
		else if(params_count == 0 && retval_count > 0)
		{
			xcall_func = (void*)py_api_xcall_no_params_ret;
		}
		else if(params_count > 0 && retval_count == 0)
		{
			xcall_func = (void*)py_api_xcall_params_no_ret;
		}
		else
		{
			xcall_func = (void*)py_api_xcall_no_params_no_ret;
		}

		xcall* pxcall = new xcall(xcall_func, ctxt.release());
		return pxcall;
	}
	catch(const std::exception& e)
	{
		handle_err_str(err, std::string(e.what()));
		return nullptr;
	}
}
//--------------------------------------------------------------------
