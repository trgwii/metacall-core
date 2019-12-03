/*
 *	MetaCall NodeJS Port by Parra Studios
 *	A complete infrastructure for supporting multiple language bindings in MetaCall.
 *
 *	Copyright (C) 2016 - 2019 Vicente Eduardo Ferrer Garcia <vic798@gmail.com>
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

/* -- Headers -- */

#include <node_port/node_port.h>
#include <metacall/metacall.h>
#include <node_loader/node_loader_impl_port.h> /* TODO: Workaround, redesign this in the future */
#include <cstring>

/* -- Methods -- */

napi_value metacall_node(napi_env env, napi_callback_info info)
{
	size_t argc = 0, args_size = 0, name_length = 0;
	napi_value * argv = NULL;
	char * name;
	void ** args = NULL;
	void * ret = NULL;
	napi_value result = NULL;
	napi_status status;

	status = napi_get_cb_info(env, info, &argc, NULL, NULL, NULL);

	node_exception(env, status);

	if (argc == 0)
	{
		napi_throw_error(env, NULL, "Invalid number of arguments");

		return NULL;
	}

	argv = static_cast<napi_value *>(malloc(sizeof(napi_value) * argc));

	if (argv == NULL)
	{
		napi_throw_error(env, NULL, "Invalid MetaCall call argv allocation");

		return NULL;
	}

	args_size = argc - 1;

	args = static_cast<void **>(malloc(sizeof(void *) * args_size));

	if (args == NULL)
	{
		napi_throw_error(env, NULL, "Invalid MetaCall call args allocation");

		free(argv);

		return NULL;
	}

	status = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

	node_exception(env, status);

	status = napi_get_value_string_utf8(env, argv[0], NULL, 0, &name_length);

	node_exception(env, status);

	if (name_length == 0)
	{
		napi_throw_error(env, NULL, "Invalid MetaCall call name");

		free(args);
		free(argv);

		return NULL;
	}

	status = napi_get_value_string_utf8(env, argv[0], NULL, 0, &name_length);

	node_exception(env, status);

	name = static_cast<char *>(malloc(sizeof(char) * (name_length + 1)));

	if (name == NULL)
	{
		napi_throw_error(env, NULL, "Invalid MetaCall call name");

		free(args);
		free(argv);

		return NULL;
	}

	status = napi_get_value_string_utf8(env, argv[0], name, name_length + 1, &name_length);

	node_exception(env, status);

	for (size_t index = 0; index < args_size; ++index)
	{
		args[index] = node_napi_to_value(env, argv[index + 1]);
	}

	ret = metacallv(name, args);

	for (size_t index = 0; index < args_size; ++index)
	{
		metacall_value_destroy(args[index]);
	}

	if (ret != NULL)
	{
		result = node_value_to_napi(env, ret);
	}

	metacall_value_destroy(ret);

	free(name);
	free(args);
	free(argv);

	return result;
}

/* TODO: Review this function for safety problems */
// this function is the handler of the "metacall_load_from_file"
napi_value metacall_node_load_from_file(napi_env env, napi_callback_info info)
{
	size_t argc = 2, result;
	uint32_t length_of_JS_array;
	napi_value argv[argc];
	char tagBuf[18];
	napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
	// checks will be done in the JS Wrapper..... SO we believe whatever the JS_Wrapper is passing is valid
	napi_get_value_string_utf8(env, argv[0], tagBuf, 18, &result);
	napi_get_array_length(env, argv[1], &length_of_JS_array);
	const char ** file_name_strings = new const char *[256];
	size_t _result = 0;
	for (size_t i = 0; i < length_of_JS_array; i++)
	{
		napi_value tmpValue;
		napi_get_element(env, argv[1], i, &tmpValue);
		// converting to strings
		char c_strings[256] = { 0 };
		napi_coerce_to_string(env, tmpValue, &tmpValue);
		napi_get_value_string_utf8(env, tmpValue, c_strings, 256, &_result);
		file_name_strings[i] = new char[_result + 1];
		strncpy((char *)file_name_strings[i], c_strings, _result + 1);
	}
	if(_result == 0) return NULL;
	int met_result = metacall_load_from_file(tagBuf, file_name_strings, sizeof(file_name_strings)/sizeof(file_name_strings[0]), NULL);
	if (met_result > 0)
	{
		napi_throw_error(env, NULL, "Metacall could not load from file");
		return NULL;
	}

	/* TODO: Return result for successful monkey patch later on */
	return NULL;
}

/* TODO: Add documentation */
napi_value metacall_node_inspect(napi_env env, napi_callback_info)
{
	napi_value result;
	size_t size = 0;
	struct metacall_allocator_std_type std_ctx = { &malloc, &realloc, &free };
	void * allocator = metacall_allocator_create(METACALL_ALLOCATOR_STD, (void *)&std_ctx);
	char * inspect_str = metacall_inspect(&size, allocator);
	napi_status status;

	if (!(inspect_str != NULL && size != 0))
	{
		napi_throw_error(env, NULL, "Invalid MetaCall inspect string");
	}

	status = napi_create_string_utf8(env, inspect_str, size - 1, &result);

	node_exception(env, status);

	metacall_allocator_free(allocator, inspect_str);

	metacall_allocator_destroy(allocator);

	return result;
}

/* TODO: Add documentation */
napi_value metacall_node_logs(napi_env env, napi_callback_info)
{
	struct metacall_log_stdio_type log_stdio = { stdout };

	if (metacall_log(METACALL_LOG_STDIO, (void *)&log_stdio) != 0)
	{
		napi_throw_error(env, NULL, "MetaCall failed to initialize debug logs");
	}

	return NULL;
}

/* TODO: Review documentation */
// This functions sets the necessary js functions that could be called in NodeJs
void metacall_node_exports(napi_env env, napi_value exports)
{
	const char function_metacall_str[] = "metacall";
	const char function_load_from_file_str[] = "metacall_load_from_file";
	const char function_inspect_str[] = "metacall_inspect";
	const char function_logs_str[] = "metacall_logs";

	napi_value function_metacall, function_load_from_file, function_inspect, function_logs;

	napi_create_function(env, function_metacall_str, sizeof(function_metacall_str) - 1, metacall_node, NULL, &function_metacall);
	napi_create_function(env, function_load_from_file_str, sizeof(function_load_from_file_str) - 1, metacall_node_load_from_file, NULL, &function_load_from_file);
	napi_create_function(env, function_inspect_str, sizeof(function_inspect_str) - 1, metacall_node_inspect, NULL, &function_inspect);
	napi_create_function(env, function_logs_str, sizeof(function_logs_str) - 1, metacall_node_logs, NULL, &function_logs);

	napi_set_named_property(env, exports, function_metacall_str, function_metacall);
	napi_set_named_property(env, exports, function_load_from_file_str, function_load_from_file);
	napi_set_named_property(env, exports, function_inspect_str, function_inspect);
	napi_set_named_property(env, exports, function_logs_str, function_logs);
}

/* TODO: Review documentation */
/* This function is called by NodeJs when the module is required */
napi_value metacall_node_initialize(napi_env env, napi_value exports)
{
	if (metacall_initialize() != 0)
	{
		/* TODO: Show error message (when error handling is properly implemented in the core lib) */
		napi_throw_error(env, NULL, "MetaCall failed to initialize");

		return NULL;
	}

	metacall_node_exports(env, exports);

	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, metacall_node_initialize)
