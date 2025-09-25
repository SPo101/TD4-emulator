#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main() {
	size_t size = 0;
	char *str = NULL;
	printf("Enter opcode in hex format: ");
	getline(&str, &size, stdin);

    FILE *f = fopen("opcode.bin", "wb");

    if(f == NULL){
    	perror("Error with opening file:");
    	exit(1);
    }

    const char *p = str;
    unsigned char c;
    while (*p && *(p+1)) {
        sscanf(p, "%2hhx", &c);      // read 2 hex digits as a byte
        fwrite(&c, 1, 1, f);          // write byte to file
        p += 2;

  	}
  	printf("Bin file was generated.\n");
    fclose(f);
}
