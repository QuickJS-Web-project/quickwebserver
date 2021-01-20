#include "quickjs.h"
#include "cutils.h"
#include "quickjs-libc.h"

#define HTTPSERVER_IMPL
#include "httpserver.h"
#include "quickwebserver.h"

/**
 * Extracting headers from JS object and applying them to the
 * current server response
 * 
 */
void acceptHttpHeaders(struct http_response_s *response, JSValue headers, JSContext *ctx) {
	JSPropertyEnum *props;
	uint32_t propsCount, i;
	const char *propValueStr, *propKeyStr;
	JSValue propValue;
	JS_GetOwnPropertyNames(ctx, &props, &propsCount, headers, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY);
	for(i = 0; i < propsCount; i++) {
        propValue = JS_GetProperty(ctx, headers, props[i].atom);
		if (!JS_IsUndefined(propValue)) {
			propKeyStr = JS_AtomToCString(ctx, props[i].atom);
			propValueStr = JS_ToCString(ctx, propValue);	
			http_response_header(response, propKeyStr, propValueStr);
		}
	}
	JS_FreeValue(ctx, propValue);
}

/**
 * Sending response to client
 * 
 */ 
void response(struct http_request_s* request, JSValue jsHandlerData, JSContext *ctx) {
	size_t len;

	if (JS_IsException(jsHandlerData))
		js_std_dump_error(ctx);
	// Getting data from callback called above:
	// status, response headers, response body
    const char *callbackResponse, *responseType;
    int status;
    JSValue content = JS_GetPropertyStr(ctx, jsHandlerData, "content");
	JSValue httpStatus =  JS_GetPropertyStr(ctx, jsHandlerData, "status");
	JSValue headers = JS_GetPropertyStr(ctx, jsHandlerData, "headers");
	JSValue respType = JS_GetPropertyStr(ctx, jsHandlerData, "responseType");
	JS_ToInt32(ctx, &status, httpStatus);
	responseType = JS_ToCStringLen(ctx, &len, respType);

	QWSResponseBody respBody;

	if (!JS_IsUndefined(content) && !JS_IsNull(content)) {
		callbackResponse = JS_ToCStringLen(ctx, &len, content);
		if (strcmp(responseType, "string") == 0) {
			createTextResponse(&respBody, callbackResponse);
		} else if (strcmp(responseType, "file") == 0) {
			createFileResponse(&respBody, callbackResponse);
			if (strcmp(respBody.content, "error") == 0) {
				responseType = "string";
				createTextResponse(&respBody, "File not found");
				status = 404;
			}
		}
	} else {
		createTextResponse(&respBody, "");
		status = 204;
	}	

	// Creating response object and sending 
	// response to client
	struct http_response_s* response = http_response_init();
	http_response_status(response, status);
	acceptHttpHeaders(response, headers, ctx);
	http_response_body(response, respBody.content, respBody.size);
	http_respond(request, response);

	JS_FreeValue(ctx, content);
	JS_FreeValue(ctx, httpStatus);
	JS_FreeValue(ctx, headers);
	JS_FreeValue(ctx, respType);

	if (strcmp(responseType, "file") == 0) {
		const char *buffer;
		buffer = respBody.content;
		free((void *)buffer);
	}
}

/**
 * JS-middle-function for passing data to the response. 
 * Finds request with needed ID and sends the data 
 * to the next method
 * 
 */ 
static JSValue serverRespond(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
	int requestId, requestIndex;
	JS_ToInt32(ctx, &requestId, argv[0]);

	JSRequest *request = getRequestById(requestId, serverRequests, &requestIndex);
	if (!request) {
		JS_ThrowInternalError(ctx, "[QuickWebServer] Request not found");
	}

	struct http_request_s* serverRequest;	
	serverRequest = request->request;
	response(serverRequest, argv[1], ctx);

	ARRAY_REMOVE(serverRequests, requestIndex, 1);

	return JS_NewInt32(ctx, 0);
}

/**
 * Parsing is done: time to pass data to JS
 * 
 */ 
