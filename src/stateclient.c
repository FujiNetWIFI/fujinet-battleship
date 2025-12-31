#include <stdlib.h>
#include <string.h>
#include "stateclient.h"
#include "misc.h"
#include "fujinet-network.h"

// Internal to this file
static char url[160];
char *requestedMove;

/*
 * @brief Makes an Api call, returning true if valid payload received
 * Returns API_CALL_*:
 *  1 - successfully received a payload
 *  2 - async - received payload, still in process, call for more data
 *  0 - error - aborted
 */
uint8_t apiCall(const char *path)
{
    static int16_t read;

    strcpy(url, "n:");
    strcat(url, serverEndpoint);
    strcat(url, path);
    strcat(url, query);
    strcat(url, query[0] ? "&bin=1&v=" API_CLIENT_VERSION : "?bin=1&v=" API_CLIENT_VERSION);

    if (network_open(url, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE))
    {
        return API_CALL_ERROR;
    }

    read = network_read(url, &clientState.firstByte, sizeof(clientState.game));
    network_close(url);

    // If no bytes read, set first byte of clientState to 0, which is the number of tables or players
    if (read <= 0)
    {
        clientState.firstByte = 0;
        return API_CALL_ERROR;
    }

    return API_CALL_SUCCESS;
}

void sendMove(char *move)
{
    if (move != NULL)
        state.apiCallWait = 0;

    requestedMove = move;
}

uint8_t getStateFromServer()
{

    if (requestedMove)
    {
        // Copy requested move to the temp buffer it is not already one and the same
        if (requestedMove != tempBuffer)
            strcpy(tempBuffer, requestedMove);

        requestedMove = NULL;
    }
    else
    {
        strcpy(tempBuffer, "state");
    }

    return apiCall(tempBuffer);
}
