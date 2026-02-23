#include <doctest/doctest.h>

#include "python3_test_env.h"
#include "python3_wrappers.h"

#include <variant>
#include <vector>

TEST_CASE("arrays")
{
	auto& env = python_test_env();

	auto make_1d = env.module_dir.load_entity("callable=make_1d_array", {}, {metaffi_int64_array_type});
	auto [arr1] = make_1d.call<std::vector<int64_t>>();
	CHECK(arr1 == std::vector<int64_t>({1, 2, 3}));

	auto make_2d = env.module_dir.load_entity("callable=make_2d_array", {}, {metaffi_int64_array_type});
	auto [arr2] = make_2d.call<std::vector<std::vector<int64_t>>>();
	CHECK(arr2 == std::vector<std::vector<int64_t>>{{1, 2}, {3, 4}});

	auto make_3d = env.module_dir.load_entity("callable=make_3d_array", {}, {metaffi_int64_array_type});
	auto [arr3] = make_3d.call<std::vector<std::vector<std::vector<int64_t>>>>();
	CHECK(arr3 == std::vector<std::vector<std::vector<int64_t>>>{{{1}, {2}}, {{3}, {4}}});

	auto make_ragged = env.module_dir.load_entity("callable=make_ragged_array", {}, {metaffi_int64_array_type});
	auto [ragged] = make_ragged.call<std::vector<std::vector<int64_t>>>();
	CHECK(ragged == std::vector<std::vector<int64_t>>{{1, 2, 3}, {4}, {5, 6}});

	auto accept_3d = env.module_dir.load_entity(
		"callable=accepts_3d_array",
		{metaffi_int64_array_type},
		{metaffi_int64_type});
	auto [sum3d] = accept_3d.call<int64_t>(arr3);
	CHECK(sum3d == 10);

	auto accept_ragged = env.module_dir.load_entity(
		"callable=accepts_ragged_array",
		{metaffi_int64_array_type},
		{metaffi_int64_type});
	auto [sum_ragged] = accept_ragged.call<int64_t>(ragged);
	CHECK(sum_ragged == 21);
}

TEST_CASE("packed array: sum 1d int array")
{
	auto& env = python_test_env();

	auto sum1d = env.module_dir.load_entity(
		"callable=sum_1d_int_array",
		{metaffi_int64_packed_array_type},
		{metaffi_int64_type});
	auto [sum_val] = sum1d.call<int64_t>(std::vector<int64_t>({1, 2, 3, 4, 5}));
	CHECK(sum_val == 15);
}

TEST_CASE("packed array: echo 1d int array")
{
	auto& env = python_test_env();

	auto echo = env.module_dir.load_entity(
		"callable=echo_1d_int_array",
		{metaffi_int64_packed_array_type},
		{metaffi_int64_packed_array_type});
	auto [result] = echo.call<std::vector<int64_t>>(std::vector<int64_t>({100, 200, 300}));
	CHECK(result == std::vector<int64_t>({100, 200, 300}));
}

TEST_CASE("packed array: echo 1d float array")
{
	auto& env = python_test_env();

	auto echo = env.module_dir.load_entity(
		"callable=echo_1d_float_array",
		{metaffi_float64_packed_array_type},
		{metaffi_float64_packed_array_type});
	auto [result] = echo.call<std::vector<double>>(std::vector<double>({1.5, 2.5, 3.5}));
	REQUIRE(result.size() == 3);
	CHECK(result[0] == doctest::Approx(1.5));
	CHECK(result[1] == doctest::Approx(2.5));
	CHECK(result[2] == doctest::Approx(3.5));
}

TEST_CASE("packed array: make 1d int array")
{
	auto& env = python_test_env();

	auto make1d = env.module_dir.load_entity(
		"callable=make_1d_int_array",
		{},
		{metaffi_int64_packed_array_type});
	auto [arr] = make1d.call<std::vector<int64_t>>();
	CHECK(arr == std::vector<int64_t>({10, 20, 30, 40, 50}));
}

