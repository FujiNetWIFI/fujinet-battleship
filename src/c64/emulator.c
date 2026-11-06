/*
  Emulator network/appkey logic for testing in VICE
*/
#include <c64.h>
#include <cbm.h>
#include <stdlib.h>

#include <conio.h>
#include <stdbool.h>
#include <stdint.h>
#include "../misc.h"

#define MAX_APPKEY_LEN 64
#define APPKEY_READ  0
#define APPKEY_WRITE 1

/**
 * @brief Convenience type for handling an appkey
 */
typedef union {
    struct
    {
        unsigned int creator;
        unsigned char app;
        unsigned char key;
        unsigned char mode;
        unsigned char reserved;
    } open;
    struct
    {
        unsigned char length;
        unsigned char reserved;
        unsigned char value[MAX_APPKEY_LEN];

    } read;
    struct
    {
       char value[MAX_APPKEY_LEN+32];
    } write;
} appkeyblock;


typedef struct {
  unsigned char command;
  appkeyblock payload; // The appkey (see appkey.h)
} appkeyC64;

static appkeyC64 appkey;


#define N_LFN 2     // Network
#define LFN 15      // Fujinet
#define DEV 11
#define SEC_NUM 0
#define OPEN_APPKEY_PREFIX "appkey-"
#define OPEN_APPKEY_DELIM "-"

#define FUJICMD_OPEN_APPKEY 0xDC
#define FUJICMD_WRITE_APPKEY 0xDE
#define FUJICMD_READ_APPKEY 0xDD

#ifdef CUSTOM_FUJINET_CALLS 

int16_t custom_network_call(char *url, uint8_t *buffer, uint16_t max_len)
{
    int16_t count;
    int8_t  i, wait;

    // Delete existing file
    cbm_open(15,11,15, "s:vice-in"); 

    // Write command file (currently just the url )
    cbm_open(N_LFN,11,1,"vice-out"); 
    cbm_write(N_LFN, url, strlen(url)); 
    cbm_close(N_LFN);

    // Wait until command file no longer exists (signifies a response is ready) */
    buffer[0]=0; 
    wait=0; 
    count = 1;
    while (count != 0 ) {

        // If we timed out (>15 seconds), return false 
        wait++; 
        if (wait > 90) {
        return 0;
        }

        // Short delay
        for (i=0;i<10;i++) { 
        waitvsync(); 
        } 

        // Check if file exists by renaming it
        count = cbm_open(15,11,15, "r0:vice-out=vice-out"); 
        cbm_close(15); 
    } 

    // Read the response into the buffer 
    cbm_open(N_LFN,11,0,"vice-in"); 
    count = cbm_read(N_LFN,buffer, max_len); 
    cbm_close(N_LFN); 

    return count;
}

unsigned char open_appkey(unsigned char open_mode, unsigned int creator_id, unsigned char app_id, char key_id)
{
    static unsigned char appkey_error;

    if (open_mode == APPKEY_WRITE)
        strcpy(appkey.payload.write.value, "s:");
    else
        strcpy(appkey.payload.write.value, "");

    itoa(creator_id, &appkey.payload.write.value[strlen(appkey.payload.write.value)], 16);

    strcat(appkey.payload.write.value, OPEN_APPKEY_DELIM);
    itoa(app_id, &appkey.payload.write.value[strlen(appkey.payload.write.value)], 16);

    strcat(appkey.payload.write.value, OPEN_APPKEY_DELIM);
    itoa(key_id, &appkey.payload.write.value[strlen(appkey.payload.write.value)], 16);

    strcat(appkey.payload.write.value, ".key");

    if (open_mode == APPKEY_WRITE) {
        appkey_error = cbm_open(LFN, DEV, 15, appkey.payload.write.value);
        cbm_close(LFN);
        appkey_error = cbm_open(LFN,DEV,open_mode,&appkey.payload.write.value[2]);
    } else {
        appkey_error = cbm_open(LFN,DEV,open_mode,&appkey.payload.write);
    }
    
    return appkey_error;
    
}

uint16_t custom_read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, char *destination)
{
    static uint8_t appkey_error;
    uint16_t read = 0;
    
    if (appkey_error = open_appkey(APPKEY_READ, creator_id, app_id, key_id))
        return 0;

    if (!appkey_error) {
        read = cbm_read(LFN, &appkey.payload, sizeof(appkey.payload.read));

        if (read>1 && appkey.payload.read.length <= MAX_APPKEY_LEN) {
            memcpy(destination, appkey.payload.read.value, appkey.payload.read.length);
        } else {
            appkey_error = 64;
        }
    }

    cbm_close(LFN);
    return appkey.payload.read.length;
}
void custom_write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint16_t count, char *data)
{
    static unsigned char appkey_error;
    static unsigned char len;

    if (appkey_error = open_appkey(APPKEY_WRITE, creator_id, app_id, key_id))
        return;

    len = count;
    if (len > MAX_APPKEY_LEN)
        len = MAX_APPKEY_LEN;

    memset(&appkey.payload, 0, sizeof(appkey.payload.read));
    appkey.payload.read.length = len;
    memcpy(&appkey.payload.read.value, data, appkey.payload.read.length);

    cbm_write(LFN, &appkey.payload.read, sizeof(appkey.payload.read));
    cbm_close(LFN);
}
#endif
