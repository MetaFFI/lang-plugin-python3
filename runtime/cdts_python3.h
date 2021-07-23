#pragma once
#include <runtime/cdts_wrapper.h>
#include <memory>
#include <Python.h>

class cdts_python3
{
private:
	std::unique_ptr<openffi::runtime::cdts_wrapper> cdts;
	
public:
	explicit cdts_python3(cdt* cdts, openffi_size cdts_length);
	explicit cdts_python3(openffi_size cdt_count);
	
	cdt* get_cdts();
	
	/**
	 * @brief Parses CDTS and returns PyTuple. Method assumes CDTS is filled!
	 * @return PyTuple with CDTS in Python objects
	 */
	PyObject* parse();
	
	/**
	 * @brief Builds CDTS from given Tuple with python objects. Function assumes CDTS is empty and in the correct size!
	 * @param tuple PyTuple to fill CDTS
	 * @param starting_index Starting index in tuple to fill CDTS
	 */
	void build(PyObject* tuple, PyObject* tuple_types, int starting_index);


private:
	
	static openffi_types get_openffi_type(PyObject* pyobj);
	
	
	template<typename T>
	PyObject* create_list(const T& p)
	{
		if(p.dimensions == 0){ throw std::runtime_error("array with 0 dimensions"); }
		if(p.dimensions > 1){ throw std::runtime_error("multi-dimensions array is not supported yet"); }
		
		return PyList_New(p.dimensions_lengths[0]);
	}
	//--------------------------------------------------------------------
	template<typename T>
	void set_numeric_to_tuple(PyObject* tuple, int index, const T& val, const std::function<PyObject*(T)>& ConvFunc)
	{
		PyObject* pyval = ConvFunc(val);
		
		if(!pyval)
		{
			std::stringstream ss;
			ss << "Failed converting value during parse at index " << index;
			throw std::runtime_error(ss.str());
		}
		
		int err = 0;
		if(err = PyTuple_SetItem(tuple, index, pyval); err)
		{
			if(err == -1){ throw std::runtime_error("Setting to PyTuple during parse failed with out of bounds."); }
			else { throw std::runtime_error("Setting to PyTuple during parse failed."); }
		}
	}
	//--------------------------------------------------------------------
	template<typename T, typename char_t>
	void set_string_to_tuple(PyObject* tuple, int index, const T& val, const openffi_size& size, const std::function<PyObject*(const char_t*, Py_ssize_t)>& ConvFunc)
	{
		PyObject* pyval = ConvFunc((char_t*)val, size);
		if(!pyval)
		{
			std::stringstream ss;
			ss << "Failed converting value during parse at index " << index;
			throw std::runtime_error(ss.str());
		}
		
		int err = 0;
		if(err = PyTuple_SetItem(tuple, index, pyval); err)
		{
			if(err == -1){ throw std::runtime_error("Setting to PyTuple during parse failed with out of bounds."); }
			else { throw std::runtime_error("Setting to PyTuple during parse failed."); }
		}
	}
	//--------------------------------------------------------------------
	template<typename T>
	void set_numeric_array_to_tuple(PyObject* tuple, int cdts_index, const openffi::runtime::numeric_n_array_wrapper<T>& arr_wrap, const std::function<PyObject*(T)>& ConvFunc)
	{
		PyObject* list = create_list(arr_wrap);
		
		openffi_size index[1] = { 0 };
		
		for(int i=0 ; i<arr_wrap.dimensions_lengths[0] ; i++)
		{
			index[0] = i;
			PyObject* elem = ConvFunc(arr_wrap.get_elem_at(index, 1));
			if(PyList_SetItem(list, i, elem) == -1)
			{
				std::stringstream ss;
				ss << "List out of bounds. Setting index " << i << " to a list of size " << arr_wrap.dimensions_lengths[0];
				throw std::runtime_error(ss.str());
			}
		}
		
		int err = 0;
		if(err = PyTuple_SetItem(tuple, cdts_index, list); err)
		{
			if(err == -1){ throw std::runtime_error("Setting to PyTuple during parse failed with out of bounds."); }
			else { throw std::runtime_error("Setting to PyTuple during parse failed."); }
		}
	}
	//--------------------------------------------------------------------
	template<typename T, typename char_t>
	void set_string_array_to_tuple(PyObject* tuple, int cdts_index, const openffi::runtime::string_n_array_wrapper<T>& arr_wrap, const std::function<PyObject*(const char_t*, Py_ssize_t)>& c_to_pyobject)
	{
		PyObject* list = create_list(arr_wrap);
		
		openffi_size index[1] = { 0 };
		
		for(int i=0 ; i<arr_wrap.dimensions_lengths[0] ; i++)
		{
			index[0] = i;
			
			T str;
			openffi_size s;
			arr_wrap.get_elem_at(index, 1, &str, &s);
			
			PyObject* elem = c_to_pyobject((char_t*)str, (Py_ssize_t)s);
			if(!str){
				throw std::runtime_error("Failed to create PyUnicode");
			}
			
			if(PyList_SetItem(list, i, elem) == -1)
			{
				std::stringstream ss;
				ss << "List out of bounds. Setting index " << i << " to a list of size " << arr_wrap.dimensions_lengths[0];
				throw std::runtime_error(ss.str());
			}
		}
		
		int err = 0;
		if(err = PyTuple_SetItem(tuple, cdts_index, list); err)
		{
			if(err == -1){ throw std::runtime_error("Setting to PyTuple during parse failed with out of bounds."); }
			else { throw std::runtime_error("Setting to PyTuple during parse failed."); }
		}
	}
	
	//====================================================================

