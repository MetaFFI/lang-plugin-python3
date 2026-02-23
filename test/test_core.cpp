#include <doctest/doctest.h>

#include "python3_test_env.h"
#include "python3_wrappers.h"

#include <memory>
#include <variant>
#include <vector>

namespace
{
void add_callback_impl(void*, cdts* params_ret, char** out_err)
{
	if(out_err)
	{
		*out_err = nullptr;
	}

	if(!params_ret)
	{
		return;
	}

	cdts& params = params_ret[0];
	cdts& retvals = params_ret[1];

	const metaffi_int64 a = params.arr[0].cdt_val.int64_val;
	const metaffi_int64 b = params.arr[1].cdt_val.int64_val;
	retvals.arr[0] = static_cast<metaffi_int64>(a + b);
}
} // namespace

TEST_CASE("core functions")
{
	auto& env = python_test_env();

	auto hello = env.module_dir.load_entity("callable=hello_world", {}, {metaffi_string8_type});
	auto [msg] = hello.call<std::string>();
	CHECK(msg == "Hello World, from Python3");

	auto div = env.module_dir.load_entity(
		"callable=div_integers",
		{metaffi_int64_type, metaffi_int64_type},
		{metaffi_float64_type});
	auto [div_val] = div.call<double>(int64_t(10), int64_t(2));
	CHECK(div_val == doctest::Approx(5.0));

	auto join = env.module_dir.load_entity(
		"callable=join_strings",
		{metaffi_string8_packed_array_type},
		{metaffi_string8_type});
	std::vector<std::string> parts = {"a", "b", "c"};
	auto [joined] = join.call<std::string>(parts);
	CHECK(joined == "a,b,c");

	auto wait = env.module_dir.load_entity(
		"callable=wait_a_bit",
		{metaffi_int64_type},
		{});
	CHECK_NOTHROW(wait.call<>(int64_t(1)));

	auto ret_null = env.module_dir.load_entity("callable=return_null", {}, {metaffi_null_type});
	auto [null_val] = ret_null.call<metaffi_variant>();
	CHECK(std::holds_alternative<cdt_metaffi_handle>(null_val));
	CHECK(std::get<cdt_metaffi_handle>(null_val).handle == nullptr);
}

TEST_CASE("errors")
{
	auto& env = python_test_env();

	auto returns_error = env.module_dir.load_entity("callable=returns_an_error", {}, {});
	try
	{
		returns_error.call<>();
		FAIL("Expected exception");
	}
	catch(const std::exception& ex)
	{
		std::string msg = ex.what();
		CHECK(msg.size() >= 5);
		CHECK(msg.substr(msg.size() - 5) == "Error");
	}

	auto raise_custom = env.module_dir.load_entity(
		"callable=raise_custom_error",
		{metaffi_string8_type},
		{});
	try
	{
		raise_custom.call<>(std::string("boom"));
		FAIL("Expected exception");
	}
	catch(const std::exception& ex)
	{
		std::string msg = ex.what();
		CHECK(msg.size() >= 4);
		CHECK(msg.substr(msg.size() - 4) == "boom");
	}
}

TEST_CASE("callbacks")
{
	auto& env = python_test_env();

	auto ret_cb = env.module_dir.load_entity("callable=return_callback_add", {}, {metaffi_callable_type});
	auto [callable_ptr] = ret_cb.call<cdt_metaffi_callable*>();
	metaffi::api::MetaFFICallable callable_owner(callable_ptr, env.runtime.runtime_plugin());

	auto [sum_from_py] = callable_owner.call<int64_t>(int64_t(3), int64_t(4));
	CHECK(sum_from_py == 7);

	auto call_cb = env.module_dir.load_entity(
		"callable=call_callback_add",
		{metaffi_callable_type},
		{metaffi_int64_type});

	metaffi_type params_types[] = {metaffi_int64_type, metaffi_int64_type};
	metaffi_type retvals_types[] = {metaffi_int64_type};
	xcall add_xcall(reinterpret_cast<void*>(add_callback_impl), nullptr);
	cdt_metaffi_callable add_callable;
	add_callable.val = &add_xcall;
	add_callable.parameters_types = params_types;
	add_callable.params_types_length = static_cast<metaffi_int8>(sizeof(params_types) / sizeof(params_types[0]));
	add_callable.retval_types = retvals_types;
	add_callable.retval_types_length = static_cast<metaffi_int8>(sizeof(retvals_types) / sizeof(retvals_types[0]));

	auto [sum_from_cpp] = call_cb.call<int64_t>(add_callable);
	CHECK(sum_from_cpp == 3);
}

