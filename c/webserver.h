#include "quickjs.h"
#include "cutils.h"
#include "httpserver.h"

JSModuleDef *js_init_module_webserver(JSContext *ctx, const char *module_name);

typedef struct {
    int reqId;
    struct http_request_s *request;
} JSRequest;

typedef struct {
    JSRequest *requests;
    size_t size;
    size_t used;
} JSRequestArray;

typedef struct {
    const char *content;
    size_t size;
} QWSResponseBody;

typedef struct {
    char *buffer;
    size_t size;
} FileData;

typedef struct {
    JSContext *serverContext;
    JSValue callbackFunction;
    JSRequestArray serverRequests;
} QWSServerContext;

void initRequestsArray(JSRequestArray *requests, size_t initialSize) {
    requests->requests = malloc(initialSize * sizeof(JSRequest));
    requests->used = 0;
    requests->size = initialSize;
}

void registerRequest(JSRequestArray *requests, JSRequest element) {
    if (requests->used == requests->size) {
        requests->size += 1;
        requests->requests = realloc(requests->requests, requests->size * sizeof(JSRequest));
    }
    requests->requests[requests->used++] = element;
}

JSRequest *getRequestById(int requestId, JSRequestArray *array, int *requestIndex) {
    JSRequest *result = NULL; 
    for (int i = 0; i < array->size; ++i) {
        if (array->requests[i].reqId == requestId) {
            result = &array->requests[i];
            *requestIndex = i;
            break;
        }
    }
    return result;
}

void stringSlice(char *dest, const char *source, int length) {
	memcpy(dest, source, length);
	dest[length] = '\0';
}

void removeSubstr(char *string, char *sub) {
    char *match;
    int len = strlen(sub);
    while ((match = strstr(string, sub))) {
        *match = '\0';
        strcat(string, match+len);
    }
}

int stringStartsWith(const char *a, const char *b) {
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

void readFile(FileData *file, const char *path) {
  char *buffer;
  FILE *fileHandler = fopen(path, "rb");
  if ( fileHandler != NULL )
  {
    fseek(fileHandler, 0L, SEEK_END);
    long size = ftell(fileHandler);
    rewind(fileHandler);
    buffer = malloc(size);
    if ( buffer != NULL )
    {
      fread(buffer, size, 1, fileHandler);
      fclose(fileHandler); fileHandler = NULL;

      if (fileHandler != NULL) fclose(fileHandler);

      file->buffer = buffer;
      file->size = size;
    }
  }
  else {
      file->buffer = "error";
      file->size = 0;
  }
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
