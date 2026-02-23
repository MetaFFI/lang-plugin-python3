#include "call_xcall.h"
#include "host_cdts_converter.h"
#include "runtime_globals.h"
#include <runtime_manager/cpython3/gil_guard.h>
#include <runtime_manager/cpython3/python_api_wrapper.h>
#include <runtime/xllr_capi_loader.h>
#include <utils/logger.hpp>
#include <utils/scope_guard.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <runtime_manager/cpython3/py_utils.h>

static spdlog::logger* LOG = metaffi::get_logger("python3.runtime");

namespace
{
	struct param_types_cache
	{
		PyObject* key = nullptr;
		Py_ssize_t count = -1;
		std::vector<metaffi_type_info> infos;
		std::vector<std::string> aliases;
	};

	thread_local param_types_cache g_param_types_cache;

	struct xcall_profile_state
	{
		bool enabled = false;
		uint64_t calls = 0;
		uint64_t parse_ns = 0;
		uint64_t convert_ns = 0;
		uint64_t invoke_ns = 0;
		uint64_t returns_ns = 0;
		uint64_t total_ns = 0;
	};

	xcall_profile_state& get_profile_state()
	{
		static xcall_profile_state state = []()
		{
			xcall_profile_state s{};
			const char* raw = std::getenv("METAFFI_PROFILE_PY_XCALL");
			if(raw)
			{
				std::string v(raw);
				for(char& c : v)
				{
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				}
				s.enabled = (v == "1" || v == "true" || v == "yes" || v == "on");
			}
			return s;
		}();
		return state;
	}

	uint64_t now_ns()
	{
		return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
	}

	void profile_commit(uint64_t parse_ns, uint64_t convert_ns, uint64_t invoke_ns, uint64_t returns_ns, uint64_t total_ns)
	{
		auto& s = get_profile_state();
		if(!s.enabled)
		{
			return;
		}

		s.calls += 1;
		s.parse_ns += parse_ns;
		s.convert_ns += convert_ns;
		s.invoke_ns += invoke_ns;
		s.returns_ns += returns_ns;
		s.total_ns += total_ns;

		if((s.calls % 5000) == 0)
		{
			const double inv = 1.0 / static_cast<double>(s.calls);
			std::fprintf(stderr,
				"[py-xcall-profiler] calls=%llu mean_ns parse=%.1f convert=%.1f invoke=%.1f returns=%.1f total=%.1f\n",
				static_cast<unsigned long long>(s.calls),
				static_cast<double>(s.parse_ns) * inv,
				static_cast<double>(s.convert_ns) * inv,
				static_cast<double>(s.invoke_ns) * inv,
				static_cast<double>(s.returns_ns) * inv,
				static_cast<double>(s.total_ns) * inv);
			std::fflush(stderr);
		}
	}

	std::vector<metaffi_type_info>* get_or_parse_param_types(PyObject* param_metaffi_types, Py_ssize_t params_count)
	{
		if(g_param_types_cache.key == param_metaffi_types && g_param_types_cache.count == params_count)
		{
			return &g_param_types_cache.infos;
		}

		Py_XDECREF(g_param_types_cache.key);
		g_param_types_cache.key = nullptr;
		g_param_types_cache.count = -1;
		g_param_types_cache.infos.clear();
		g_param_types_cache.aliases.clear();
		g_param_types_cache.infos.reserve(static_cast<size_t>(params_count));
		g_param_types_cache.aliases.reserve(static_cast<size_t>(params_count));

		for(Py_ssize_t i = 0; i < params_count; i++)
		{
			PyObject* item = pPyTuple_GetItem(param_metaffi_types, i);
			if(!item)
			{
				pPyErr_SetString(pPyExc_RuntimeError, "failed to read parameter type item");
				return nullptr;
			}

			metaffi_type_info info{};
			info.alias = nullptr;
			info.is_free_alias = 0;
			info.fixed_dimensions = 0;

			// metaffi_type_info object path
			if(pPyObject_HasAttrString(item, "type") && pPyObject_HasAttrString(item, "fixed_dimensions"))
			{
				PyObject* type_obj = pPyObject_GetAttrString(item, "type");
				PyObject* alias_obj = pPyObject_GetAttrString(item, "alias");
				PyObject* dimensions_obj = pPyObject_GetAttrString(item, "fixed_dimensions");

				if(!type_obj || !alias_obj || !dimensions_obj || pPyErr_Occurred())
				{
					Py_XDECREF(type_obj);
					Py_XDECREF(alias_obj);
					Py_XDECREF(dimensions_obj);
					return nullptr;
				}

				info.type = static_cast<metaffi_type>(pPyLong_AsUnsignedLongLong(type_obj));
				info.fixed_dimensions = pPyLong_AsLong(dimensions_obj);
				if(pPyErr_Occurred())
				{
					Py_DECREF(type_obj);
					Py_DECREF(alias_obj);
					Py_DECREF(dimensions_obj);
					return nullptr;
				}

				if(!Py_IsNone(alias_obj))
				{
					const char* alias_utf8 = pPyBytes_AsString(alias_obj);
					if(pPyErr_Occurred())
					{
						Py_DECREF(type_obj);
						Py_DECREF(alias_obj);
						Py_DECREF(dimensions_obj);
						return nullptr;
					}
					g_param_types_cache.aliases.emplace_back(alias_utf8 ? alias_utf8 : "");
					info.alias = const_cast<char*>(g_param_types_cache.aliases.back().c_str());
				}

				Py_DECREF(type_obj);
				Py_DECREF(alias_obj);
				Py_DECREF(dimensions_obj);
			}
			// integer type shortcut path
			else if(pPyLong_Check(item))
			{
				info.type = static_cast<metaffi_type>(pPyLong_AsUnsignedLongLong(item));
				if(pPyErr_Occurred())
				{
					return nullptr;
				}
			}
			else
			{
				pPyErr_SetString(pPyExc_RuntimeError, "expected metaffi_type_info or integer");
				return nullptr;
			}

			g_param_types_cache.infos.emplace_back(info);
		}

		Py_INCREF(param_metaffi_types);
		g_param_types_cache.key = param_metaffi_types;
		g_param_types_cache.count = params_count;
		return &g_param_types_cache.infos;
	}
}

