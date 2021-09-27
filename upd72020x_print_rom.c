#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

static const uint16_t crc_table[16] = {
	0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387, // 0 - 7
	0x8408, 0x9489, 0xA50A, 0xB58B, 0xC60C, 0xD68D, 0xE70E, 0xF78F, // 8 - 15
};

static uint16_t update_crc16(uint16_t crc, const uint8_t *t, int len)
{
	int n;
	uint16_t c = crc ^ 0xffffU;
	uint8_t m;
	for(n = 0; n < len; n++) {
		m = t[n];
		c = crc_table[(c ^ m) & 0xf] ^ (c >> 4);
		c = crc_table[(c ^ (m >> 4)) & 0xf] ^ (c >> 4);
	}

	return c ^ 0xffffU;
}

static uint32_t vscd_data32(const uint16_t *vscd)
{
	const uint8_t *p = (const uint8_t *)vscd;
	return p[1] | p[3] << 8 | p[5] << 16 | p[7] << 24;
}

static uint16_t vscd_data16(const uint16_t *vscd)
{
	const uint8_t *p = (const uint8_t *)vscd;
	return p[1] | p[3] << 8;
}

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

	uint16_t vscd_len = buf[0];
	printf("Length: %u (0x%04x)\n", vscd_len, vscd_len);
	uint16_t calc_crc = update_crc16(0, (uint8_t *)buf, vscd_len + 2);
	uint16_t img_crc = buf[vscd_len/2 + 1];
	if (calc_crc != img_crc) {
		fprintf(stderr, "CRC missmatch: img=0x%04x, calc=0x%04x\n",
		        img_crc, calc_crc);
		exit(EXIT_FAILURE);
	}
	printf("vscd crc: 0x%04x\n", img_crc);

	uint8_t *p = (uint8_t *)&buf[1];
	for (unsigned int i = 0; i < vscd_len; i = i+2) {
		printf("%2d: 0x%02x: 0x%02x\n", i, p[i], p[i+1]);
	}

	printf("Subsystem Vendor ID:       0x%04x\n", vscd_data16(&buf[1]));
	printf("Subsystem ID:              0x%04x\n", vscd_data16(&buf[3]));
	printf("PHY Control 0:         0x%08x\n", vscd_data32(&buf[5]));
	printf("PHY Control 1:         0x%08x\n", vscd_data32(&buf[9]));
	printf("PHY Control 2:         0x%08x\n", vscd_data32(&buf[13]));
	printf("Host Controller Conf.: 0x%08x\n", vscd_data32(&buf[17]));

	uint16_t *fw = &buf[vscd_len/2 + 2];
	if (0x55aa != fw[0]) {
		fprintf(stderr, "No Firmware Found (Header Code @0x%x) 0x%04x\n",
		        vscd_len + 4, fw[0]);
		exit(EXIT_FAILURE);
	}
	printf("FW Version: 0x%04x (@0x%04x)\n", fw[fw[2] / 2], fw[2]);

	return 0;
}
