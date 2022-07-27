#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "parse.h"

/*
 * Return a deep copy of a given strarray_t struct.
 * Not that it does not free the input struct
 *
 * strarray_t *input - strarray_t to be copied
 * Return: strarray_t deep copy
 */

strarray_t *duplicateStrArr(strarray_t *input) {
  strarray_t *new = (strarray_t *)malloc(sizeof(strarray_t));
  new->size = input->size;
  char **temparr = (char **)malloc(sizeof(char *) * input->size);
  for (int i = 0; i < input->size; i++) {
    temparr[i] = (char *)malloc(sizeof(char) * (strlen(input->strings[i]) + 1));
    strcpy(temparr[i], input->strings[i]);
  }
  new->strings = temparr;
  return new;
}

/*
 * A function that parses a given c string and converts
 * it into a strarray_t with tokens separated by a space.
 *
 * char *str - c string to be parsed by spaces
 * Return: strarray_t with parsed tokens of str
 */
strarray_t *parse(char *str) {
  int str_len = strlen(str) + 1;
  char strcopy[str_len];
  char strcopy2[str_len];

  strcpy(strcopy, str);
  strcpy(strcopy2, str);

  int wordCount = 0;
  char *temp;
  temp = strtok(strcopy, " ");
  // get size of token array first
  while (temp != NULL) {
    temp = strtok(NULL, " ");
    wordCount += 1;
  }

  // init token array given word count
  char **result = (char **)malloc(sizeof(char *) * wordCount);
  char *pch;
  pch = strtok(strcopy2, " ");
  int index = 0;

  // assign each element of token array to corresponding token
  while (pch != NULL && index < wordCount) {
    result[index] = (char *)malloc(sizeof(char) * (strlen(pch) + 1));
    strcpy(result[index], pch);

    result[index][strlen(pch)] = '\0';
    pch = strtok(NULL, " ");
    index += 1;
  }

  // init strarray_t to return
  strarray_t *ret = (strarray_t *)malloc(sizeof(strarray_t));
  ret->size = wordCount;
  ret->strings = result;
  return ret;
}
/*
 * A function that is used to free memory from strarray_t structs.
 *
 * strarray_t *tokens - strarray_t to be freed
 */
void freeStrArr(strarray_t *tokens) {
  if (tokens->size == 0) {
    free(tokens);
    return;
  }
  int maxSize = tokens->size;
  for (int i = 0; i < maxSize; i++) {
    free(tokens->strings[i]);
  }

  free(tokens->strings);
  free(tokens);
}
