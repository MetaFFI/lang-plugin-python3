"""
End-to-end tests for Python3 host compiler.

Tests the generated host code against the xllr.test plugin to verify:
1. The metaffi compiler generates valid host code
2. The generated code correctly interfaces with xllr.test

Expected values are based on test_entities.md documentation.

Required environment variables:
- METAFFI_SOURCE_ROOT: Path to MetaFFI source root (for IDL files)
- METAFFI_HOME: Path to MetaFFI build output (for xllr.test plugin)
"""

import unittest
import subprocess
import os
import sys
import shutil
import platform
import math
import signal

# Module-level state set by setUpModule
_generated_file = None
_host_module = None


def _returncode_desc(rc: int) -> str:
    if rc < 0:
        signum = -rc
        try:
            signame = signal.Signals(signum).name
        except ValueError:
            signame = "UNKNOWN"
        return f"{rc} (signal {signame}/{signum})"
    return str(rc)


def _get_xllr_test_plugin_path() -> str:
    metaffi_home = os.environ.get('METAFFI_HOME')
    if not metaffi_home:
        raise RuntimeError("METAFFI_HOME not set")

    if platform.system() == 'Windows':
        candidates = ['xllr.test.dll']
    elif platform.system() == 'Darwin':
        candidates = ['xllr.test.dylib', 'libxllr.test.dylib']
    else:
        candidates = ['xllr.test.so', 'libxllr.test.so']

    base_dir = os.path.join(metaffi_home, 'test')
    for lib_name in candidates:
        plugin_path = os.path.join(base_dir, lib_name)
        if os.path.exists(plugin_path):
            return plugin_path

    raise FileNotFoundError(
        f"xllr.test plugin not found in: {base_dir} (checked: {', '.join(candidates)})"
    )


def setUpModule():
    """Run metaffi compiler and bind the generated host module."""
    global _generated_file, _host_module

    metaffi_source_root = os.environ.get('METAFFI_SOURCE_ROOT')
    if not metaffi_source_root:
        raise unittest.SkipTest("METAFFI_SOURCE_ROOT not set")

    idl_path = os.path.join(
        metaffi_source_root, 'sdk', 'test_modules', 'guest_modules', 'test', 'xllr.test.idl.json'
    )
    if not os.path.exists(idl_path):
        raise unittest.SkipTest(f"IDL file not found: {idl_path}")

    # Output directory relative to this test file
    test_dir = os.path.dirname(os.path.abspath(__file__))
    
    os.chdir(test_dir)
    
    output_base_dir = os.path.join(test_dir, 'output')
    test_output_dir = os.path.join(output_base_dir, 'test')

    # Clean and create output directory
    if os.path.exists(output_base_dir):
        shutil.rmtree(output_base_dir)
    os.makedirs(output_base_dir, exist_ok=True)

    # Run metaffi compiler
    print(f"+++ py_host_compiler_e2e: invoking metaffi compiler", flush=True)
    print(f"+++ py_host_compiler_e2e: idl={idl_path}", flush=True)
    print(f"+++ py_host_compiler_e2e: cwd={output_base_dir}", flush=True)
    result = subprocess.run(
        ['metaffi', '-c', '--idl', idl_path, '-h', 'python3'],
        capture_output=True,
        text=True,
        cwd=output_base_dir
    )
    print(f"+++ py_host_compiler_e2e: metaffi returncode={_returncode_desc(result.returncode)}", flush=True)

    if result.stdout:
        print(f"Compiler stdout:\n{result.stdout}")
    if result.stderr:
        print(f"Compiler stderr:\n{result.stderr}")
        if "corrupted double-linked list" in result.stderr or "invalid pointer" in result.stderr:
            print("+++ py_host_compiler_e2e: allocator-corruption marker detected in stderr", flush=True)

    if result.returncode != 0:
        raise RuntimeError(f"Compiler failed with code {result.returncode}:\n{result.stderr}\ncmd: metaffi -c --idl {idl_path} -h python3\ncwd: {output_base_dir}")

    _generated_file = os.path.join(test_output_dir, 'host_MetaFFIHost.py')
    if not os.path.exists(_generated_file):
        raise RuntimeError(f"Generated file not found: {_generated_file}")

    # Add to path and import
    if test_output_dir not in sys.path:
        sys.path.insert(0, test_output_dir)

    import host_MetaFFIHost as host
    plugin_path = _get_xllr_test_plugin_path()
    print(f"+++ py_host_compiler_e2e: binding generated host to plugin={plugin_path}", flush=True)
    host.bind_module_to_code(plugin_path, 'test')
    print("+++ py_host_compiler_e2e: bind_module_to_code completed", flush=True)
    _host_module = host


