#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
	int fd;
	if (argc != 2) {
		fprintf(stderr, "USAGE %s <file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Can't open file `%s': %s\n", argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	};

	uint16_t buf[64 * 1024];
	read(fd, buf, sizeof(buf));
	if (buf[0] != 0x55aa) {
		fprintf(stderr, "Wrong header Code: 0x%04x\n", buf[0]);
		exit(EXIT_FAILURE);
	}

	printf("FW Ver Pointer: 0x%04x\n", buf[2]);
	printf("FW Version: 0x%04x\n", buf[buf[2]/sizeof(buf[0])]);
	return 0;
}
