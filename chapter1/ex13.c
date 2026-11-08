#include <stdio.h>

// histogram of word lengths in input
int main()
{
  int c, i, current_word_length;
  int lengths[100] = {};

  current_word_length = 0;

  while ((c = getchar()) != EOF)
  {
    if (c == ' ' || c == '\n')
    {
      ++lengths[current_word_length];
      current_word_length = 0;
    }
    else
      ++current_word_length;
  }
  // count last word before EOF
  ++lengths[current_word_length];

  for (i = 0; i < 10; i++)
  {
    printf("lengths[%d]: %d\n", i, lengths[i]);
  }
}