def tearDownModule():
    """Clean up sys.path."""
    print("+++ py_host_compiler_e2e: tearDownModule start", flush=True)
    test_dir = os.path.dirname(os.path.abspath(__file__))
    test_output_dir = os.path.join(test_dir, 'output', 'test')
    if test_output_dir in sys.path:
        sys.path.remove(test_output_dir)
    print("+++ py_host_compiler_e2e: tearDownModule done", flush=True)


class TestCompilerGeneration(unittest.TestCase):
    """Tests for compiler code generation."""

    def test_compiler_generates_file(self):
        self.assertTrue(os.path.exists(_generated_file))
        self.assertTrue(_generated_file.endswith('host_MetaFFIHost.py'))

    def test_generated_file_has_expected_content(self):
        with open(_generated_file, 'r') as f:
            content = f.read()
        self.assertIn('def bind_module_to_code', content)
        self.assertIn('def return_int64', content)
        self.assertIn('class TestHandle', content)
        self.assertIn('def get_g_name', content)


class TestPrimitiveReturns(unittest.TestCase):
    """Tests for functions that return primitive values."""

    def test_return_int8(self):
        self.assertEqual(_host_module.return_int8(), 42)

    def test_return_int16(self):
        self.assertEqual(_host_module.return_int16(), 1000)

    def test_return_int32(self):
        self.assertEqual(_host_module.return_int32(), 100000)

    def test_return_int64(self):
        self.assertEqual(_host_module.return_int64(), 9223372036854775807)

    def test_return_uint8(self):
        self.assertEqual(_host_module.return_uint8(), 255)

    def test_return_uint16(self):
        self.assertEqual(_host_module.return_uint16(), 65535)

    def test_return_uint32(self):
        self.assertEqual(_host_module.return_uint32(), 4294967295)

    def test_return_uint64(self):
        self.assertEqual(_host_module.return_uint64(), 18446744073709551615)

    def test_return_float32(self):
        self.assertTrue(math.isclose(_host_module.return_float32(), 3.14159, rel_tol=1e-5))

    def test_return_float64(self):
        self.assertTrue(math.isclose(_host_module.return_float64(), 3.141592653589793, rel_tol=1e-10))

    def test_return_bool_true(self):
        self.assertIs(_host_module.return_bool_true(), True)

    def test_return_bool_false(self):
        self.assertIs(_host_module.return_bool_false(), False)

    def test_return_string8(self):
        self.assertEqual(_host_module.return_string8(), "Hello from test plugin")

    def test_return_null(self):
        self.assertIsNone(_host_module.return_null())


class TestPrimitiveAccepts(unittest.TestCase):
    """Tests for functions that accept primitive values."""

    def test_accept_int8(self):
        _host_module.accept_int8(42)

    def test_accept_int64(self):
        _host_module.accept_int64(12345678901234)

    def test_accept_float64(self):
        _host_module.accept_float64(3.14159)

    def test_accept_bool(self):
        _host_module.accept_bool(True)
        _host_module.accept_bool(False)

    def test_accept_string8(self):
        _host_module.accept_string8("test string")


class TestEchoFunctions(unittest.TestCase):
    """Tests for echo functions that return input unchanged."""

    def test_echo_int64(self):
        self.assertEqual(_host_module.echo_int64(123), 123)
        self.assertEqual(_host_module.echo_int64(-456), -456)
        self.assertEqual(_host_module.echo_int64(0), 0)

    def test_echo_float64(self):
        self.assertTrue(math.isclose(_host_module.echo_float64(3.14), 3.14, rel_tol=1e-10))
        self.assertTrue(math.isclose(_host_module.echo_float64(-2.718), -2.718, rel_tol=1e-10))

    def test_echo_string8(self):
        self.assertEqual(_host_module.echo_string8("test"), "test")
        self.assertEqual(_host_module.echo_string8(""), "")
        self.assertEqual(_host_module.echo_string8("hello world"), "hello world")

    def test_echo_bool(self):
        self.assertIs(_host_module.echo_bool(True), True)
        self.assertIs(_host_module.echo_bool(False), False)


class TestArithmeticFunctions(unittest.TestCase):
    """Tests for arithmetic functions."""

    def test_add_int64(self):
        self.assertEqual(_host_module.add_int64(3, 4), 7)
        self.assertEqual(_host_module.add_int64(0, 0), 0)
        self.assertEqual(_host_module.add_int64(-5, 10), 5)

    def test_add_float64(self):
        self.assertTrue(math.isclose(_host_module.add_float64(1.5, 2.5), 4.0, rel_tol=1e-10))

    def test_concat_strings(self):
        self.assertEqual(_host_module.concat_strings("Hello", " World"), "Hello World")
        self.assertEqual(_host_module.concat_strings("", "test"), "test")
        self.assertEqual(_host_module.concat_strings("abc", ""), "abc")