TEST_CASE("callback error")
{
	auto& env = python_test_env();

	auto module_dict_getter = env.module_dir.load_entity(
		"attribute=__dict__,getter",
		{},
		{metaffi_handle_type});
	auto [module_dict_ptr] = module_dict_getter.call<cdt_metaffi_handle*>();
	PyHandle module_dict_owner(module_dict_ptr);

	auto eval_entity = env.builtins.load_entity(
		"callable=eval",
		{metaffi_string8_type, metaffi_handle_type, metaffi_handle_type},
		{metaffi_callable_type});
	auto [raise_cb_ptr] = eval_entity.call<cdt_metaffi_callable*>(
		std::string("lambda: (_ for _ in ()).throw(Exception('callback error'))"),
		*module_dict_owner.get(),
		*module_dict_owner.get());
	metaffi::api::MetaFFICallable raise_cb_owner(raise_cb_ptr, env.runtime.runtime_plugin());

	auto cb_raise = env.module_dir.load_entity(
		"callable=callback_raises_error",
		{metaffi_callable_type},
		{metaffi_any_type});
	auto [cb_msg] = cb_raise.call<metaffi_variant>(*raise_cb_owner.get());
	CHECK(std::holds_alternative<metaffi_string8>(cb_msg));
	CHECK(take_string8(std::get<metaffi_string8>(cb_msg)).find("callback error") != std::string::npos);
}

TEST_CASE("error tuple")
{
	auto& env = python_test_env();

	auto ret_err_tuple = env.module_dir.load_entity(
		"callable=return_error_tuple",
		{metaffi_bool_type},
		{metaffi_bool_type, metaffi_any_type});
	auto [ok_flag, ok_err] = ret_err_tuple.call<bool, metaffi_variant>(true);
	CHECK(ok_flag);
	CHECK(std::holds_alternative<cdt_metaffi_handle>(ok_err));
	CHECK(std::get<cdt_metaffi_handle>(ok_err).handle == nullptr);

	auto [err_flag, err_val] = ret_err_tuple.call<bool, metaffi_variant>(false);
	CHECK(err_flag == false);
	CHECK(std::holds_alternative<metaffi_string8>(err_val));
	CHECK(take_string8(std::get<metaffi_string8>(err_val)) == "error");
}

