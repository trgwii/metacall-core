/*
 *	Loader Library by Parra Studios
 *	A plugin for loading nodejs code at run-time into a process.
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

#ifndef NODE_LOADER_IMPL_PORT_H
#define NODE_LOADER_IMPL_PORT_H 1

#include <node_loader/node_loader_api.h>

/* TODO:
	This is a workaround to expose type conversions from loader to the port.
	In the future this must be avoided, and a proper design must be implemented,
	exposing port capabilities from loader.
	The easiest way of doing it is passing metacallv, metacallt and all family to
	the port by a list of function pointers when the loader registers into metacall
	so we have a two way dependency injection. After doing this, the loader must export
	those functions to NodeJS runtime through N-API as like how it is done in the node port.
*/

#include <node_api.h>

#ifdef __cplusplus
extern "C" {
#endif

NODE_LOADER_API void * node_napi_to_value(napi_env env, napi_value v);

NODE_LOADER_API napi_value node_value_to_napi(napi_env env, void * arg);

NODE_LOADER_API void node_exception(napi_env env, napi_status status);

#ifdef __cplusplus
}
#endif

#endif /* NODE_LOADER_IMPL_PORT_H */
