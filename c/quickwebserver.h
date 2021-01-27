#include "quickjs.h"
#include "cutils.h"
#include "httpserver.h"

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_quickwebserver
#endif

typedef struct {
    int reqId;
    struct http_request_s *request;
    JSValue parsed;
} JSRequest;

typedef struct {
    JSRequest *data;
    size_t capacity;
    size_t size;
} JSRequestArray;

typedef struct {
    const char *content;
    size_t size;
} QWSResponseBody;

typedef struct {
    JSContext *serverContext;
    JSValue callbackFunction;
    int requestsCount;
} QWSServerContext;

typedef struct {
  char *buf;
  int index;
  int req_id;
  char *content_type;
} chunk_buf_t;

typedef struct {
  char *buf;
  int index;
  int requestId;
  struct http_request_s* request;
} body_buf_t;

JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name);
static int js_quickwebserver_init(JSContext *ctx, JSModuleDef *m);
static JSValue startServer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
void requestCallback(int reqId);
static JSValue serverRespond(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
void response(struct http_request_s* request, JSValue jsHandlerData, JSContext *ctx);
void acceptHttpHeaders(struct http_response_s *response, JSValue headers, JSContext *ctx);

static const JSCFunctionListEntry js_quickwebserver_funcs[];

JSRequestArray *serverRequests;
QWSServerContext QWS;
struct http_server_s* server;

#include "quickwebserver-utils.h"

JSRequest *getRequestById(int requestId, JSRequestArray *array, int *requestIndex) {
    JSRequest *result = NULL; 
    for (int i = 0; i < array->size; ++i) {
        if (array->data[i].reqId == requestId) {
            result = &array->data[i];
            *requestIndex = i;
            break;
        }
    }
    return result;
}

void createTextResponse(QWSResponseBody *response, const char *text) {
    response->content = text;
    response->size = strlen(text);
}

void createFileResponse(QWSResponseBody *response, const char *filePath) {
    FileData file;
    readFile(&file, filePath);
    response->content = file.buffer;
    response->size = file.size;
}

void chunk_req_cb(struct http_request_s* request) {
  http_string_t str = http_request_chunk(request);
  chunk_buf_t* chunk_buffer = (chunk_buf_t*)http_request_userdata(request);
  if (str.len > 0) {
    memcpy(chunk_buffer->buf + chunk_buffer->index, str.buf, str.len);
    chunk_buffer->index += str.len;
    http_request_read_chunk(request, chunk_req_cb);
  } else {
    key_value_array *body_arr = parseMultipartBody(chunk_buffer->buf, chunk_buffer->index, chunk_buffer->content_type);
    int requestIndex;
    JSRequest *req = getRequestById(chunk_buffer->req_id, serverRequests, &requestIndex);
    JSValue js_body = JS_NewObject(QWS.serverContext);
    for (int i = 0; i < body_arr->size; i++) {
        JS_DefinePropertyValueStr(QWS.serverContext, js_body, body_arr->data[i].key, 
                        JS_NewString(QWS.serverContext, body_arr->data[i].value), 
                        JS_PROP_C_W_E);
    }
    JS_DefinePropertyValueStr(QWS.serverContext, req->parsed, "body",
                        js_body,
                        JS_PROP_C_W_E);
    int req_id = chunk_buffer->req_id;
    requestCallback(req_id);
    free(chunk_buffer->buf);
    free(chunk_buffer);
  }
}

void getRequestBody(struct http_request_s* request, int requestId) {
    http_string_t contentTypeHeader = http_request_header(request, "Content-Type");
	char request_content_type[contentTypeHeader.len];
	const char *request_content_type_p = request_content_type;
	stringSlice(request_content_type, contentTypeHeader.buf, contentTypeHeader.len);
    short is_form_data = strstr(request_content_type_p, "multipart/form-data") != NULL;

    http_string_t contentLengthHeader = http_request_header(request, "Content-Length");
    char request_content_length[contentLengthHeader.len];
    stringSlice(request_content_length, contentLengthHeader.buf, contentLengthHeader.len);
    char *request_content_length_p = request_content_length, *end;
    long int content_length = strtol(request_content_length_p, &end, 10);

    if (http_request_has_flag(request, HTTP_FLG_STREAMED)) {
        // @todo write for
        printf("Chunked!\n");
        chunk_buf_t* chunk_buffer = (chunk_buf_t*)calloc(1, sizeof(chunk_buf_t));
        chunk_buffer->buf = (char*)malloc(content_length);
        chunk_buffer->req_id = requestId;
        chunk_buffer->content_type = malloc((strlen(request_content_type) + 1) * sizeof(char *));
        chunk_buffer->content_type[strlen(request_content_type)] = '\n';
        strcpy(chunk_buffer->content_type, request_content_type);

        http_request_set_userdata(request, chunk_buffer);
        http_request_read_chunk(request, chunk_req_cb);
    } else {
        int requestIndex;
        JSRequest *req = getRequestById(requestId, serverRequests, &requestIndex);
        http_string_t body = http_request_body(request);
        if (is_form_data && body.len != 0) {
            key_value_array *body_arr = parseMultipartBody(body.buf, content_length, request_content_type_p);
            JSValue js_body = JS_NewObject(QWS.serverContext);
            for (int i = 0; i < body_arr->size; i++) {
                JS_DefinePropertyValueStr(QWS.serverContext, js_body, body_arr->data[i].key, 
                                JS_NewString(QWS.serverContext, body_arr->data[i].value), 
                                JS_PROP_C_W_E);
            }
            JS_DefinePropertyValueStr(QWS.serverContext, req->parsed, "body",
                                js_body,
                                JS_PROP_C_W_E);
            free(body_arr);
        } else {
            JS_DefinePropertyValueStr(QWS.serverContext, req->parsed, "body",
                                JS_NewString(QWS.serverContext, body.buf),
                                JS_PROP_C_W_E);
        }
        requestCallback(requestId);
    }
}