TEST_CASE("bytes and memory")
{
	auto& env = python_test_env();

	auto bytes_buf = env.module_dir.load_entity("callable=returns_bytes_buffer", {}, {metaffi_uint8_packed_array_type});
	auto [bytes_val] = bytes_buf.call<std::vector<uint8_t>>();
	CHECK(bytes_val == std::vector<uint8_t>({1, 2, 3}));

	auto bytearray_ret = env.module_dir.load_entity("callable=returns_bytearray", {}, {metaffi_handle_type});
	auto [bytearray_ptr] = bytearray_ret.call<cdt_metaffi_handle*>();
	PyHandle bytearray_handle(bytearray_ptr);

	auto bytes_entity = env.builtins.load_entity(
		"callable=bytes",
		{metaffi_handle_type},
		{metaffi_uint8_packed_array_type});
	auto [bytearray_bytes] = bytes_entity.call<std::vector<uint8_t>>(*bytearray_handle.get());
	CHECK(bytearray_bytes == std::vector<uint8_t>({4, 5, 6}));

	auto memview_ret = env.module_dir.load_entity("callable=returns_memoryview", {}, {metaffi_handle_type});
	auto [memview_ptr] = memview_ret.call<cdt_metaffi_handle*>();
	PyHandle memview_handle(memview_ptr);
	auto [memview_bytes] = bytes_entity.call<std::vector<uint8_t>>(*memview_handle.get());
	CHECK(memview_bytes == std::vector<uint8_t>({7, 8, 9}));
}

TEST_CASE("nested dict and list of objects")
{
	auto& env = python_test_env();

	auto nested_dict_ret = env.module_dir.load_entity("callable=returns_nested_dict", {}, {metaffi_handle_type});
	auto [dict_ptr] = nested_dict_ret.call<cdt_metaffi_handle*>();
	PyDict dict{PyHandle{dict_ptr}};

	auto inner_a = PyDict(dict.get_handle(std::string("a")));
	auto list_b = PyList(inner_a.get_handle(std::string("b")));
	auto b1 = list_b.get_any(1);
	CHECK(std::holds_alternative<metaffi_int64>(b1));
	CHECK(std::get<metaffi_int64>(b1) == 2);

	auto inner_c = PyDict(inner_a.get_handle(std::string("c")));
	auto d_val = inner_c.get_any(std::string("d"));
	CHECK(std::holds_alternative<metaffi_string8>(d_val));
	CHECK(take_string8(std::get<metaffi_string8>(d_val)) == "e");

	auto list_objects = env.module_dir.load_entity("callable=returns_list_of_objects", {}, {metaffi_handle_type});
	auto [list_ptr] = list_objects.call<cdt_metaffi_handle*>();
	PyList obj_list{PyHandle{list_ptr}};
	CHECK(obj_list.size() == 3);

	auto name_get = env.module_dir.load_entity(
		"callable=SomeClass.__getattribute__,instance_required",
		{metaffi_handle_type, metaffi_string8_type},
		{metaffi_string8_type});

	auto obj0 = obj_list.get_handle(0);
	auto [name0] = name_get.call<std::string>(*obj0.get(), std::string("name"));
	CHECK(name0 == "a");

	auto obj1 = obj_list.get_handle(1);
	auto [name1] = name_get.call<std::string>(*obj1.get(), std::string("name"));
	CHECK(name1 == "b");

	auto obj2 = obj_list.get_handle(2);
	auto [name2] = name_get.call<std::string>(*obj2.get(), std::string("name"));
	CHECK(name2 == "c");
}

