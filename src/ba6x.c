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

// Byte combinations for each display command
const unsigned char SEQ_CLEAR[4] = {0x1B, 0x5B, 0x32, 0x4A}; //Clear screen
const unsigned char SEQ_FR[4] = {0x1B, 0x52, 0x01, 0x00}; //?????
const unsigned char SEQ_CHARSET[4] = {0x1B, 0x52, 0x31, 0x03}; //character set
unsigned char SEQ_CURSOR[7] = {0x1B, 0x5B, 0x31, 0x3B, 0x31, 0x48, 0x00}; //Command to set cursor pos - index 2 is x, 4 is y

void prepareBuffer(const unsigned char *sequence, unsigned char size) { //Why do you need a seperate function to preprare the buffer!? And why doesn't it return anything?
  /*  Checks if sequence of bytes is correct length,
   *  allocates memory for the buffer variable
   *  and prepends the "Write command" instruction bytes
   */
     if (size > BA6X_BYTES) {
    fprintf(stderr, "<Driver> Can not transmit a sequence> 29 bytes!\n");
    return; //The screen only accepts 32 bytes at a time,
  }

  int i = 0;

  memset(buffer, 0x00, BA6X_LEN); // Allocates 32 bytes of memory, set all to 0

  buffer[0] = 0x02; //0x02 0x00 for "write" command
  buffer[2] = size; //3rd byte is size of data we are sending

  for (i = 3; i < size+3; i++) {
    buffer[i] = sequence[i-3]; //Presumably this adds the characters you want to display into the bytes to be sent to the display
  }
}

void sendBuffer(hid_device *display, const unsigned char *sequence) {  //Pretty simple, 'prepares' the buffer to be send then writes it to the HID device
  prepareBuffer(sequence, strlen(sequence));
  hid_write(display, buffer, BA6X_LEN); //Okay so buffer is a global variable. That's FINE. At least the hid library does the complicated stuff for me.
}

void setCursor(hid_device *display, unsigned short x, unsigned short y) { //This function sends the sequence of bytes to change the cursor position
  SEQ_CURSOR[2] = 0x30 + x; //2 bytes for x
  SEQ_CURSOR[4] = 0x30 + y; //2 for y (why!? it's a 4 high screen at maximum!)
  sendBuffer(display, SEQ_CURSOR);
}

int initializeDevice() { //The funky stuff

  struct hid_device_info *devs, *cur_dev; //I think this gets or sets some stuff in the hidapi
  const wchar_t targetManufacturer[] = L"Wincor Nixdorf"; //Almost definitely setting some stuff in the hidapi

  hid_init(); //"finalizes the hid api"
  /* Auto-detection du périphérique */
  //Wincor Nixdorf

	devs = hid_enumerate(0x0, 0x0); //I should read the documentation on hidapi
	cur_dev = devs;

	while (cur_dev) {
    if (DEBUG) fprintf(stdout, "<Debug> Peripherals found: %04hx %04hx - %ls - %s\n", cur_dev->vendor_id, cur_dev->product_id, cur_dev->manufacturer_string ,cur_dev->path);

    #if defined(__APPLE__)
      if (!wcscmp(targetManufacturer, cur_dev->manufacturer_string) ) {
        fprintf(stdout, "<Driver - M> USB Display: %04hx %04hx on port %s..\n", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path);
        display = hid_open(cur_dev->vendor_id, cur_dev->product_id, NULL);
        break;
      }
    #elif defined(linux)
      if (!wcscmp(targetManufacturer, cur_dev->manufacturer_string) &&  !strcmp(cur_dev->path+(strlen(cur_dev->path)-2), "01")) {
        fprintf(stdout, "<Driver - L> USB Display %04hx %04hx on port %s..\n", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path);
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
