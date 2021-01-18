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
void requestCallback(struct http_request_s* request, JSValue httpObject);
static JSValue serverRespond(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
void response(struct http_request_s* request, JSValue jsHandlerData, JSContext *ctx);
void parseHttp(struct http_request_s* request);
void acceptHttpHeaders(struct http_response_s *response, JSValue headers, JSContext *ctx);

#include "quickwebserver-utils.h"

static const JSCFunctionListEntry js_quickwebserver_funcs[];

typedef struct {
    int reqId;
    struct http_request_s *request;
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
