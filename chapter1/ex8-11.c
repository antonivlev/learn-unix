#include <stdio.h>

// fix double spaces, replace characters
int main()
{
  int c;
  int num_consec_spaces;

  while ((c = getchar()) != EOF)
  {
    if (c == ' ')
      ++num_consec_spaces;
    else
      num_consec_spaces = 0;

    if (c == ' ' && num_consec_spaces > 1)
      continue;

    if (c == '\t')
    {
      putchar('\\');
      putchar('t');
    }
    else
      putchar(c);
  }
}