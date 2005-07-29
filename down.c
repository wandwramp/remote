#include <stdio.h>

int main(int argc, char *argv[])
{
  FILE *file;
  char line[132]; 
  int lines;
  int ch;          /*Yep it's an int.  See man getchar*/
  
  if (argc < 2) {
    fprintf(stderr, "Please specify a filename to upload.\n");
    return -1;
  }

  file = fopen(argv[1], "r");
  if ( file == NULL ) {
    fprintf(stderr, "Could not open file: %s\n", argv[1]);
    return -1;
    //perror(argv[0]);
  }
  
  fprintf(stderr, "Uploading file: %s\n", argv[1]);
  
  lines = 0;
  while ( NULL != fgets(line, 132, file) )
    {
      printf("%s", line);
      ++lines;
      fflush(NULL);

      if ((ch = getchar()) != '.') {
	char old1 = '\0', old2 = '\0', old3;
	fflush(NULL);
	
	old3 = ch;

	while (!(old1 == ' ' && old2 == '>' && old3 == ' ')) {
	  if (old1 != '\0')
	    fprintf(stderr, "%c", old1);

	  old1 = old2;
	  old2 = old3;
	  old3 = getchar();
	}
	printf("\n");
	
	//fprintf(stderr, "\nError sending file, received '%c'.\n", ch);
	return -1;
      }

      fprintf(stderr, "%c", ch);
      fflush(NULL);
    }  
  fprintf(stderr, "\n%s\n", fgets(line, 132, stdin));
  fprintf(stderr, "%s", fgets(line, 132, stdin));
  printf("\n");

  return 0;
}





