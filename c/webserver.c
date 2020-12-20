#include "quickjs.h"
#include "cutils.h"
#include "quickjs-libc.h"

#define HTTPSERVER_IMPL
#include "httpserver.h"
#include "webserver.h"

JSValue callbackFunction;
JSContext *serverContext;

int requestsCount = 0;
JSRequestArray serverRequests;

pthread_t serverThread; 
struct http_server_s* server;

/**
 * Extracting headers from JS object and applying them to the
 * current server response
 * 
 */
void accept_http_headers(struct http_response_s* response, JSValue headers, JSContext *ctx) {
	JSPropertyEnum *props;
	uint32_t props_count, i;
	const char *prop_val_str, *prop_key_str;
	JSValue prop_val;
	JS_GetOwnPropertyNames(ctx, &props, &props_count, headers, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY);
	for(i = 0; i < props_count; i++) {
        prop_val = JS_GetProperty(ctx, headers, props[i].atom);
		if (!JS_IsUndefined(prop_val)) {
			prop_key_str = JS_AtomToCString(ctx, props[i].atom);
			prop_val_str = JS_ToCString(ctx, prop_val);	
			http_response_header(response, prop_key_str, prop_val_str);
		}
	}
	JS_FreeValue(ctx, prop_val);
}

/**
 * Parsing HTTP request and creating JS object with
 * request data (url, method, headers)
 * 
 */ 
JSValue parse_http(struct http_request_s* request) {
	JSValue ret_obj;

	ret_obj = JS_NewObject(serverContext);

	// Getting URL 
	http_string_t target = http_request_target(request);
	char url[target.len];
	string_slice(url, target.buf, target.len);
	JS_DefinePropertyValueStr(serverContext, ret_obj, "url",
                                  JS_NewString(serverContext, url),
                                  JS_PROP_C_W_E);

	// Getting request method
	http_string_t method = http_request_method(request);
	char request_method[method.len];
	string_slice(request_method, method.buf, method.len);
	JS_DefinePropertyValueStr(serverContext, ret_obj, "method",
                                  JS_NewString(serverContext, request_method),
                                  JS_PROP_C_W_E);

	// Getting request body
	http_string_t body = http_request_body(request);
	char request_body[body.len];
	string_slice(request_body, body.buf, body.len);
	JS_DefinePropertyValueStr(serverContext, ret_obj, "body",
                                  JS_NewString(serverContext, request_body),
                                  JS_PROP_C_W_E);

	// Getting all the headers
	JSValue headers_obj;
	headers_obj = JS_NewObject(serverContext);
	http_string_t key, val;
	int iter = 0;
	while (http_request_iterate_headers(request, &key, &val, &iter)) {
	    char header_key[key.len], header_value[val.len];
	    string_slice(header_key, key.buf, key.len);
	    string_slice(header_value, val.buf, val.len);
		JS_DefinePropertyValueStr(serverContext, headers_obj, header_key,
								  JS_NewString(serverContext, header_value),
								  JS_PROP_C_W_E);
	}
	JS_DefinePropertyValueStr(serverContext, ret_obj, "headers", headers_obj, JS_PROP_C_W_E);
	

	return ret_obj;
}

/**
 * Sending response to client
 * 
 */ 
