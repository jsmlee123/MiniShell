#ifndef HEADER_FILE
#define HEADER_FILE

/*
 * A custom struct used to store tokens.
 * size -> the number of tokens in our struct
 * strings -> the array of tokens in our struct
 *
 * struct strarray->strarrray_t
 */
typedef struct strarray {
  int size;
  char **strings;
} strarray_t;

strarray_t *duplicateStrArr(strarray_t *input);
strarray_t *parse(char *str);
void freeStrArr(strarray_t *tokens);

#endif