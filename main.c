#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ctype.h>
#include <inttypes.h>


void memdump_show_usage()
{
	printf("Usage\n");
}

int base_scanf(const char *buf, int base, off_t *value)
{
	int ret = 0;

	switch (base) {
	case 10:
		ret = sscanf(buf, "%" PRIu64, value);
		break;
	case 16:
		ret = sscanf(buf, "%" PRIX64, value);
		break;
	case 8:
		ret = sscanf(buf, "%" PRIo64, value);
		break;
	default:
		fprintf(stderr, "Unknown base\n");
		break;
	}

	if (ret == EOF || !ret) {
		fprintf(stderr, "Couldn't parse number: %s\n", buf);
		return 1;
	}

	return 0;
}

int parse_input(const char *input, off_t *val)
{
	int base;

	if (input[0] == '0')
		if (input[1] == 'x' || input[1] == 'X')
			base = 16;
		else
			base = 8;
	else
		base = 10;

	return base_scanf(input, base, val);
}

int main(int argc, char **argv)
{
	void *map_base, *virt_addr;
	off_t target;
	size_t size;
	unsigned page_size, mapped_size, offset_in_page;
	int fd;
	int page_count;
	int i;

	page_size = getpagesize();
	/* ADDRESS */
	if (!argv[1] || !argv[2]) {
		memdump_show_usage();
		exit(EXIT_FAILURE);
	}
	if (parse_input(argv[1], &target))
	    exit(EXIT_FAILURE);

	printf("target: 0x%" PRIX64 "\n", target);

	/* SIZE */
	if (parse_input(argv[2], &size))
	    exit(EXIT_FAILURE);

	printf("size: 0x%" PRIX64 "\n", size);

	fd = open("/dev/mem", O_RDONLY | O_SYNC);
	if (!fd) {
		perror("Can't open /dev/mem");
		exit(EXIT_FAILURE);
	}

	page_count = size / page_size;
	if (size % page_size)
		page_count++;

	printf("Mapping %d pages\n",page_count);
	mapped_size = page_count * page_size;
	offset_in_page = (unsigned)target & (page_size - 1);
	if (offset_in_page + size > page_size) {
		/* This access spans pages.
		 * Must map two pages to make it possible: */
		mapped_size += getpagesize();
	}
	printf("Mapped size: %d\n", mapped_size);
	map_base = mmap(NULL,
			mapped_size,
			PROT_READ,
			MAP_SHARED,
			fd,
			target & ~(off_t)(page_size - 1));
	if (map_base == MAP_FAILED) {
		perror("Failed to map /dev/mem to memory\n");
		exit(EXIT_FAILURE);
	}

	virt_addr = (char*)map_base + offset_in_page;

	for (i = 0; i < size; i++) {
		if (i && (i % 16 == 0))
			printf("\n");
		printf("%02x ",*(volatile uint8_t*)(virt_addr + i));
	}

	printf("\n");

	if (munmap(map_base, mapped_size) == -1) {
		perror("Can't unmap memory");
		exit(EXIT_FAILURE);
	}
	close(fd);

	return EXIT_SUCCESS;
}
