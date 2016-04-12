#define DEBUG 1

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 3363
#define BACKLOG 2

#define BA6X_BYTES 29
#define BA6X_LEN 32

#define BA6X_COL 5
#define BA6X_LINE 7

#define MAX_STR 255
#define BA63_VID 0x0aa7
#define BA63_PID 0x0200
#define MAX_CLIENT 0xa

#define UNKNOWN_COMMAND "Unknown"
#define SUCCESS_COMMAND "Success"
#define FAILED_PARAMS "WrongParams"
#define NB_PARAMS 5

extern hid_device *display;
extern pthread_mutex_t displayLock;
extern unsigned char buffer[BA6X_LEN];
extern const unsigned char SEQ_CLEAR[5];
extern const unsigned char SEQ_FR[4];
extern const unsigned char SEQ_CHARSET[4];
extern unsigned char SEQ_CURSOR[7];

void prepareBuffer(unsigned char *sequence, unsigned char size);
void sendBuffer(hid_device *display, unsigned char *sequence);
void setCursor(hid_device *display, unsigned short x, unsigned short y);
int initializeDevice();
int freeDevice();

int extractParams(unsigned char *src, unsigned char **dst);
void replace(unsigned char *src, unsigned char occ, unsigned char new);
