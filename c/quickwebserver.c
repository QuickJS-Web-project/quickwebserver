#include "quickjs.h"
#include "cutils.h"
#include "quickjs-libc.h"

#define HTTPSERVER_IMPL
#include "httpserver.h"
#include "quickwebserver.h"

QWSServerContext QWS;
struct http_server_s* server;
JSRequestArray *serverRequests;

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
 * Parsing HTTP request and creating JS object with
 * request data (url, method, headers)
 * 
 */ 
void parseHttp(struct http_request_s* request) {
	JSValue returnObject;

	returnObject = JS_NewObject(QWS.serverContext);

	// Getting URL 
	http_string_t target = http_request_target(request);
	char url[target.len];
	stringSlice(url, target.buf, target.len);
	JS_DefinePropertyValueStr(QWS.serverContext, returnObject, "url",
                                  JS_NewString(QWS.serverContext, url),
                                  JS_PROP_C_W_E);

	// Getting request method
	http_string_t method = http_request_method(request);
	char request_method[method.len];
	stringSlice(request_method, method.buf, method.len);
	JS_DefinePropertyValueStr(QWS.serverContext, returnObject, "method",
                                  JS_NewString(QWS.serverContext, request_method),
                                  JS_PROP_C_W_E);

	// Getting content type
	http_string_t contentTypeHeader = http_request_header(request, "Content-Type");
	char request_content_type[contentTypeHeader.len];
	const char *request_content_type_p = request_content_type;
	stringSlice(request_content_type, contentTypeHeader.buf, contentTypeHeader.len);

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

		// If body is too large, we should check if key and value of header are 
		// filled; else we will face an infinite loop and server will crash
		if (strlen(header_key_p) == 0 && strlen(header_value_p) == 0) break;

		JS_DefinePropertyValueStr(QWS.serverContext, headersObject, header_key,
								  JS_NewString(QWS.serverContext, header_value),
								  JS_PROP_C_W_E);
	}
	JS_DefinePropertyValueStr(QWS.serverContext, returnObject, "headers", headersObject, JS_PROP_C_W_E);

	// Getting request body
	http_string_t body = http_request_body(request);
	short is_form_data = strstr(request_content_type_p, "multipart/form-data") != NULL;

	if (http_request_has_flag(request, HTTP_FLG_STREAMED)) {
		http_string_t contentLengthHeader = http_request_header(request, "Content-Length");
		char request_content_length[contentLengthHeader.len];
		stringSlice(request_content_length, contentLengthHeader.buf, contentLengthHeader.len);
		char *request_content_length_p = request_content_length, *end;
		long int content_length = strtol(request_content_length_p, &end, 10);

		chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
		printf("%d\n",content_length);
    	chunk_buffer->buf = (char*)malloc(content_length);
		chunk_buffer->request = request;
		chunk_buffer->parsedDataObject = &returnObject;
		http_request_set_userdata(request, chunk_buffer);

		deal_with_chunked(request, request_content_type_p);
	} else {
		if (is_form_data && body.len != 0) 
			getMultipartFile(body.buf, body.len, request_content_type_p);
	}
	// @hotfix: big request payloads causing app failure; chuked body parsing is not understandable. 
	// For now just handling zero-length body and notifying user about possible issue. 
	if (body.len != 0) {
		// JS_DefinePropertyValueStr(QWS.serverContext, returnObject, "body",
        //                           JS_NewString(QWS.serverContext, body.buf),
        //                           JS_PROP_C_W_E);
	} else {
		if (is_form_data) {
			// hs_error_response(request, 413, "Payload too large");
			// return 0;
		}
	}
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
 * What happens when request hits our server
 * 
 */ 
void requestCallback(struct http_request_s* request, JSValue httpObject) {
	// Parsing request and calling JS calback with
	// parsed data
	// JSValue httpObject;
	// if (!parseHttp(&httpObject, request)) return;

	JSValue cbFunc, cbReturnValue;
	JSRequest jsRequest;
	jsRequest.reqId = QWS.requestsCount + 1;
	jsRequest.request = request;
	ARRAY_PUSH(serverRequests, jsRequest);
	QWS.requestsCount++;

	JSValue requestId = JS_NewInt32(QWS.serverContext, jsRequest.reqId);
	JSValue callbackObject = JS_NewObject(QWS.serverContext);
	JS_DefinePropertyValueStr(QWS.serverContext, callbackObject, "http", httpObject, JS_PROP_C_W_E);
	JS_DefinePropertyValueStr(QWS.serverContext, callbackObject, "requestId", requestId, JS_PROP_C_W_E);
	cbFunc = JS_DupValue(QWS.serverContext, QWS.callbackFunction);
	cbReturnValue = JS_Call(QWS.serverContext, cbFunc, JS_UNDEFINED, 1, (JSValueConst *)&callbackObject);

	JS_FreeValue(QWS.serverContext, cbFunc);
	JS_FreeValue(QWS.serverContext, httpObject);
	JS_FreeValue(QWS.serverContext, cbReturnValue);
	JS_FreeValue(QWS.serverContext, requestId);
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

  	struct http_server_s* server = http_server_init(port, parseHttp);
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