void response(struct http_request_s* request, JSValue js_handler_data, JSContext *ctx) {
	size_t len;

	if (JS_IsException(js_handler_data))
		js_std_dump_error(ctx);
	// Getting data from callback called above:
	// status, response headers, response body
    const char *callbackResponse, *contentType, *responseType;
    int status;
    JSValue content = JS_GetPropertyStr(ctx, js_handler_data, "content");
	JSValue httpStatus =  JS_GetPropertyStr(ctx, js_handler_data, "status");
	JSValue headers = JS_GetPropertyStr(ctx, js_handler_data, "headers");
	JSValue respType = JS_GetPropertyStr(ctx, js_handler_data, "responseType");
	JS_ToInt32(ctx, &status, httpStatus);
	responseType = JS_ToCStringLen(ctx, &len, respType);

	QWSResponseBody respBody;

	if (!JS_IsUndefined(content) && !JS_IsNull(content)) {
		callbackResponse = JS_ToCStringLen(ctx, &len, content);
		if (strcmp(responseType, "string") == 0) {
			create_text_response(&respBody, callbackResponse);
		} else if (strcmp(responseType, "file") == 0) {
			create_file_response(&respBody, callbackResponse);
			if (strcmp(respBody.content, "error") == 0) {
				responseType = "string";
				create_text_response(&respBody, "File not found");
				status = 404;
			}
		}
	} else {
		create_text_response(&respBody, "");
		status = 204;
	}	

	// Creating response object and sending 
	// response to client
	struct http_response_s* response = http_response_init();
	http_response_status(response, status);
	accept_http_headers(response, headers, ctx);
	http_response_body(response, respBody.content, respBody.size);
	http_respond(request, response);

	JS_FreeValue(ctx, content);
	JS_FreeValue(ctx, httpStatus);
	JS_FreeValue(ctx, headers);
	JS_FreeValue(ctx, respType);

	if (strcmp(responseType, "file") == 0) {
		char *buffer;
		buffer = respBody.content;
		free(buffer);
	}
}

/**
 * JS-middle-function for passing data to the response. 
 * Finds request with needed ID and sends the data 
 * to the next method
 * 
 */ 
static JSValue server_respond(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
	int requestId, requestIndex;
	JS_ToInt32(ctx, &requestId, argv[0]);

	JSRequest *request = getRequestById(requestId, &serverRequests, &requestIndex);
	struct http_request_s* server_request;
	if (!request) {
		JS_ThrowReferenceError(ctx, "[QuickWebServer] Request to respond not found");
	}
	
	server_request = request->request;
	response(server_request, argv[1], ctx);
}

/**
 * What happens when request hits our server
 * 
 */ 
void request_callback(struct http_request_s* request) {
	JSValue func1, ret;

	JSRequest js_request;
	js_request.reqId = requestsCount + 1;
	js_request.request = request;
	registerRequest(&serverRequests, js_request);
	requestsCount++;

	// Parsing request and calling JS calback with
	// parsed data
	JSValue httpObject = parse_http(request);
	JSValue requestId = JS_NewInt32(serverContext, js_request.reqId);
	JSValue callbackObject = JS_NewObject(serverContext);
	JS_DefinePropertyValueStr(serverContext, callbackObject, "http", httpObject, JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(serverContext, callbackObject, "requestId", requestId, JS_PROP_C_W_E);
	func1 = JS_DupValue(serverContext, callbackFunction);
	ret = JS_Call(serverContext, func1, JS_UNDEFINED, 1, (JSValueConst *)&callbackObject);

	JS_FreeValue(serverContext, func1);
	JS_FreeValue(serverContext, httpObject);
	JS_FreeValue(serverContext, ret);
	JS_FreeValue(serverContext, requestId);
}

static JSValue start_server(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	if (argc != 2) {
        return JS_ThrowReferenceError(ctx, "[QuickWebServer] Wrong number of arguments for server launching");
	}
	if (!JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowReferenceError(ctx, "[QuickWebServer] Main server event callback function not found");
	}
    if (!JS_IsNumber(argv[1])) {
        return JS_ThrowTypeError(ctx, "[QuickWebServer] Port is not a number");
    }

	initRequestsArray(&serverRequests, 1);


	serverContext = ctx;
	callbackFunction = argv[0];

	int port;
	JS_ToInt32(ctx, &port, argv[1]);

  	struct http_server_s* server = http_server_init(port, request_callback);
	http_server_listen(server);
}

static JSValue greet(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NewString(ctx, "QuickJS Webserver :: Greetings!");
}

static const JSCFunctionListEntry js_webserver_funcs[] = {
    JS_CFUNC_DEF("greet", 0, greet),
    JS_CFUNC_DEF("startServer", 2, start_server),
	JS_CFUNC_DEF("respond", 2, server_respond),
};

static int js_webserver_init(JSContext *ctx, JSModuleDef *m)
{
    return JS_SetModuleExportList(ctx, m, js_webserver_funcs, countof(js_webserver_funcs));
}

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_webserver
#endif

JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name)
{
    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_webserver_init);
    
    if (!m)
        return NULL;
    
    JS_AddModuleExportList(ctx, m, js_webserver_funcs, countof(js_webserver_funcs));
    return m;
}