TEST_CASE("optional and collections")
{
	auto& env = python_test_env();

	auto returns_opt = env.module_dir.load_entity(
		"callable=returns_optional",
		{metaffi_bool_type},
		{metaffi_any_type});
	auto [opt_true] = returns_opt.call<metaffi_variant>(true);
	CHECK(std::holds_alternative<metaffi_int64>(opt_true));
	CHECK(std::get<metaffi_int64>(opt_true) == 123);

	auto [opt_false] = returns_opt.call<metaffi_variant>(false);
	CHECK(std::holds_alternative<cdt_metaffi_handle>(opt_false));
	CHECK(std::get<cdt_metaffi_handle>(opt_false).handle == nullptr);

	auto accepts_primitives = env.module_dir.load_entity(
		"callable=accepts_primitives",
		{metaffi_bool_type, metaffi_int64_type, metaffi_float64_type, metaffi_handle_type,
		 metaffi_int64_type, metaffi_string8_type, metaffi_uint8_packed_array_type, metaffi_handle_type},
		{metaffi_bool_type, metaffi_int64_type, metaffi_float64_type, metaffi_handle_type,
		 metaffi_int64_type, metaffi_string8_type, metaffi_uint8_packed_array_type, metaffi_handle_type});

	auto complex_ctor = env.builtins.load_entity(
		"callable=complex",
		{metaffi_float64_type, metaffi_float64_type},
		{metaffi_handle_type});
	auto [complex_ptr] = complex_ctor.call<cdt_metaffi_handle*>(3.0, 4.0);
	PyHandle complex_handle(complex_ptr);

	std::vector<uint8_t> bytes_val = {'x'};
	auto bytearray_ctor = env.builtins.load_entity(
		"callable=bytearray",
		{metaffi_uint8_packed_array_type},
		{metaffi_handle_type});
	auto [bytearray_ptr] = bytearray_ctor.call<cdt_metaffi_handle*>(bytes_val);
	PyHandle bytearray_handle(bytearray_ptr);

	auto [b, i, f, complex_out, byte_val, text, bytes_out, bytearray_out] =
		accepts_primitives.call<bool, int64_t, double, cdt_metaffi_handle*, int64_t, std::string,
		                        std::vector<uint8_t>, cdt_metaffi_handle*>(
			true, int64_t(1), 2.0, *complex_handle.get(), int64_t(255), std::string("s"), bytes_val, *bytearray_handle.get());

	PyHandle complex_ret(complex_out);
	PyHandle bytearray_ret(bytearray_out);

	CHECK(b);
	CHECK(i == 1);
	CHECK(f == doctest::Approx(2.0));
	CHECK(byte_val == 255);
	CHECK(text == "s");
	CHECK(bytes_out == bytes_val);
	CHECK(!complex_ret.is_null());
	CHECK(!bytearray_ret.is_null());

	auto accepts_collections = env.module_dir.load_entity(
		"callable=accepts_collections",
		{metaffi_handle_type, metaffi_handle_type, metaffi_handle_type, metaffi_handle_type, metaffi_handle_type},
		{metaffi_handle_type, metaffi_handle_type, metaffi_handle_type, metaffi_handle_type, metaffi_handle_type});

	auto list = PyList::create();
	list.append(int64_t(1));
	auto tuple = PyTuple::from_ints({2});
	auto set = PySet::create();
	set.add(int64_t(3));
	auto frozenset = PyFrozenSet::from_ints({4});
	auto dict = PyDict::create();
	dict.set(std::string("k"), int64_t(5));

	auto [list_ptr, tuple_ptr, set_ptr, frozenset_ptr, dict_ptr] =
		accepts_collections.call<cdt_metaffi_handle*, cdt_metaffi_handle*, cdt_metaffi_handle*, cdt_metaffi_handle*, cdt_metaffi_handle*>(
			*list.handle(), *tuple.handle(), *set.handle(), *frozenset.handle(), *dict.handle());

	PyHandle list_ret(list_ptr);
	PyHandle tuple_ret(tuple_ptr);
	PyHandle set_ret(set_ptr);
	PyHandle frozenset_ret(frozenset_ptr);
	PyHandle dict_ret(dict_ptr);

	CHECK(!list_ret.is_null());
	CHECK(!tuple_ret.is_null());
	CHECK(!set_ret.is_null());
	CHECK(!frozenset_ret.is_null());
	CHECK(!dict_ret.is_null());
}
