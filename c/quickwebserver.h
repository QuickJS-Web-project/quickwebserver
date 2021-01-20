#include "quickjs.h"
#include "cutils.h"
#include "httpserver.h"

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_quickwebserver
#endif

JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name);
static int js_quickwebserver_init(JSContext *ctx, JSModuleDef *m);
static JSValue startServer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
void requestCallback(struct http_request_s* request, int reqId);
static JSValue serverRespond(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
void response(struct http_request_s* request, JSValue jsHandlerData, JSContext *ctx);
void acceptHttpHeaders(struct http_response_s *response, JSValue headers, JSContext *ctx);

static const JSCFunctionListEntry js_quickwebserver_funcs[];

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
  int requestId;
  struct http_request_s* request;
} body_buf_t;

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

void getRequestBody(struct http_request_s* request, int requestId) {
    if (http_request_has_flag(request, HTTP_FLG_STREAMED)) {
        // @todo write for
    } else {
        int requestIndex;
        JSRequest *req = getRequestById(requestId, serverRequests, &requestIndex);
        http_string_t body = http_request_body(request);
        JS_DefinePropertyValueStr(QWS.serverContext, req->parsed, "body",
                                JS_NewString(QWS.serverContext, body.buf),
                                JS_PROP_C_W_E);
        requestCallback(request, requestId);
    }
}
