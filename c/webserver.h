#include "quickjs.h"
#include "cutils.h"
#include "httpserver.h"

JSModuleDef *js_init_module_webserver(JSContext *ctx, const char *module_name);

typedef struct {
    int reqId;
    struct http_request_s* request;
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

size_t remove_by_index( JSRequest a[], size_t n, size_t i )
{
    if ( i < n )
    {
        memmove( a + i, a + i + 1, ( n - i - 1 ) * sizeof( *a ) );
        --n;
    }

    return n;
}

void unregisterRequest(JSRequestArray* array, int indexToRemove)
{
    array->size = remove_by_index(array->requests, array->size, indexToRemove);
    array->used = array->used - 1;
    // @todo check memory leaks?
}

JSRequest *getRequestById(int requestId, JSRequestArray *array, int *requestIndex) {
    JSRequest *result; 
    for (int i = 0; i < array->size; ++i) {
        if (array->requests[i].reqId == requestId) {
            result = &array->requests[i];
            *requestIndex = i;
            break;
        }
    }
    return result;
}

/**
 * Simple wrapper for slicing strings with pleasure
 * 
 */ 
void string_slice(char *dest, const char *source, int length) {
	memcpy(dest, source, length);
	dest[length] = '\0';
}

void removeSubstr (char *string, char *sub) {
    char *match;
    int len = strlen(sub);
    while ((match = strstr(string, sub))) {
        *match = '\0';
        strcat(string, match+len);
    }
}

int string_starts_with(const char *a, const char *b) {
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

void read_file(FileData *file, const char *path) {
  char *buffer;
  FILE *fh = fopen(path, "rb");
  if ( fh != NULL )
  {
    fseek(fh, 0L, SEEK_END);
    long s = ftell(fh);
    rewind(fh);
    buffer = malloc(s);
    if ( buffer != NULL )
    {
      fread(buffer, s, 1, fh);
      fclose(fh); fh = NULL;

      if (fh != NULL) fclose(fh);

      file->buffer = buffer;
      file->size = s;
    }
  }
  else {
      file->buffer = "error";
      file->size = 0;
  }
}

void create_text_response(QWSResponseBody *response, const char *text) {
    response->content = text;
    response->size = strlen(text);
}

void create_file_response(QWSResponseBody *response, const char *filePath) {
    FileData file;
    read_file(&file, filePath);
    response->content = file.buffer;
    response->size = file.size;
}