void requestCallback(int reqId) {
	JSValue cbFunc, cbReturnValue;

	int reqIdx = 0;
	JSRequest *req = getRequestById(reqId, serverRequests, &reqIdx);

	JSValue requestId = JS_NewInt32(QWS.serverContext, req->reqId);
	JSValue callbackObject = JS_NewObject(QWS.serverContext);
	JS_DefinePropertyValueStr(QWS.serverContext, callbackObject, "http", req->parsed, JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(QWS.serverContext, callbackObject, "requestId", requestId, JS_PROP_C_W_E);
	cbFunc = JS_DupValue(QWS.serverContext, QWS.callbackFunction);
	cbReturnValue = JS_Call(QWS.serverContext, cbFunc, JS_UNDEFINED, 1, (JSValueConst *)&callbackObject);

	JS_FreeValue(QWS.serverContext, cbFunc);
	JS_FreeValue(QWS.serverContext, cbReturnValue);
	JS_FreeValue(QWS.serverContext, requestId);
}

void parseRequest(struct http_request_s* request) {
	JSRequest jsRequest;
	jsRequest.reqId = QWS.requestsCount + 1;
	jsRequest.request = request;

	jsRequest.parsed = JS_NewObject(QWS.serverContext);


	// Getting URL 
	http_string_t target = http_request_target(request);
	char url[target.len];
	stringSlice(url, target.buf, target.len);
	JS_DefinePropertyValueStr(QWS.serverContext, jsRequest.parsed, "url",
                                  JS_NewString(QWS.serverContext, url),
                                  JS_PROP_C_W_E);

	// Getting request method
	http_string_t method = http_request_method(request);
	char request_method[method.len];
	stringSlice(request_method, method.buf, method.len);
	JS_DefinePropertyValueStr(QWS.serverContext, jsRequest.parsed, "method",
                                  JS_NewString(QWS.serverContext, request_method),
                                  JS_PROP_C_W_E);

	// Getting all the headers
	JSValue headersObject;
	headersObject = JS_NewObject(QWS.serverContext);
	http_string_t key, val;
	int iter = 0;
	while (http_request_iterate_headers(request, &key, &val, &iter)) {
	    char header_key[key.len + 1], header_value[val.len + 1];
	    char *header_key_p = header_key;
	    char *header_value_p = header_value;
	    stringSlice(header_key_p, key.buf, key.len);
	    stringSlice(header_value_p, val.buf, val.len);
		JS_DefinePropertyValueStr(QWS.serverContext, headersObject, header_key,
								  JS_NewString(QWS.serverContext, header_value),
								  JS_PROP_C_W_E);
	}
	JS_DefinePropertyValueStr(QWS.serverContext, jsRequest.parsed, "headers", headersObject, JS_PROP_C_W_E);

	ARRAY_PUSH(serverRequests, jsRequest);
	QWS.requestsCount++;

	// Getting request body
	getRequestBody(request, jsRequest.reqId);
}

/**
 * JS-function to start server
 * 
 */ 
static JSValue startServer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
	if (argc != 2) {
        return JS_ThrowReferenceError(ctx, "[QuickWebServer] Wrong number of arguments for server launching");
	}
	if (!JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowReferenceError(ctx, "[QuickWebServer] Main server event callback function not found");
	}
    if (!JS_IsNumber(argv[1])) {
        return JS_ThrowTypeError(ctx, "[QuickWebServer] Port is not a number");
    }

    ARRAY_CREATE(serverRequests, 2, 0);

	QWS.serverContext = ctx;
	QWS.callbackFunction = argv[0];
	QWS.requestsCount = 1;

	int port;
	JS_ToInt32(ctx, &port, argv[1]);

  	struct http_server_s* server = http_server_init(port, parseRequest);
	http_server_listen(server);

	return JS_NewInt32(ctx, 0);
}

/**
 * JS finctions list
 * 
 */ 
static const JSCFunctionListEntry js_quickwebserver_funcs[] = {
    JS_CFUNC_DEF("startServer", 2, startServer),
	JS_CFUNC_DEF("respond", 2, serverRespond),
};

/**
 * Set list of "exports" of our QuickJS module
 * 
 */ 
static int js_quickwebserver_init(JSContext *ctx, JSModuleDef *m) {
    return JS_SetModuleExportList(ctx, m, js_quickwebserver_funcs, countof(js_quickwebserver_funcs));
}

/**
 * Init QuickJS module
 * 
 */ 
JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name) {
    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_quickwebserver_init);
    
    if (!m)
        return NULL;
    
    JS_AddModuleExportList(ctx, m, js_quickwebserver_funcs, countof(js_quickwebserver_funcs));
    return m;
}
