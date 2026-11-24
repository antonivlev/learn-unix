/*

:a Wash dishes
:a Do something

:l
===>
1. Wash dishes
2. Do something

:r 1

:l
===>
1. Do something

*/
#include <stdio.h>

#define BUF_SIZE 100
#define NUM_ROWS 60

int readline(char buf[])
{
  int c, i;
  i = 0;

  while ((c = getchar()) != EOF && c != '\n')
  {
    buf[i] = c;
    ++i;
  }
  buf[i] = '\0';

  return i;
}

typedef enum
{
  ADD,
  REMOVE,
  LIST
} Operation;

Operation get_opcode(char line[])
{
  if (line[0] != ':')
  {
    return 0;
  }

  char c;
  switch ((c = line[1]))
  {
  case 'a':
    return ADD;
  case 'r':
    return REMOVE;
  case 'l':
    return LIST;
  }

  return 0;
}

void nullify(char buf[])
{
  int i;
  for (i = 0; i < BUF_SIZE; ++i)
  {
    buf[i] = '\0';
  }
}

void nullify_todos(char todos[][BUF_SIZE])
{
  int i;
  for (i = 0; i < NUM_ROWS; ++i)
  {
    nullify(todos[i]);
  }
}

void copy(const source[], char dest[])
{
  int i;
  for (i = 0; i < BUF_SIZE; ++i)
  {
    dest[i] = source[i];
  }
}

void fill_content(char buf[], char line[])
{
  nullify(buf);
  // :a hello
  int i = 3;
  char c;

  while ((c = line[i]) != '\0')
  {
    buf[i - 3] = line[i];
    ++i;
  }
  ++i;
  buf[i] = '\0';
}

void add_todo(char content[], char todos[NUM_ROWS][BUF_SIZE])
{
  int i;

  for (i = 0; i < NUM_ROWS; i++)
  {
    if (todos[i][0] == '\0')
    {
      break;
    }
  }

  copy(content, todos[i]);
}

void print_todos(char todos[NUM_ROWS][BUF_SIZE])
{
  int i;

  for (i = 0; i < NUM_ROWS; i++)
  {
    if (todos[i][0] != '\0')
    {
      printf("%d. %s\n", i + 1, todos[i]);
    }
  }
}

int main()
{
  int n, opcode;

  char line[BUF_SIZE], content[BUF_SIZE];

  char todos[NUM_ROWS][BUF_SIZE];

  nullify_todos(todos);

  while ((n = readline(line)) > 0)
  {
    Operation op = get_opcode(line);
    if (op == ADD)
    {
      fill_content(content, line);
      add_todo(content, todos);
    }

    if (op == LIST)
    {
      print_todos(todos);
    }
  }
}
