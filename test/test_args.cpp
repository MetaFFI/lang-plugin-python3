#include <doctest/doctest.h>

#include "python3_test_env.h"
#include "python3_wrappers.h"

#include <variant>
#include <vector>

TEST_CASE("args and signatures")
{
	auto& env = python_test_env();

	auto positional_only = env.module_dir.load_entity(
		"callable=positional_only",
		{metaffi_int64_type, metaffi_int64_type, metaffi_int64_type},
		{metaffi_int64_type});
	auto [pos_sum] = positional_only.call<int64_t>(int64_t(1), int64_t(2), int64_t(3));
	CHECK(pos_sum == 6);

	auto positional_default = env.module_dir.load_entity(
		"callable=positional_only",
		{metaffi_int64_type, metaffi_int64_type},
		{metaffi_int64_type});
	auto [pos_sum_default] = positional_default.call<int64_t>(int64_t(1), int64_t(2));
	CHECK(pos_sum_default == 6);

	auto keyword_only = env.module_dir.load_entity(
		"callable=keyword_only,named_args",
		{metaffi_handle_type},
		{metaffi_string8_type});
	auto kw_dict = PyDict::create();
	kw_dict.set(std::string("named"), std::string("x"));
	auto [kw_val] = keyword_only.call<std::string>(*kw_dict.handle());
	CHECK(kw_val == "x");

	auto var_positional = env.module_dir.load_entity(
		"callable=var_positional,varargs",
		{metaffi_int64_packed_array_type},
		{metaffi_int64_packed_array_type});
	std::vector<int64_t> args = {1, 2};
	auto [var_args] = var_positional.call<std::vector<int64_t>>(args);
	CHECK(var_args == args);

	auto var_keyword = env.module_dir.load_entity(
		"callable=var_keyword,named_args",
		{metaffi_handle_type},
		{metaffi_handle_type});
	auto kw_only = PyDict::create();
	kw_only.set(std::string("a"), int64_t(1));
	kw_only.set(std::string("b"), int64_t(2));
	auto [kw_dict_ptr] = var_keyword.call<cdt_metaffi_handle*>(*kw_only.handle());
	PyDict kw_result{PyHandle{kw_dict_ptr}};
	auto kw_a = kw_result.get_any(std::string("a"));
	CHECK(std::holds_alternative<metaffi_int64>(kw_a));
	CHECK(std::get<metaffi_int64>(kw_a) == 1);

	auto var_pos_kw = env.module_dir.load_entity(
		"callable=var_positional_and_keyword,varargs,named_args",
		{metaffi_int64_packed_array_type, metaffi_handle_type},
		{metaffi_handle_type, metaffi_handle_type});
	auto kw2 = PyDict::create();
	kw2.set(std::string("x"), int64_t(9));
	auto [list_ptr, dict_ptr] = var_pos_kw.call<cdt_metaffi_handle*, cdt_metaffi_handle*>(args, *kw2.handle());
	PyList list{PyHandle{list_ptr}};
	PyDict dict{PyHandle{dict_ptr}};
	CHECK(list.size() == 2);
	auto dict_x = dict.get_any(std::string("x"));
	CHECK(std::holds_alternative<metaffi_int64>(dict_x));
	CHECK(std::get<metaffi_int64>(dict_x) == 9);

	auto mixed_args = env.module_dir.load_entity(
		"callable=mixed_args,varargs,named_args",
		{metaffi_int64_type, metaffi_int64_type, metaffi_int64_packed_array_type, metaffi_handle_type},
		{metaffi_int64_type, metaffi_int64_type, metaffi_handle_type, metaffi_int64_type, metaffi_handle_type});
	auto mixed_kwargs = PyDict::create();
	mixed_kwargs.set(std::string("c"), int64_t(4));
	mixed_kwargs.set(std::string("d"), int64_t(5));
	auto [a_val, b_val, args_ptr, c_val, kwargs_ptr] =
		mixed_args.call<int64_t, int64_t, cdt_metaffi_handle*, int64_t, cdt_metaffi_handle*>(
			int64_t(1), int64_t(2), std::vector<int64_t>{3}, *mixed_kwargs.handle());
	PyList args_list{PyHandle{args_ptr}};
	PyDict kwargs{PyHandle{kwargs_ptr}};
	CHECK(a_val == 1);
	CHECK(b_val == 2);
	CHECK(c_val == 4);
	CHECK(args_list.size() == 1);
	auto kw_d = kwargs.get_any(std::string("d"));
	CHECK(std::holds_alternative<metaffi_int64>(kw_d));
	CHECK(std::get<metaffi_int64>(kw_d) == 5);

	auto default_args = env.module_dir.load_entity(
		"callable=default_args",
		{},
		{metaffi_int64_type, metaffi_string8_type, metaffi_any_type});
	auto [def_a, def_b, def_c] = default_args.call<int64_t, std::string, metaffi_variant>();
	CHECK(def_a == 1);
	CHECK(def_b == "x");
	CHECK(std::holds_alternative<cdt_metaffi_handle>(def_c));
	CHECK(std::get<cdt_metaffi_handle>(def_c).handle == nullptr);

	auto overload = env.module_dir.load_entity(
		"callable=overload",
		{metaffi_any_type},
		{metaffi_any_type});
	auto [ov_int] = overload.call<metaffi_variant>(int64_t(1));
	CHECK(std::holds_alternative<metaffi_int64>(ov_int));
	CHECK(std::get<metaffi_int64>(ov_int) == 2);

	auto [ov_str] = overload.call<metaffi_variant>(std::string("abc"));
	CHECK(std::holds_alternative<metaffi_string8>(ov_str));
	CHECK(take_string8(std::get<metaffi_string8>(ov_str)) == "ABC");

	auto [ov_list] = overload.call<metaffi_variant>(std::vector<int64_t>{1, 2, 3});
	CHECK(std::holds_alternative<metaffi_int64>(ov_list));
	CHECK(std::get<metaffi_int64>(ov_list) == 3);

	auto star_only = env.module_dir.load_entity(
		"callable=accepts_star_only,named_args",
		{metaffi_int64_type, metaffi_handle_type},
		{metaffi_int64_type, metaffi_int64_type});
	auto star_kwargs = PyDict::create();
	star_kwargs.set(std::string("named"), int64_t(9));
	auto [star_val, star_named] = star_only.call<int64_t, int64_t>(int64_t(1), *star_kwargs.handle());
	CHECK(star_val == 1);
	CHECK(star_named == 9);

	auto accepts_kwargs = env.module_dir.load_entity(
		"callable=accepts_kwargs,named_args",
		{metaffi_string8_type, metaffi_handle_type},
		{metaffi_string8_type, metaffi_handle_type});
	auto kwargs_val = PyDict::create();
	kwargs_val.set(std::string("k"), int64_t(7));
	auto [val_out, kwargs_out_ptr] = accepts_kwargs.call<std::string, cdt_metaffi_handle*>(std::string("v"), *kwargs_val.handle());
	PyDict kwargs_out{PyHandle{kwargs_out_ptr}};
	CHECK(val_out == "v");
	auto kw_k = kwargs_out.get_any(std::string("k"));
	CHECK(std::holds_alternative<metaffi_int64>(kw_k));
	CHECK(std::get<metaffi_int64>(kw_k) == 7);
}