	template<typename T>
	void set_numeric_to_cdts(PyObject* tuple, int index, T& val, const std::function<T(PyObject*)>& pyobject_to_c, const std::function<int(PyObject*)>& check_pyobject)
	{
		PyObject* obj = PyTuple_GetItem(tuple, index);
		
		if(!check_pyobject(obj))
		{
			std::stringstream ss;
			ss << "Expecting numeric type at index: " << index << ". Given type is: " << obj->ob_type->tp_name;
			throw std::runtime_error(ss.str());
		}
		
		val = pyobject_to_c(obj);
	}
	
	template<typename T>
	void set_numeric_array_to_cdts(PyObject* tuple, int index, T*& arr, openffi_size*& dimensions_lengths, openffi_size& dimensions, const std::function<T(PyObject*)>& pyobject_to_c, const std::function<int(PyObject*)>& check_pyobject)
	{
		PyObject* obj = PyTuple_GetItem(tuple, index);
		
		if(!PyList_Check(obj) && !PyTuple_Check(obj))
		{
			std::stringstream ss;
			ss << "Expecting list/tuple type at index: " << index << ". Given type is: " << obj->ob_type->tp_name;
			throw std::runtime_error(ss.str());
		}
		
		std::function<PyObject*(PyObject*, Py_ssize_t)> get_item;
		Py_ssize_t size;
		
		if(PyList_Check(obj))
		{
			size = PyList_Size(obj);
			get_item = PyList_GetItem;
		}
		else
		{
			size = PyTuple_Size(obj);
			get_item = PyTuple_GetItem;
		}
		
		dimensions = 1; // TODO: handle multi-dimensions
		dimensions_lengths = (openffi_size*)malloc(sizeof(openffi_size));
		dimensions_lengths[0] = size;
		arr = (T*)malloc(sizeof(T)*size);
		
		for(int i=0 ; i<size ; i++)
		{
			PyObject* cur_item = get_item(obj, i);
			if(!check_pyobject(cur_item))
			{
				std::stringstream ss;
				ss << "Expecting numeric type at index: " << index << ". Given type is: " << cur_item->ob_type->tp_name;
				throw std::runtime_error(ss.str());
			}
			
			arr[i] = pyobject_to_c(cur_item);
		}
	}
	
	template<typename T, typename char_t>
	void set_string_to_cdts(PyObject* tuple, int index, T& val, openffi_size& s, const std::function<char_t*(PyObject*, Py_ssize_t*)>& pyobject_to_c, const std::function<int(PyObject*)>& check_pyobject, const std::function<char_t*(char_t*, const char_t*, size_t)>& scpy)
	{
		PyObject* obj = PyTuple_GetItem(tuple, index);
		
		if(!check_pyobject(obj))
		{
			std::stringstream ss;
			ss << "Expecting string type at index: " << index << ". Given type is: " << obj->ob_type->tp_name;
			throw std::runtime_error(ss.str());
		}
		
		char_t* temp = pyobject_to_c(obj, (Py_ssize_t*)&s); // temp "memory deallocated by python"
		if(s == 0) // size was not stored in python string
		{
			s = PyUnicode_GetLength(obj);
		}
		
		val = (T)malloc(sizeof(char_t)*s);
		scpy((char_t*)val, (const char_t*)temp, (size_t)s);
	}
	
	template<typename T, typename char_t>
	void set_string_array_to_cdts(PyObject* tuple, int index, T*& arr, openffi_size*& strings_lengths, openffi_size*& dimensions_lengths, openffi_size& dimensions, const std::function<char_t*(PyObject*, Py_ssize_t*)>& pyobject_to_c, const std::function<int(PyObject*)>& check_pyobject, const std::function<char_t*(char_t*, const char_t*, size_t)>& scpy)
	{
		PyObject* obj = PyTuple_GetItem(tuple, index);
		
		if(!PyList_Check(obj) && !PyTuple_Check(obj))
		{
			std::stringstream ss;
			ss << "Expecting list/tuple type at index: " << index << ". Given type is: " << obj->ob_type->tp_name;
			throw std::runtime_error(ss.str());
		}
		
		std::function<PyObject*(PyObject*, Py_ssize_t)> get_item;
		Py_ssize_t size;
		
		if(PyList_Check(obj))
		{
			size = PyList_Size(obj);
			get_item = PyList_GetItem;
		}
		else
		{
			size = PyTuple_Size(obj);
			get_item = PyTuple_GetItem;
		}
		
		dimensions = 1; // TODO: handle multi-dimensions
		dimensions_lengths = (openffi_size*)malloc(sizeof(openffi_size));
		dimensions_lengths[0] = size;
		
		strings_lengths = (openffi_size*)malloc(sizeof(openffi_size)*size);
		arr = (T*)malloc(sizeof(T)*size);
		
		for(int i=0 ; i<size ; i++)
		{
			PyObject* cur_item = get_item(obj, i);
			if(!check_pyobject(cur_item))
			{
				std::stringstream ss;
				ss << "Expecting string type at index: " << index << ". Given type is: " << cur_item->ob_type->tp_name;
				throw std::runtime_error(ss.str());
			}
			
			char_t* temp = pyobject_to_c(cur_item, (Py_ssize_t*)(&strings_lengths[i])); // temp "memory deallocated by python"
			if(strings_lengths[i] == 0) // size was not stored in python string
			{
				strings_lengths[i] = PyUnicode_GetLength(cur_item);
			}
			
			arr[i] = (T)malloc(sizeof(char_t)*strings_lengths[i]);
			scpy((char_t*)arr[i], (const char_t*)temp, (size_t)strings_lengths[i]);
		}
	}
};