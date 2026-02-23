#include <doctest/doctest.h>

#include "python3_test_env.h"
#include "python3_wrappers.h"

#include <variant>
#include <vector>

TEST_CASE("single file module")
{
	auto& env = python_test_env();

	auto hello = env.module_file.load_entity("callable=hello_world", {}, {metaffi_string8_type});
	auto [msg] = hello.call<std::string>();
	CHECK(msg.find("single file") != std::string::npos);

	auto ret_null = env.module_file.load_entity("callable=return_null", {}, {metaffi_null_type});
	auto [null_val] = ret_null.call<metaffi_variant>();
	CHECK(std::holds_alternative<cdt_metaffi_handle>(null_val));
	CHECK(std::get<cdt_metaffi_handle>(null_val).handle == nullptr);

	auto get_buffers = env.module_file.load_entity("callable=get_three_buffers", {}, {metaffi_array_type});
	auto [buffers] = get_buffers.call<std::vector<std::vector<uint8_t>>>();
	CHECK(buffers.size() == 3);
	CHECK(buffers[0] == std::vector<uint8_t>({1, 2, 3, 4}));
	CHECK(buffers[1] == std::vector<uint8_t>({5, 6, 7}));
	CHECK(buffers[2] == std::vector<uint8_t>({8, 9}));

	auto positional_only = env.module_file.load_entity(
		"callable=positional_only",
		{metaffi_int64_type, metaffi_int64_type, metaffi_int64_type},
		{metaffi_int64_type});
	auto [pos_sum] = positional_only.call<int64_t>(int64_t(1), int64_t(2), int64_t(3));
	CHECK(pos_sum == 6);

	auto keyword_only = env.module_file.load_entity(
		"callable=keyword_only,named_args",
		{metaffi_handle_type},
		{metaffi_string8_type});
	auto kw_dict = PyDict::create();
	kw_dict.set(std::string("named"), std::string("x"));
	auto [kw_val] = keyword_only.call<std::string>(*kw_dict.handle());
	CHECK(kw_val == "x");

	auto var_args = env.module_file.load_entity(
		"callable=var_args,varargs",
		{metaffi_int64_packed_array_type},
		{metaffi_int64_packed_array_type});
	std::vector<int64_t> args = {1, 2};
	auto [va] = var_args.call<std::vector<int64_t>>(args);
	CHECK(va == args);

	auto var_kwargs = env.module_file.load_entity(
		"callable=var_kwargs,named_args",
		{metaffi_handle_type},
		{metaffi_handle_type});
	auto kwargs = PyDict::create();
	kwargs.set(std::string("a"), int64_t(1));
	auto [kw_ptr] = var_kwargs.call<cdt_metaffi_handle*>(*kwargs.handle());
	PyDict kw_out{PyHandle{kw_ptr}};
	auto kw_a = kw_out.get_any(std::string("a"));
	CHECK(std::holds_alternative<metaffi_int64>(kw_a));
	CHECK(std::get<metaffi_int64>(kw_a) == 1);
}