TEST_CASE("multiple return values and any")
{
	auto& env = python_test_env();

	auto ret_multi = env.module_dir.load_entity(
		"callable=return_multiple_return_values",
		{},
		{metaffi_int64_type, metaffi_string8_type, metaffi_float64_type, metaffi_null_type,
		 metaffi_uint8_array_type, metaffi_handle_type});

	auto [num, text, value, null_val, bytes, obj_ptr] =
		ret_multi.call<int64_t, std::string, double, metaffi_variant, std::vector<uint8_t>, cdt_metaffi_handle*>();
	PyHandle obj_handle(obj_ptr);

	CHECK(num == 1);
	CHECK(text == "string");
	CHECK(value == doctest::Approx(3.0));
	CHECK(bytes == std::vector<uint8_t>({1, 2, 3}));
	CHECK(std::holds_alternative<cdt_metaffi_handle>(null_val));
	CHECK(std::get<cdt_metaffi_handle>(null_val).handle == nullptr);

	auto obj_str = env.module_dir.load_entity(
		"callable=SomeClass.__str__,instance_required",
		{metaffi_handle_type},
		{metaffi_string8_type});
	auto [obj_text] = obj_str.call<std::string>(*obj_handle.get());
	CHECK(obj_text.find("SomeClass") != std::string::npos);

	auto ret_any_any = env.module_dir.load_entity(
		"callable=return_any",
		{metaffi_int64_type},
		{metaffi_any_type});
	{
		auto [val] = ret_any_any.call<metaffi_variant>(int64_t(0));
		CHECK(std::holds_alternative<metaffi_int64>(val));
		CHECK(std::get<metaffi_int64>(val) == 1);
	}
	{
		auto [val] = ret_any_any.call<metaffi_variant>(int64_t(1));
		CHECK(std::holds_alternative<metaffi_string8>(val));
		CHECK(take_string8(std::get<metaffi_string8>(val)) == "string");
	}
	{
		auto [val] = ret_any_any.call<metaffi_variant>(int64_t(2));
		CHECK(std::holds_alternative<metaffi_float64>(val));
		CHECK(std::get<metaffi_float64>(val) == doctest::Approx(3.0));
	}
	{
		auto [val] = ret_any_any.call<metaffi_variant>(int64_t(999));
		CHECK(std::holds_alternative<cdt_metaffi_handle>(val));
		CHECK(std::get<cdt_metaffi_handle>(val).handle == nullptr);
	}

	auto ret_any_list = env.module_dir.load_entity(
		"callable=return_any",
		{metaffi_int64_type},
		{metaffi_string8_array_type});
	auto [list_vals] = ret_any_list.call<std::vector<std::string>>(int64_t(3));
	CHECK(list_vals == std::vector<std::string>({"list", "of", "strings"}));

	auto ret_any_handle = env.module_dir.load_entity(
		"callable=return_any",
		{metaffi_int64_type},
		{metaffi_handle_type});
	auto [obj_any_ptr] = ret_any_handle.call<cdt_metaffi_handle*>(int64_t(4));
	PyHandle obj_any(obj_any_ptr);
	auto [obj_any_text] = obj_str.call<std::string>(*obj_any.get());
	CHECK(obj_any_text.find("SomeClass") != std::string::npos);
}

TEST_CASE("accepts any and buffers")
{
	auto& env = python_test_env();

	auto accepts_any = env.module_dir.load_entity(
		"callable=accepts_any",
		{metaffi_int64_type, metaffi_any_type},
		{});
	CHECK_NOTHROW(accepts_any.call<>(int64_t(0), int64_t(1)));
	CHECK_NOTHROW(accepts_any.call<>(int64_t(1), std::string("string")));
	CHECK_NOTHROW(accepts_any.call<>(int64_t(2), double(3.0)));
	CHECK_NOTHROW(accepts_any.call<>(int64_t(3), nullptr));

	std::vector<uint8_t> bytes = {1, 2};
	CHECK_NOTHROW(accepts_any.call<>(int64_t(4), bytes));

	auto class_get = env.module_dir.load_entity("attribute=SomeClass,getter", {}, {metaffi_handle_type});
	auto [class_ptr] = class_get.call<cdt_metaffi_handle*>();
	PyHandle class_handle(class_ptr);

	auto new_entity = env.module_dir.load_entity(
		"callable=SomeClass.__new__",
		{metaffi_handle_type},
		{metaffi_handle_type});
	auto [instance_ptr] = new_entity.call<cdt_metaffi_handle*>(*class_handle.get());
	PyHandle instance_handle(instance_ptr);

	auto init_entity = env.module_dir.load_entity(
		"callable=SomeClass.__init__,instance_required",
		{metaffi_handle_type, metaffi_string8_type},
		{});
	CHECK_NOTHROW(init_entity.call<>(*instance_handle.get(), std::string("any")));
	CHECK_NOTHROW(accepts_any.call<>(int64_t(5), *instance_handle.get()));

	auto get_buffers = env.module_dir.load_entity(
		"callable=get_three_buffers",
		{},
		{metaffi_array_type});
	auto [buffers] = get_buffers.call<std::vector<std::vector<uint8_t>>>();
	CHECK(buffers.size() == 3);
	CHECK(buffers[0] == std::vector<uint8_t>({1, 2, 3, 4}));
	CHECK(buffers[1] == std::vector<uint8_t>({5, 6, 7}));
	CHECK(buffers[2] == std::vector<uint8_t>({8, 9}));

	auto expect_buffers = env.module_dir.load_entity(
		"callable=expect_three_buffers",
		{metaffi_array_type},
		{});
	CHECK_NOTHROW(expect_buffers.call<>(buffers));
}

