#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Covers regions 0x44 through to 0x3a37
// bytes 0x16 -> 0x43 don't seem to be covered by checksum at all
#define REGION_0 {0x0044, 0x0020, 0x04}
#define REGION_1 {0x0064, 0x0760, 0x06}
#define REGION_2 {0x07C4, 0x07C0, 0x08}
#define REGION_3 {0x0F84, 0x1B5C, 0x0A}
#define REGION_4 {0x2AE0, 0x04B0, 0x0C}
#define REGION_5 {0x2F90, 0x0940, 0x0E}
#define REGION_6 {0x38D0, 0x0090, 0x10}
#define REGION_7 {0x3960, 0x0010, 0x12}
#define REGION_8 {0x3970, 0x00C8, 0x14}
#define ALL_REGIONS {REGION_0, REGION_1, REGION_2, REGION_3, REGION_4, REGION_5, REGION_6, REGION_7, REGION_8}
#define NUM_REGIONS (9)

// first word of madden save file
#define HEADER ((uint32_t)0x66EEDBBA)
// checksum multiplier varies once per 4 bytes
#define ADDRESS_MASK (0xFFFFFFFC)
// magic number to initialise checksum -> region of all zero would have checksum 0x13
#define CHECKSUM_INIT (0x13)
#define FILESIZE (0x40000)

struct Region
{
	uint32_t START_OFFSET;
	uint32_t SIZE;
	uint32_t CHECKSUM_ADDRESS;
};

void usage(char *exe)
{
	printf("usage: %s sav_filename [output]\n", exe);
}

uint16_t get_diff(struct Region region, uint32_t addr)
{
	uint16_t diff;
	addr &= ADDRESS_MASK;
	return region.SIZE - (addr - region.START_OFFSET);
}

uint16_t swap_endian(uint16_t shrt)
{
	return (shrt >> 8) | ((shrt << 8) & 0xff00);
}

uint16_t read_short(uint8_t *buffer, uint32_t addr)
{
	return (((uint16_t)buffer[addr]) << 8) | ((uint16_t)buffer[addr+1]);
}

void write_short(uint8_t *buffer, uint32_t addr, uint16_t val)
{
	buffer[addr] = (uint8_t)(val >> 8);
	buffer[addr+1] = (uint8_t)val;
}

void write_word(uint8_t *buffer, uint32_t addr, uint32_t val) {
	write_short(buffer, addr, (uint16_t)(val >> 16));
	write_short(buffer, addr+2, (uint16_t)(val));
}

int find_expected_sum(uint8_t *buffer, struct Region region, int verbose, int overwrite)
{
	uint32_t addr;
	uint16_t current_checksum;
	uint16_t expected_checksum = CHECKSUM_INIT;
	uint32_t END_OFFSET = region.START_OFFSET + region.SIZE;

	current_checksum = read_short(buffer, region.CHECKSUM_ADDRESS);

	if (verbose)
	{
		printf("current checksum: %hx\n", current_checksum);
	}

	for (addr = region.START_OFFSET; addr < END_OFFSET; addr++)
	{
		expected_checksum += get_diff(region, addr) * (uint16_t)buffer[addr];
	}
	expected_checksum = swap_endian(expected_checksum);

	if (verbose)
	{
		printf("expected_checksum: %hx\n", expected_checksum);
	}
	if (overwrite)
	{
		write_short(buffer, region.CHECKSUM_ADDRESS, expected_checksum);
	}

	return expected_checksum == current_checksum;
}

void find_length(uint8_t *buffer, uint32_t known_start, uint16_t checksum_adress)
{
	struct Region foundRegion = {known_start, 0, 0x16};
	uint32_t foundIt = 0;
	uint32_t max_possible_size = FILESIZE - checksum_adress;

	for (uint32_t i = 1; i < max_possible_size; i++)
	{
		foundRegion.SIZE = i;
		if (find_expected_sum(buffer, foundRegion, 0, 0))
		{
			printf("found suspected size: %x\n", foundRegion.SIZE);
			foundIt = i;
		}
		else if (foundIt)
		{
			break;
		}
	}
	printf("done finding\n");
}

int main(int argc, char **argv)
{
	char *output_fname;
	FILE *input, *output;
	uint8_t *buffer;
	uint32_t i;
	struct Region all_regions[] = ALL_REGIONS;

	if (argc < 2)
	{
		usage(argv[0]);
		return -1;
	}
	output_fname = (argc >= 3) ? argv[2] : argv[1];
	input = fopen(argv[1], "rb");

	if (!input)
	{
		printf("Unable to open '%s' for reading\n", argv[1]);
		usage(argv[0]);
		return -1;
	}

	buffer = (uint8_t *)malloc(FILESIZE);
	for (i = 0; i < FILESIZE; i++)
	{
		buffer[i] = (uint16_t)fgetc(input);
	}
	fclose(input);

	for (i = 0; i < NUM_REGIONS; i++)
	{
		find_expected_sum(buffer, all_regions[i], 1, 1);
	}
	write_word(buffer, 0x0, HEADER);
	output = fopen(output_fname, "wb");

	if (!input)
	{
		printf("Unable to open '%s' for writing\n", output_fname);
		usage(argv[0]);
		return -1;
	}

	for (i = 0; i < FILESIZE; i++)
	{
		fputc(buffer[i], output);
	}
	free(buffer);
	fclose(output);

	return 0;
}
