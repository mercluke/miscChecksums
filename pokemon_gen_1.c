#include <stdio.h>
#include <stdlib.h>

struct Variant {
    int filesize;
    int offset;
    int checksum;
};

struct Variant US = {0x8000, 0x2598, 0x3523};
struct Variant JP = {0x8000, 0x2598, 0x3594};

int main(int argc, char** argv) {

if (argc != 3) {
	printf("supply a filename pls\n");
	return -1;
}

struct Variant variant;
char* game = argv[1];
char* fname = argv[2];

switch(game[0]) {
    case 'J':
    case 'j':
        variant = JP;
        break;
    case 'U':
    case 'u':
        variant = US;
        break;
    default:
        printf("Unknown variant: '%s'", game);
        return -1; 
}


FILE* sav = fopen(fname, "r");
char* buffer = malloc(variant.filesize);
fread(buffer, 1, variant.filesize, sav);
fclose(sav);
char sum = 0;

for(int i = variant.offset; i < variant.checksum; i++) {
	sum += buffer[i];
}
printf("sum: %02x\n", (~sum) & 0xFF);

buffer[variant.checksum] = (~sum) & 0xFF;

sav = fopen(fname, "wb");
fwrite(buffer, 1, variant.filesize, sav);
fclose(sav);
free(buffer); 

return 0;
}
