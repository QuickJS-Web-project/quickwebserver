#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define RATTUS_LOGGING 0


typedef struct {
    char *buffer;
    unsigned long size;
} FileData;

typedef struct {
    char *data;
    size_t capacity;
    size_t size;
} string_array;

typedef struct {
    size_t *data;
    size_t capacity;
    size_t size;
} size_t_array;

typedef struct {
  char *key;
  char *value;
} key_value_t;

typedef struct {
  key_value_t *data;
  size_t capacity;
  size_t size;
} key_value_array;

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

void slice_str(const char *str, char *buffer, size_t start, size_t end) {
    size_t j = 0;
    for ( size_t i = start; i <= end; ++i ) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}

char *getMultipartFile(const char *buffer, size_t buf_length, char *boundary, char *boundary_closing) {
  char *filename;

  char *ptr = strtok((char *)buffer, "\r\n");
  short content_next = 0;
  while(ptr != NULL)
	{
    char *filenameString = strstr(ptr, "filename");
    if (filenameString) {
      char *filename_temp = cut_string(filenameString, "filename=\"", '"');
      filename = malloc(strlen(filename_temp));
      strcpy(filename, filename_temp);
      free(filename_temp);
    }
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

  FILE *resFile = fopen(filename, "wb");
  for (int i = 2; i < file_end ; i++) {
    fputc(ptr[i], resFile);
  }
  fclose(resFile);
  
  return filename;
}

key_value_array *parseMultipartBody(const char *buffer, size_t buf_length, const char *content_type) {
  char *boundary = cut_string(content_type, "boundary=", '\0');

  char b_start[strlen(boundary) + 3];
  snprintf(b_start, sizeof b_start, "%s%s", "--", boundary);
  char *b_start_p = b_start;

  size_t_array *content_beginnings;
  size_t_array *content_endings; 

  ARRAY_CREATE(content_beginnings, 2, 0);
  ARRAY_CREATE(content_endings, 2, 0);

  size_t loop_idx = 0;
  size_t boundary_start = 0;
  while (loop_idx <= buf_length)
  {
    char byte_v = buffer[loop_idx];

    if (boundary_start == 0 && byte_v == b_start_p[0]) {
      boundary_start = loop_idx;
    }

    int pos = loop_idx - boundary_start;

    if (pos <= strlen(b_start_p) && b_start_p[pos] && b_start_p[pos] == byte_v) {
      if (loop_idx - boundary_start == strlen(b_start_p) - 1) {
        if (boundary_start != 0) ARRAY_PUSH(content_endings, boundary_start + strlen(b_start_p));
        ARRAY_PUSH(content_beginnings, loop_idx + 3);
      }
    } else {
      boundary_start = 0;
    }
    loop_idx++;
  }

  key_value_array *body_arr;
  ARRAY_CREATE(body_arr, 2, 0);

  for (int i = 0; i < content_endings->size; i++) {
    size_t contentLength = content_endings->data[i] - content_beginnings->data[i];
    char *contentBlock = malloc(contentLength);
    slice_str(buffer, contentBlock, content_beginnings->data[i], content_endings->data[i]);
    if (strstr(contentBlock, "filename")) {
      char fkey[10];
      snprintf(fkey, sizeof fkey, "%s%d", "file", i);

      key_value_t file_object; 
      file_object.key = malloc(strlen(fkey));
      strcpy(file_object.key, fkey);
      file_object.value = getMultipartFile(contentBlock, contentLength, boundary, b_start_p);
      file_object.key[strlen(file_object.key)] = '\0';
      ARRAY_PUSH(body_arr, file_object);
    } else {
      char *key = cut_string(contentBlock, "name=\"", '"');
      key[strlen(key)] = '\0';
      char *value = strtok(contentBlock, "\n");
      while (1)
      {
        if (value[0] == 13 || value[0] == 10) {
          value = strtok(NULL, "\n");
          break;
        }
        value = strtok(NULL, "\n");
      }
      
      value[strlen(value) - 1] = '\0';

      key_value_t field_object;
      field_object.key = key;
      field_object.value = value;
      ARRAY_PUSH(body_arr, field_object);
    }
    free(contentBlock);
  }

  return body_arr;
}

void print_log(const char *message) {
  if (RATTUS_LOGGING) {
    printf("%s", message);
  }
}
