#ifndef MISC_H
#define MISC_H

//#include <joystick.h>
#include <conio.h>
#include "platform-specific/graphics.h"
#include "platform-specific/input.h"
#include <stdbool.h>
#include <stdint.h>

// Client version string to send to server
#define API_CLIENT_VERSION "2"

// FujiNet AppKey settings. These should not be changed
#define AK_LOBBY_CREATOR_ID 1   // FUJINET Lobby
#define AK_LOBBY_APP_ID 1       // Lobby Enabled Game
#define AK_LOBBY_KEY_USERNAME 0 // Lobby Username key
#define AK_LOBBY_KEY_SERVER 5   // Battleship registered as Lobby appkey 5 at https://github.com/FujiNetWIFI/fujinet-firmware/wiki/SIO-Command-$DC-Open-App-Key

// Battleship
#define AK_CREATOR_ID 0xE41C // Eric Carr's creator id
#define AK_APP_ID 5          // Battleship App ID
#define AK_KEY_PREFS 0       // Preferences

#define PLAYER_MAX 4

#define FUJITZEE_SCORE 14

#define STATUS_LOBBY 0
#define STATUS_PLACE_SHIPS 1
#define STATUS_GAMESTART 10
#define STATUS_MISS 11     // Attack did not hit any ships
#define STATUS_HIT 12      // A ship was hit
#define STATUS_SUNK 13     // Also implies a ship was hit
#define STATUS_GAMEOVER 99 // Also implies a ship was hit

// Set at start to force full redraw
#define STATE_INVALID 200

// Player status
#define PLAYER_STATUS_DEFAULT 0
#define PLAYER_STATUS_DEFEATED 1
#define PLAYER_STATUS_VIEWING 2
#define PLAYER_STATUS_READY 3
#define PLAYER_STATUS_PLACE_SHIPS 10

#define FIELD_ATTACK 1
#define FIELD_MISS 2

#define LEGEND_SHIP_DESTROYED 0
#define LEGEND_SHIP_INTACT 1

#define DRAWSHIP_SHOW 0
#define DRAWSHIP_HIDE 1

typedef struct
{
    char table[9];
    char name[21];
    char players[6];
} Table;

typedef struct
{
    char name[9];
    uint8_t playerStatus;
    uint8_t gamefield[100];
    uint8_t shipsLeft[5];
} Player;

typedef struct
{
    char name[9];
    uint8_t ready;
} LobbyPlayer;

typedef struct
{
    uint8_t count;
    Table table[10];
} Tables;

typedef struct
{
    uint8_t playerCount;
    char prompt[33];
    uint8_t status;
    uint8_t playerStatus;
    int8_t activePlayer;
    uint8_t moveTime;
    uint8_t lastAttackPos;
    // First 5 are my ships, last 5 are winner, sent at game over
    uint8_t myShips[10];
    Player players[PLAYER_MAX];
} Game;

typedef struct
{
    uint8_t playerCount;
    char prompt[33];
    uint8_t status;
    uint8_t playerStatus;
    int8_t activePlayer;
    uint8_t moveTime;
    char serverName[21];
    LobbyPlayer players[PLAYER_MAX];
} Lobby;

typedef union
{
    uint8_t firstByte;
    Game game;
    Lobby lobby;
    Tables tables;
} ClientState;

extern ClientState clientState;

typedef struct
{

    // Internal game state
    uint8_t prevPlayerCount;
    uint8_t prevStatus;
    uint8_t prevPlayerStatus;
    uint8_t apiCallWait;

    int8_t prevActivePlayer;
    int8_t prevAttackPos;

    bool countdownStarted;
    bool waitingOnEndGameContinue;
    bool drawBoard;
    bool inGame;

    // Track gamefield state - used to know when to fire shoot animation
    uint8_t gamefield[PLAYER_MAX][100];

    // Track ships left - used to know when to fire sink animation
    uint8_t shipsLeft[PLAYER_MAX][5];
} GameState;

typedef struct
{
    uint16_t key;
    bool trigger;
    int8_t dirX;
    int8_t dirY;
} InputStruct;

typedef struct
{
    uint8_t debugFlag; // 0xFF to use localhost instead of server
    bool seenHelp;
    uint8_t disableSound;
    uint8_t colorMode;
    uint8_t reserved[20]; // Reserve blank space for future
} PrefsStruct;

extern char tempBuffer[128];
extern char serverEndpoint[50];
extern char localServer[];
extern char query[50];
extern char playerName[12];
extern uint8_t shipSize[5];

extern GameState state;
extern InputStruct input;
extern PrefsStruct prefs;

// Common local scope temp variables

void pause(uint8_t frames);
void clearCommonInput();
void readCommonInput();
void loadPrefs();
void savePrefs();

/// @brief Helper method to write to an appkey
void write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint16_t count, char *data);

/// @brief Helper method to read from an appkey.
/// NULL will be appended to data in case this is a string, though the length returned will not consider the NULL.
uint16_t read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, char *destination);

#endif /* MISC_H */