class TestArrayFunctions(unittest.TestCase):
    """Tests for array functions."""

    def test_return_int64_array_1d(self):
        self.assertEqual(list(_host_module.return_int64_array_1d()), [1, 2, 3])

    def test_return_int64_array_2d(self):
        result = _host_module.return_int64_array_2d()
        self.assertEqual([list(row) for row in result], [[1, 2], [3, 4]])

    def test_return_string_array(self):
        self.assertEqual(list(_host_module.return_string_array()), ['one', 'two', 'three'])

    def test_sum_int64_array(self):
        self.assertEqual(_host_module.sum_int64_array([1, 2, 3, 4, 5]), 15)

    def test_echo_int64_array(self):
        self.assertEqual(list(_host_module.echo_int64_array([10, 20, 30])), [10, 20, 30])

    def test_join_strings(self):
        self.assertEqual(_host_module.join_strings(['a', 'b', 'c']), 'a, b, c')


class TestTestHandleClass(unittest.TestCase):
    """Tests for the TestHandle class."""

    def test_constructor(self):
        h = _host_module.TestHandle()
        self.assertIsNotNone(h)
        self.assertGreaterEqual(h.id, 1)
        self.assertEqual(h.data, "test_data")

    def test_id_property_readonly(self):
        h1 = _host_module.TestHandle()
        h2 = _host_module.TestHandle()
        self.assertNotEqual(h1.id, h2.id)
        self.assertGreater(h2.id, h1.id)

    def test_data_property_getter(self):
        h = _host_module.TestHandle()
        self.assertEqual(h.data, "test_data")

    def test_data_property_setter(self):
        h = _host_module.TestHandle()
        h.data = "new_value"
        self.assertEqual(h.data, "new_value")

    def test_append_to_data_method(self):
        h = _host_module.TestHandle()
        h.data = "base"
        h.append_to_data("_suffix")
        self.assertEqual(h.data, "base_suffix")

    def test_multiple_operations(self):
        h = _host_module.TestHandle()
        initial_id = h.id
        self.assertEqual(h.data, "test_data")

        h.data = "modified"
        self.assertEqual(h.data, "modified")

        h.append_to_data("_appended")
        self.assertEqual(h.data, "modified_appended")

        self.assertEqual(h.id, initial_id)


class TestGlobalVariable(unittest.TestCase):
    """Tests for global variable g_name."""

    def test_get_g_name_default(self):
        _host_module.set_g_name("default_name")
        self.assertEqual(_host_module.get_g_name(), "default_name")

    def test_set_g_name(self):
        _host_module.set_g_name("test_value")
        self.assertEqual(_host_module.get_g_name(), "test_value")

    def test_g_name_persistence(self):
        _host_module.set_g_name("persistent_value")
        self.assertEqual(_host_module.get_g_name(), "persistent_value")
        self.assertEqual(_host_module.get_g_name(), "persistent_value")
        _host_module.set_g_name("default_name")


class TestErrorHandling(unittest.TestCase):
    """Tests for error handling functions."""

    def test_throw_error(self):
        with self.assertRaises(Exception) as ctx:
            _host_module.throw_error()
        self.assertIn("Test error thrown intentionally", str(ctx.exception))

    def test_throw_with_message(self):
        with self.assertRaises(Exception) as ctx:
            _host_module.throw_with_message("Custom error message")
        self.assertIn("Custom error message", str(ctx.exception))

    def test_error_if_negative_positive(self):
        _host_module.error_if_negative(42)
        _host_module.error_if_negative(0)

    def test_error_if_negative_negative(self):
        with self.assertRaises(Exception):
            _host_module.error_if_negative(-1)


class TestMultipleReturnValues(unittest.TestCase):
    """Tests for functions with multiple return values."""

    def test_return_two_values(self):
        self.assertEqual(_host_module.return_two_values(), (42, "answer"))

    def test_return_three_values(self):
        result = _host_module.return_three_values()
        self.assertEqual(result[0], 1)
        self.assertTrue(math.isclose(result[1], 2.5, rel_tol=1e-10))
        self.assertIs(result[2], True)

    def test_swap_values(self):
        self.assertEqual(_host_module.swap_values(123, "hello"), ("hello", 123))


if __name__ == '__main__':
    unittest.main()
