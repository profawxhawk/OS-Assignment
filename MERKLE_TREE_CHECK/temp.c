#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
  char str[] ="- This, a sample string.";
  char const* const fileName = argv[1];
  char * pch;
  FILE* file = fopen(fileName, "r");
  char line[256];
  // fgets(line, sizeof(line), file);
  
  while(fgets(line, sizeof(line), file)){
    pch = strtok (line," ");
    while (pch != NULL)
        {
            // printf("%d\n", strlen(pch));
            if (strcmp("\n",pch) == 0){
                printf("%s\n", "same");
            }else{
                printf ("%s\n",pch);
            }
            pch = strtok (NULL, " ");
        }
  }
  return 0;
    /*
    char const* const fileName = argv[1];
    FILE* file = fopen(fileName, "r");
    char line[256];
    char* pch;
    char* a;
    while (fgets(line, sizeof(line), file)) {
        pch = strtok (line," ");
        while (pch != NULL)
        {
            //printf ("%s\n",pch);
            printf("Enter a string\n");
            gets(a);
            if (strcmp(a,pch) == 0){
                printf("%s\n", "same");
            }
            pch = strtok (NULL, " ");
            
        } 
    }
    

    fclose(file);
*/
    return 0;
}