PyObject* call_xcall(void* pxcall_ptr, void* context, PyObject* param_metaffi_types, PyObject* retval_metaffi_types, PyObject* args)
{
	const bool do_profile = get_profile_state().enabled;
	const uint64_t call_start_ns = do_profile ? now_ns() : 0;
	uint64_t parse_ns = 0;
	uint64_t convert_ns = 0;
	uint64_t invoke_ns = 0;
	uint64_t returns_ns = 0;

	METAFFI_DEBUG(LOG, "call_xcall: pxcall_ptr={}, context={}", pxcall_ptr, context);
	
	if(pxcall_ptr == nullptr)
	{
		METAFFI_DEBUG(LOG, "call_xcall: pxcall_ptr is NULL!");
		pPyErr_SetString(pPyExc_RuntimeError, "xcall is null");
		Py_INCREF(pPy_None);
		return pPy_None;
	}
	
	// Acquire GIL using SDK's gil_guard
	gil_guard guard;
	
	// Ensure runtime manager is initialized
	get_runtime_manager();

	xcall pxcall(pxcall_ptr, context);
	
	METAFFI_DEBUG(LOG, "call_xcall: xcall created, pxcall_and_context[0]={}, pxcall_and_context[1]={}",
		pxcall.pxcall_and_context[0], pxcall.pxcall_and_context[1]);

	Py_ssize_t retval_count = pPyTuple_Size(retval_metaffi_types);
	
	Py_ssize_t params_count = pPyTuple_Size(param_metaffi_types);
	if(pPyErr_Occurred())
	{
		Py_INCREF(pPy_None);
		return pPy_None;
	}

	if (params_count > 0 || retval_count > 0)
	{
		const uint64_t parse_start_ns = do_profile ? now_ns() : 0;
		std::vector<metaffi_type_info>* param_metaffi_types_vec = get_or_parse_param_types(param_metaffi_types, params_count);
		if(do_profile)
		{
			parse_ns = now_ns() - parse_start_ns;
		}
		if(!param_metaffi_types_vec)
		{
			Py_INCREF(pPy_None);
			return pPy_None;
		}

		const uint64_t convert_start_ns = do_profile ? now_ns() : 0;
		cdts* pcdts = convert_host_params_to_cdts(args, param_metaffi_types_vec->data(), params_count, retval_count);
		if(do_profile)
		{
			convert_ns = now_ns() - convert_start_ns;
		}
		if(pcdts == nullptr)
		{
			Py_INCREF(pPy_None);
			return pPy_None;
		}
		metaffi::utils::scope_guard free_cdts_guard([&](){ xllr_free_cdts_buffer(pcdts); });

		char* out_err = nullptr;
		// Convention: caller always passes cdts[2] with [0]=params, [1]=retvals. Callee accepts both.
		const uint64_t invoke_start_ns = do_profile ? now_ns() : 0;
		pxcall(pcdts, &out_err);
		if(do_profile)
		{
			invoke_ns = now_ns() - invoke_start_ns;
		}

		if(out_err)
		{
			pPyErr_SetString(pPyExc_RuntimeError, out_err);
			xllr_free_string(out_err);
			Py_INCREF(pPy_None);
			return pPy_None;
		}

		if(retval_count == 0) {
			if(do_profile)
			{
				profile_commit(parse_ns, convert_ns, invoke_ns, 0, now_ns() - call_start_ns);
			}
			Py_INCREF(pPy_None);
			return pPy_None;
		}

		const uint64_t returns_start_ns = do_profile ? now_ns() : 0;
		PyObject* converted = convert_host_return_values_from_cdts(pcdts, 1);
		if(do_profile)
		{
			returns_ns = now_ns() - returns_start_ns;
		}
		if(converted == nullptr)
		{
			Py_INCREF(pPy_None);
			return pPy_None;
		}

		if(do_profile)
		{
			profile_commit(parse_ns, convert_ns, invoke_ns, returns_ns, now_ns() - call_start_ns);
		}
		return converted;
	}
	else
	{
		char* out_err = nullptr;
		const uint64_t invoke_start_ns = do_profile ? now_ns() : 0;
		pxcall(&out_err);
		if(do_profile)
		{
			invoke_ns = now_ns() - invoke_start_ns;
		}
		if (out_err)
		{
			// Set the error - PyDLL will automatically check and raise it
			if(pPyExc_RuntimeError != nullptr)
			{
				pPyErr_SetString(pPyExc_RuntimeError, out_err);
			}
			xllr_free_string(out_err);
			Py_INCREF(pPy_None);
			return pPy_None;
		}

		if(do_profile)
		{
			profile_commit(parse_ns, 0, invoke_ns, 0, now_ns() - call_start_ns);
		}
		Py_INCREF(pPy_None);
		return pPy_None;
	}
}
