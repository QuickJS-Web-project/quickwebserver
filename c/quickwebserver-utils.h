#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
    char *buffer;
    unsigned long size;
} FileData;

typedef struct {
  char *buf;
  int index;
  struct http_request_s* request;
  JSValue *parsedDataObject;
} chunk_buf_t;

/**
 * For next 3 defines special thanks to https://github.com/andrei-markeev/ts2c
 */ 

#define ARRAY_CREATE(array, init_capacity, init_size) {\
    array = malloc(sizeof(*array)); \
    array->data = malloc((init_capacity) * sizeof(*array->data)); \
    assert(array->data != NULL); \
    array->capacity = init_capacity; \
    array->size = init_size; \
}

#define ARRAY_PUSH(array, item) {\
    if (array->size == array->capacity) {  \
        array->capacity *= 2;  \
        array->data = realloc(array->data, array->capacity * sizeof(*array->data)); \
        assert(array->data != NULL); \
    }  \
    array->data[array->size++] = item; \
}

#define ARRAY_REMOVE(array, pos, num) {\
    memmove(&(array->data[pos]), &(array->data[(pos) + num]), (array->size - (pos) - num) * sizeof(*array->data)); \
    array->size -= num; \
}

void stringSlice(char *dest, const char *source, int length) {
	memcpy(dest, source, length);
	dest[length] = '\0';
}

void readFile(FileData *file, const char *path) {
  char *buffer;
  FILE *fileHandler = fopen(path, "rb");
  if ( fileHandler != NULL )
  {
    fseek(fileHandler, 0L, SEEK_END);
    long size = ftell(fileHandler);
    rewind(fileHandler);
    buffer = (char *)malloc(size);
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

char *cut_string(const char *buffer, const char *start_from, char end_char) {
  char *start_with = strstr(buffer, start_from);
  short start_pos = start_with - buffer + strlen(start_from);
  short idx = 0;
  char *result = malloc(sizeof(char *) * idx);
  while (1)
  {
    char letter = buffer[start_pos + idx];
    if (letter == end_char) {
      result = realloc(result, sizeof(char*) * (idx - 1));
      result[idx] = '\0';
      break;
    }
    result = realloc(result, sizeof(char*) * (idx));
    result[idx] = letter;
    idx++;
  }
  return result;
}

char *content_type_global;

// @TODO: parse multiple files
// @TODO: parse rest body
void getMultipartFile(const char *buffer, size_t buf_length, const char *content_type) {
  printf("%s\n", content_type);

  char *boundary = cut_string(content_type, "boundary=", '\n');

  printf("Boundary: %s\n", boundary);

  // Closing boundary has "--" added to beginning and ending
  char b_end[strlen(boundary) + 5];
  snprintf(b_end, sizeof b_end, "%s%s%s", "--", boundary, "--");
  char *boundary_closing = b_end;

  char *ptr = strtok((char *)buffer, "\r\n");
  short content_next = 0;
  while(ptr != NULL)
	{
    if (strlen(ptr) == 1 && (ptr[0] == '\r' || ptr[0] == '\n')) {
      content_next = 1;
    }
    if (content_next == 1) {
      break;
    }
    		ptr = strtok(NULL, "\n\n");
	}

  size_t boundary_start = 0;
  size_t file_end = 0;
  size_t i = 0;
  while(i < buf_length) {
    char byte_v = ptr[i];

    if (boundary_start == 0 && byte_v == boundary_closing[0]) {
      boundary_start = i;
    }
    
    if (boundary_start != 0 && boundary_closing[i - boundary_start] == byte_v) {
      if (i - boundary_start == strlen(boundary_closing) - 1) {
        file_end = i - strlen(boundary_closing) - 1;
        break;
      }
    } else {
      boundary_start = 0;
    }
    i++;
  }

  printf("File end: %d\n", file_end);

  // @todo: real file name
  FILE *resFile = fopen("./test.png", "wb");
  for (int i = 2; i < file_end ; i++) {
    fputc(ptr[i], resFile);
  }
  fclose(resFile);
}

chunk_buf_t *chunk_b_ptr;

void chunk_req_cb(struct http_request_s* request) {
  http_string_t str = http_request_chunk(request);
  chunk_buf_t* chunk_buffer = (chunk_buf_t*)http_request_userdata(request);
  if (str.len > 0) {
    // printf("Reading chunk, %d\n", str.len);
    memcpy(chunk_buffer->buf + chunk_buffer->index, str.buf, str.len);
    chunk_buffer->index += str.len;
    http_request_read_chunk(request, chunk_req_cb);
  } else {
    // printf("Reading chunk finished! %s\n", content_type_global);
    // printf("%s\n", chunk_buffer->buf);
    getMultipartFile(chunk_buffer->buf, chunk_buffer->index, content_type_global);
    // requestCallback(request, chunk_buffer->parsedDataObject);
    // http_response_body(chunk_buffer->response, chunk_buffer->buf, chunk_buffer->index);
    // http_respond(request, chunk_buffer->response);
    // free(chunk_buffer->buf);
    // free(chunk_buffer);
  }
}

void deal_with_chunked(http_request_t *request, char *content_type) {
  // printf("Reading chunk start\n");
  printf("CT deal: %s\n", content_type);
  content_type_global = malloc(strlen(content_type));
  strcpy(content_type_global, content_type);
  http_request_read_chunk(request, chunk_req_cb);
}
