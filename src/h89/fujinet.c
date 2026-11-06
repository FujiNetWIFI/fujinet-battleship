#ifdef __H89__

/**
 * @brief   FujiNet calls - Heathkit H89 (CUSTOM_FUJINET_CALLS)
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * Everything that talks to the H89 FujiNet card through fujinet-lib is in
 * this one file, because it has to see the LIBRARY's <fujinet-fuji.h>: the
 * h89 build of the lib implements the appkey calls as macros over its bus
 * call, so the prototypes in src/fujinet-fuji.h (which src/misc.h pulls in)
 * link against nothing. This file therefore includes none of the game's
 * headers, and the shared code reaches the card through the
 * CUSTOM_FUJINET_CALLS hooks instead. (src/sms/fujinet.c, less the lobby.)
 */

#include <stdbool.h>
#include <stdint.h>
#include <fujinet-fuji.h>
#include <fujinet-network.h>

int16_t custom_network_call(char *url, uint8_t *buffer, uint16_t max_len)
{
    int16_t read;

    if (network_open(url, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE) != FN_ERR_OK)
        return -1;

    read = network_read(url, buffer, max_len);
    network_close(url);
    return read;
}

uint16_t custom_read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, char *destination)
{
    uint16_t read = 0;

    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    if (!fuji_read_appkey(key_id, &read, (uint8_t *)destination))
        read = 0;
    return read;
}

void custom_write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint16_t count, char *data)
{
    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    fuji_write_appkey(key_id, count, (uint8_t *)data);
}

#endif /* __H89__ */
