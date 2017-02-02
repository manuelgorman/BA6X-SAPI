#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <wchar.h>

#include "../include/hidapi.h"
#include "../include/common.h"

wchar_t moduleName[MAX_STR];

hid_device *display = NULL;
pthread_mutex_t displayLock;

unsigned char buffer[BA6X_LEN] = {
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00
};

const unsigned char SEQ_CLEAR[5] = {0x1B, 0x5B, 0x32, 0x4A, 0x00};
const unsigned char SEQ_FR[4] = {0x1B, 0x52, 0x01, 0x00};
const unsigned char SEQ_CHARSET[4] = {0x1B, 0x52, 0x31, 0x00};
unsigned char SEQ_CURSOR[7] = {0x1B, 0x5B, 0x31, 0x3B, 0x31, 0x48, 0x00};

void prepareBuffer(const unsigned char *sequence, unsigned char size) {

  if (size > BA6X_BYTES) {
    fprintf(stderr, "<Driver> Can not transmit a sequence> 29 bytes!\n");
    return;
  }

  int i = 0;

  memset(buffer, 0x00, BA6X_LEN);

  buffer[0] = 0x02;
  buffer[2] = size;

  for (i = 3; i < size+3; i++) {
    buffer[i] = sequence[i-3];
  }
}

void sendBuffer(hid_device *display, const unsigned char *sequence) {
  prepareBuffer(sequence, strlen(sequence));
  hid_write(display, buffer, BA6X_LEN);
}

void setCursor(hid_device *display, unsigned short x, unsigned short y) {
  SEQ_CURSOR[2] = 0x30 + x;
  SEQ_CURSOR[4] = 0x30 + y;
  sendBuffer(display, SEQ_CURSOR);
}

int initializeDevice() {

  struct hid_device_info *devs, *cur_dev;
  const wchar_t targetManufacturer[] = L"Wincor Nixdorf";

  hid_init();
  /* Auto-detection du périphérique */
  //Wincor Nixdorf

	devs = hid_enumerate(0x0, 0x0);
	cur_dev = devs;

	while (cur_dev) {
    if (DEBUG) fprintf(stdout, "<Debug> Peripherals found: %04hx %04hx - %ls - %s\n", cur_dev->vendor_id, cur_dev->product_id, cur_dev->manufacturer_string ,cur_dev->path);

    #if defined(__APPLE__)
      if (!wcscmp(targetManufacturer, cur_dev->manufacturer_string) ) {
        fprintf(stdout, "<Driver> USB Display: %04hx %04hx on port %s..\n", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path);
        display = hid_open(cur_dev->vendor_id, cur_dev->product_id, NULL);
        break;
      }
    #elif defined(linux)
      if (!wcscmp(targetManufacturer, cur_dev->manufacturer_string) &&  !strcmp(cur_dev->path+(strlen(cur_dev->path)-2), "01")) {
        fprintf(stdout, "<Pilote> USB Display %04hx %04hx on port %s..\n", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path);
        display = hid_open_path(cur_dev->path);
        break;
      }
    #else
      fprintf(stderr, "<Fatal> Device not supported\n");
      exit(-1);
    #endif

		cur_dev = cur_dev->next;
	}

	hid_free_enumeration(devs);
  if (!display) return 0;

  hid_get_product_string(display, moduleName, MAX_STR);
  if (DEBUG) fprintf(stdout, "<Info> Connected to \"%ls\" display.\n", moduleName);
  return display!=NULL;
}

int freeDevice() {
  hid_close(display);
  return hid_exit();
}
