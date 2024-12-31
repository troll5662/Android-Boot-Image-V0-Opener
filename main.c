#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BOOT_MAGIC_VAL "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#define BOOT_ID_SIZE 8
#define BOOT_EXTRA_ARGS_SIZE 1024

const char texts[10][18] = {
	"Kernel Size:    \0",
	"Kernel Address: \0",
	"Ramdisk Size:   \0",
	"Ramdisk Address:\0",
	"Second Size:    \0",
	"Second Address: \0",
	"Tags Address:   \0",
	"Page Size:      \0",
	"Unused:         \0",
	"OS Version:     \0"
};

int main(int argc, char *argv[]) {
	fprintf(stdout, "\n");
	int pagesize = 1;
	int kernel_size, ramdisk_size = 0;
	FILE *image;
	image = fopen(argv[1], "r");
	if(image == NULL) {
		fprintf(stderr, "Failed to open file: %s\n", argv[1]);
		return -1;
	}

	// Check BootMagic
	{
		uint8_t boot_magic[BOOT_MAGIC_SIZE + 1] = { 0x00 };

		fread(boot_magic, sizeof(uint8_t), BOOT_MAGIC_SIZE, image);
		if (strcmp(boot_magic, BOOT_MAGIC_VAL) != 0) {
			fprintf(stderr, "Invalid BootMagic. Expected \"ANDROID!\" got \"%s\"\n", boot_magic);
			return -1;
		}
		fprintf(stdout, "BootMagic check succesful. Begining extraction.\n\n");
	}

	// Read Simple Values and assign shit
	{
		uint32_t read_val;
		for (int i = 0; i < 10; i++) {
			fread(&read_val, sizeof(uint32_t), 1, image);
			fprintf(stdout, "%s 0x%08X  (%u)\n", texts[i] , read_val, read_val);
			switch (i) {
				case 0:
					kernel_size = read_val;
					break;
				case 2:
					ramdisk_size = read_val;
					break;
				case 7:
					pagesize = read_val;
					break;
				default:
					break;
			}
		}
	}

	// Read Harder To Read Values (boot name, commandline, id values, extra cmdline)
	{
		uint8_t boot_name[BOOT_NAME_SIZE] = { 0x00 };
		fread(boot_name, sizeof(uint8_t), BOOT_NAME_SIZE, image);
		fprintf(stdout, "Name:            \"%s\"\n", boot_name);
	}
	{
		uint8_t cmdline[BOOT_ARGS_SIZE] = { 0x00 };
		fread(cmdline, sizeof(uint8_t), BOOT_ARGS_SIZE, image);
		fprintf(stdout, "Commandline:     \"%s\"\n", cmdline);
	}
	{
		uint32_t id[BOOT_ID_SIZE] = { 0x00 };
		fread(id, sizeof(uint32_t), BOOT_ID_SIZE, image);
		for (int i = 0; i < BOOT_ID_SIZE; i++) {
			fprintf(stdout, "ID[%d]:           0x%08X  (%u)\n", i, id[i], id[i]);
		}

	}
	{
		uint8_t extra_cmdline[BOOT_EXTRA_ARGS_SIZE] = { 0x00 };
		fread(extra_cmdline, sizeof(uint8_t), BOOT_EXTRA_ARGS_SIZE, image);
		fprintf(stdout, "Extra Cmdline:   \"%s\"\n", extra_cmdline);
	}

	fprintf(stdout, "\n");

	// here we extract shit from the image
	rewind(image);
	uint8_t *buffer = calloc(pagesize, sizeof(uint8_t));
	// header
	{
		FILE *header = fopen("header", "w");
		fread(buffer, pagesize, 1, image);
		fwrite(buffer, pagesize, 1, header);
		fprintf(stdout, "Wrote header\n");
		fclose(header);
	}
	{
		FILE *kernel = fopen("kernel", "w");
		int pages = ceil((double) kernel_size / (double) pagesize);
		for (int i = 0; i < pages; i++) {
			fread(buffer, pagesize, 1, image);
			fwrite(buffer, pagesize, 1, kernel);
		}
		fprintf(stdout, "Wrote kernel\n");
		fclose(kernel);
	}
	{
		FILE *ramdisk = fopen("ramdisk", "w");
		int pages = ceil((double) ramdisk_size / (double) pagesize);
		for (int i = 0; i < pages; i++) {
			fread(buffer, pagesize, 1, image);
			fwrite(buffer, pagesize, 1, ramdisk);
		}
		fprintf(stdout, "Wrote ramdisk\n");
		fclose(ramdisk);
	}
	{
		FILE *qcdt = fopen("qcdt", "w");
		fread(buffer, pagesize, 1, image);
		fwrite(buffer, pagesize, 1, qcdt);
		fprintf(stdout, "Wrote qcdt\n");
		fclose(qcdt);
	}
	{
		FILE *dt = fopen("dt", "w");
		while(fread(buffer, pagesize, 1, image) != 0) {
			fwrite(buffer, pagesize, 1, dt);
		}
		fprintf(stdout, "Wrote dt\n");
		fclose(dt);
	}

	fprintf(stdout, "\n");

	fclose(image);
	fprintf(stdout, "closed image\n");

	fprintf(stdout, "\n");
	
	return 0;
}