TEST_CASE("handle roundtrip preserves runtime id")
{
	auto& env = python_test_env();

	auto dict = PyDict::create();
	auto vec = std::make_unique<std::vector<int64_t>>(std::initializer_list<int64_t>{1, 2, 3});

	dict.set(std::string("vector"), vec.get());

	auto get_item = env.builtins.load_entity(
		"callable=dict.__getitem__,instance_required",
		{metaffi_handle_type, metaffi_any_type},
		{metaffi_handle_type});
	auto [native_ptr] = get_item.call<std::vector<int64_t>*>(*dict.handle(), std::string("vector"));

	CHECK(native_ptr == vec.get());
	CHECK((*native_ptr).size() == 3);
	CHECK((*native_ptr)[0] == 1);
	CHECK((*native_ptr)[1] == 2);
	CHECK((*native_ptr)[2] == 3);
}

TEST_CASE("mixed arrays as handles")
{
	auto& env = python_test_env();

	auto diff_objects = env.module_dir.load_entity(
		"callable=returns_array_of_different_objects",
		{},
		{metaffi_handle_type});
	auto [list_ptr] = diff_objects.call<cdt_metaffi_handle*>();
	PyList list{PyHandle{list_ptr}};
	CHECK(list.size() == 6);

	auto v0 = list.get_any(0);
	CHECK(std::holds_alternative<metaffi_int64>(v0));
	CHECK(std::get<metaffi_int64>(v0) == 1);

	auto v1 = list.get_any(1);
	CHECK(std::holds_alternative<metaffi_string8>(v1));
	CHECK(take_string8(std::get<metaffi_string8>(v1)) == "string");

	auto v2 = list.get_any(2);
	CHECK(std::holds_alternative<metaffi_float64>(v2));
	CHECK(std::get<metaffi_float64>(v2) == doctest::Approx(3.0));

	auto v3 = list.get_any(3);
	CHECK(std::holds_alternative<cdt_metaffi_handle>(v3));
	CHECK(std::get<cdt_metaffi_handle>(v3).handle == nullptr);

	auto bytes_handle = list.get_handle(4);
	auto bytes_entity = env.builtins.load_entity(
		"callable=bytes",
		{metaffi_handle_type},
		{metaffi_uint8_array_type});
	auto [bytes] = bytes_entity.call<std::vector<uint8_t>>(*bytes_handle.get());
	CHECK(bytes == std::vector<uint8_t>({1, 2, 3}));

	auto obj_handle = list.get_handle(5);
	auto obj_str = env.module_dir.load_entity(
		"callable=SomeClass.__str__,instance_required",
		{metaffi_handle_type},
		{metaffi_string8_type});
	auto [obj_text] = obj_str.call<std::string>(*obj_handle.get());
	CHECK(obj_text.find("SomeClass") != std::string::npos);

	auto diff_dims = env.module_dir.load_entity(
		"callable=returns_array_with_different_dimensions",
		{},
		{metaffi_handle_type});
	auto [dim_ptr] = diff_dims.call<cdt_metaffi_handle*>();
	PyList dim_list{PyHandle{dim_ptr}};
	CHECK(dim_list.size() == 3);

	auto first_list = dim_list.get_handle(0);
	auto second_val = dim_list.get_any(1);
	auto third_list = dim_list.get_handle(2);
	CHECK(std::holds_alternative<metaffi_int64>(second_val));
	CHECK(std::get<metaffi_int64>(second_val) == 4);

	auto first_pylist = PyList(PyHandle(first_list.release()));
	auto third_pylist = PyList(PyHandle(third_list.release()));
	CHECK(first_pylist.size() == 3);
	CHECK(third_pylist.size() == 2);
}
