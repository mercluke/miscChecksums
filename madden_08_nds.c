#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Region {
    uint32_t START_OFFSET;
    uint32_t SIZE;
    uint32_t CHECKSUM_ADDRESS;
};


// Covers regions 0x44 through to 0x3a37
struct Region region_1 = {0x0044, 0x0020, 0x04};
struct Region region_2 = {0x0064, 0x0760, 0x06};
struct Region region_3 = {0x07C4, 0x07C0, 0x08};
struct Region region_4 = {0x0F84, 0x1B5C, 0x0A};
struct Region region_5 = {0x2AE0, 0x04B0, 0x0C};
struct Region region_6 = {0x2F90, 0x0940, 0x0E};
struct Region region_7 = {0x38D0, 0x0090, 0x10};
struct Region region_8 = {0x3960, 0x0010, 0x12};
struct Region region_9 = {0x3970, 0x00C8, 0x14};

// bytes 0x16 -> 0x43 don't seem to be covered by checksum at all

/*checksum multiplier varies once per 4 bytes*/
#define ADDRESS_MASK (0xFFFFFFFC)
#define CHECKSUM_INIT (0x13)

void usage(char* exe) {
	printf("usage: %s filename\n", exe);
}

uint16_t getDiff(struct Region region, uint32_t addr) {
	uint16_t diff;
	addr &= ADDRESS_MASK;
	return region.SIZE - (addr-region.START_OFFSET);
}

uint16_t swapEndian(uint16_t shrt) {
return (shrt>>8) | ((shrt<<8) & 0xff00);
}

int checkExpectedSum(FILE* input, struct Region region, int verbose) {
	uint32_t addr;
	uint16_t current_checksum;
	uint16_t expected_checksum = CHECKSUM_INIT;
	uint32_t END_OFFSET = region.START_OFFSET + region.SIZE;
	
	fseek(input, region.CHECKSUM_ADDRESS, SEEK_SET);
	current_checksum = (((uint16_t)fgetc(input)) << 8) | (uint16_t)fgetc(input);

	if(verbose) {
		printf("current checksum: %hx\n", current_checksum);
	}

	fseek(input, region.START_OFFSET, SEEK_SET);	
	for(addr = region.START_OFFSET; addr < END_OFFSET; addr++) {
		unsigned char val = fgetc(input);
		expected_checksum += getDiff(region, addr) * (uint16_t)val;
	}
	expected_checksum = swapEndian(expected_checksum);

	if(verbose) {
		printf("expected_checksum: %hx\n", expected_checksum);
	}

	return expected_checksum == current_checksum;
}

void findLength(FILE* input) {
	uint32_t known_start = 0x3A38;
	struct Region foundRegion = {known_start, 0, 0x16};
	uint32_t foundIt = 0;
	uint32_t MAX_POSSIBLE_SIZE = 0x40000 - known_start;

	for(uint32_t i = 1; i < MAX_POSSIBLE_SIZE; i++) {
		foundRegion.SIZE = i;
		if(checkExpectedSum(input, foundRegion, 0)) {
			printf("found suspected size: %x\n", foundRegion.SIZE);
			foundIt = i;
		} else {
			if(foundIt) {
				break;
			}
		}
	}
	printf("done finding\n");
}


int main (int argc, char** argv) {
	uint32_t addr;
	if (argc < 2) {
		usage(argv[0]);
		return -1;
	}
	FILE* input = fopen(argv[1], "rb");

	if(!input) {
		printf("Unable to open '%s' for reading\n", argv[1]);
		usage(argv[0]);
		return -1;
	}

	printf("Region 1:\n");
	checkExpectedSum(input, region_1, 1);
	printf("Region 2:\n");
	checkExpectedSum(input, region_2, 1);
	printf("Region 3:\n");
	checkExpectedSum(input, region_3, 1);
	printf("Region 4:\n");
	checkExpectedSum(input, region_4, 1);
	printf("Region 5:\n");
	checkExpectedSum(input, region_5, 1);
	printf("Region 6:\n");
	checkExpectedSum(input, region_6, 1);
	printf("Region 7:\n");
	checkExpectedSum(input, region_7, 1);
	printf("Region 8:\n");
	checkExpectedSum(input, region_8, 1);
	printf("Region 9:\n");
	checkExpectedSum(input, region_9, 1);

	//findLength(input);	


	fclose(input);

	return 0;
}
