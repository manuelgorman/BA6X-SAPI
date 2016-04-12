#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include<pthread.h>

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
const unsigned char SEQ_CURSOR[7] = {0x1B, 0x5B, 0x31, 0x3B, 0x31, 0x48, 0x00};

void prepareBuffer(unsigned char *sequence, unsigned char size) {
  
  if (size > BA6X_BYTES) {
    fprintf(stderr, "<Pilote> Impossible de transmettre une sequence > 29 bytes!\n");
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

void sendBuffer(hid_device *display, unsigned char *sequence) {
  prepareBuffer(sequence, strlen(sequence));
  hid_write(display, buffer, BA6X_LEN);
}

int initializeDevice() {
  hid_init();
  display = hid_open(0x0aa7, 0x0200, NULL);
  hid_get_product_string(display, moduleName, MAX_STR);
  if (DEBUG) fprintf(stdout, "<Info> Connected to \"%ls\" display.\n", moduleName);
  return display!=NULL;
}

int freeDevice() {
  hid_close(display);
  return hid_exit();
}
