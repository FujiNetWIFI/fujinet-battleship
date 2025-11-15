#include <stdlib.h>
#include "stateclient.h"
#include "screens.h"
#include "misc.h"
#include "gamelogic.h"

#include "platform-specific/graphics.h"
#include "platform-specific/input.h"
#include "platform-specific/util.h"
#include "platform-specific/sound.h"
#include <string.h>

#define PLAYER_NAME_MAX 8
#define PLAYER_BOX_TOP 13
#define REMOVE_PLAYER_KEY '/'
#define INGAME_MENU_X WIDTH / 2 - 8

#ifdef __WATCOMC__
#include <conio.h>
#define cgetc getch
#endif

bool inBorderedScreen = false, prevBorderedScreen = false, savedScreen = false;

bool saveScreen()
{
    prevBorderedScreen = inBorderedScreen;
    return savedScreen = saveScreenBuffer();
}

bool restoreScreen()
{
    if (savedScreen)
    {
        savedScreen = false;
        inBorderedScreen = prevBorderedScreen;
        restoreScreenBuffer();
        return true;
    }
    return false;
}

/// @brief Shows information about the game
void showHelpScreen()
{
    uint8_t y;
#define X WIDTH / 2 - 16

    resetScreen();

    drawTextAlt(WIDTH / 2 - 14, 1, "how to play FUJI BATTLESHIP");
    drawLine(WIDTH / 2 - 16, 2, 31);
    y = 3;

    // 12345678901234567890123456789012
    y++;
    drawTextAlt(X, y, "BATTLESHIP is a classic game of");
    y++;
    drawTextAlt(X, y, "sea battle for up to 4 players!");

    y += 3;
    centerTextAlt(y, "GAME PLAY");

    y++;
    y++;
    drawTextAlt(X, y, "player starts with FIVE ships,");
    y++;
    drawTextAlt(X, y, "taking turns attacking until");
    y++;
    drawTextAlt(X, y, "only ONE player remains.");
    y++;
    y++;
    drawTextAlt(X, y, "when you attack a location, ALL");
    y++;
    drawTextAlt(X, y, "ENEMY players are attacked.");

    y++;
    y++;
    y++;
    drawTextAlt(X, y, "use your strategy and luck to");
    y++;
    drawTextAlt(X, y, "be the last player standing!");

    centerStatusText("press any key to close");

    clearCommonInput();
    cgetc();
}

/// @brief Action called in Welcome Screen to check if a server name is stored in an app key
void welcomeActionVerifyServerDetails()
{
    uint8_t i;
    // Read server endpoint into server and query
    read_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, tempBuffer);

    i = (uint8_t)strlen(tempBuffer);
    if (i)
    {
        // Split "server?query" into separate variables
        while (--i)
        {
            if (tempBuffer[i] == '?')
            {
                strcpy(query, tempBuffer + i);
                tempBuffer[i] = 0;
                strcpy(serverEndpoint, tempBuffer);
                break;
            }
        }
    }
}

void drawLogo()
{
    drawBox(WIDTH / 2 - 8, 0, 15, 1);
    drawTextAlt(WIDTH / 2 - 7, 1, "FUJI battleship");
}

void showPlayerNameScreen()
{
    uint8_t i, y;
    resetScreen();

    centerText(13, "ENTER YOUR NAME");
    drawBox(WIDTH / 2 - PLAYER_NAME_MAX / 2 - 1, 16, PLAYER_NAME_MAX + 1, 1);
    drawText(WIDTH / 2 - PLAYER_NAME_MAX / 2, 17, playerName);

    i = (uint8_t)strlen(playerName);

    clearCommonInput();
    while (!inputFieldCycle(WIDTH / 2 - PLAYER_NAME_MAX / 2, 17, PLAYER_NAME_MAX, playerName))
        ;

    for (y = 13; y < 19; ++y)
        centerText(y, "                 ");

    write_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME, strlen(playerName), playerName);
}

/// @brief Action called in Welcome Screen to verify player has a name
void welcomeActionVerifyPlayerName()
{
    char *ptr;
    // Read player's name from app key
    read_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME, tempBuffer);
    tempBuffer[8] = 0;
    strcpy(playerName, tempBuffer);

    // Capture username if player didn't come in from the lobby
    if (!playerName[0])
    {
        showPlayerNameScreen();
    }

    // Convert to lowercase
    ptr = playerName;
    while (*ptr)
    {
        if (*ptr >= 65 && *ptr <= 90)
            *ptr += 32;
        ptr++;
    }
}

/// @brief Shows the Welcome Screen with Logo. Asks player's name
void showWelcomeScreen()
{
    // Retrieve the main player's name
    welcomeActionVerifyPlayerName();

    // Parse server url from app key if present
    welcomeActionVerifyServerDetails();

    // If first run, show the help screen
    if (!prefs.seenHelp)
    {
        prefs.seenHelp = true;
        savePrefs();
        showHelpScreen();
    }

    pause(30);
}

#define TWID 26
#define RMAR (WIDTH / 2 + TWID / 2)
#define LMAR (WIDTH / 2 - TWID / 2)

/// @brief Shows a screen to select a table to join
void showTableSelectionScreen()
{
    uint8_t shownCursor, tableIndex, blinkCursor, redrawScreen, i, j;
    Table *table;
    state.inGame = tableIndex = blinkCursor = 0;

    resetScreen();

    // An empty query means a table needs to be selected
    while (strlen(query) == 0)
    {
        // Show names of local player(s)

        strcpy(tempBuffer, "HELLO ");
        strcat(tempBuffer, playerName);
        centerTextAlt(20, tempBuffer);

        if (clientState.tables.count > 0)
        {
            for (i = 0; i < clientState.tables.count; ++i)
            {
                drawSpace(LMAR, 9 + i * 2, TWID);
            }
        }
        waitvsync();
        centerText(12, "      refreshing game list..      ");

        drawLogo();

        centerText(4, "choose a game to join");
        drawText(LMAR, 7, "game");
        drawText(RMAR - 7, 7, "players");
        drawLine(LMAR, 8, TWID);

        // waitvsync();

        clientState.tables.count = 0;
        apiCall("tables");

        if (clientState.tables.count > 0)
        {
            drawSpace(LMAR, 12, TWID);
            for (i = 0; i < clientState.tables.count; ++i)
            {
                table = &clientState.tables.table[i];
                j = 9 + i * 2;
                drawTextAlt(LMAR, j, table->name);
                drawTextAlt(RMAR - 5, j, table->players);

                if (table->players[0] > '0')
                {
                    drawIcon(RMAR - 7, j, ICON_PLAYER);
                }
                // cgetc();
            }
        }
        else
        {
            centerText(12, "no servers are available");
        }

        centerStatusText("Refresh    Help     Name    Quit");

#ifdef COLOR_TOGGLE
        if (prefs.color)
        {
            drawLine(6, HEIGHT, 1);  // R
            drawLine(15, HEIGHT, 1); // H
            drawLine(21, HEIGHT, 1); // P
            drawLine(30, HEIGHT, 1); // Q
        }
#endif

        shownCursor = !clientState.tables.count;

        clearCommonInput();
        while (!input.trigger || !clientState.tables.count)
        {

            if (clientState.tables.count)
            {
                drawIcon(LMAR - 2, 9 + tableIndex * 2, blinkCursor < 50 ? ICON_MARK : ICON_MARK_ALT);
            }

            waitvsync();
            blinkCursor = (blinkCursor + 1) % 60;
            readCommonInput();

            if (input.key == 'h' || input.key == 'H')
            {
                saveScreen();
                showHelpScreen();
                if (!restoreScreen())
                {
                    resetScreen();
                    break;
                }
            }
            else if (input.key == 'r' || input.key == 'R')
            {
                drawBlank(LMAR - 2, 9 + tableIndex * 2);
                break;
            }
            else if (input.key == 'c' || input.key == 'C')
            {
                cycleNextColor();
                savePrefs();
#ifdef COLOR_CYCLE_REQUIRES_REDRAW
                break;
#endif
            }
            else if (input.key == 's' || input.key == 'S')
            {
                prefs.disableSound = !prefs.disableSound;
                soundCursor();
                savePrefs();
            }
            else if (input.key == 'n' || input.key == 'N')
            {
                showPlayerNameScreen();
                resetScreen();
                break;
            }
            else if (input.key == 'q' || input.key == 'Q')
            {
                quit();
            } /*else if (input.key != 0) {
                itoa(input.key, tempBuffer, 10);
                drawStatusText(tempBuffer);
            }*/

            if (!shownCursor || (clientState.tables.count > 0 && input.dirY))
            {

                // Visually unselect old table
                table = &clientState.tables.table[tableIndex];
                j = table->name[0]; // Reference table so cmoc 0.1.96 optimizer does not corrupt it
                j = 9 + tableIndex * 2;

                drawBlank(LMAR - 2, j);
                drawTextAlt(LMAR, j, table->name);
                drawTextAlt(RMAR - 5, j, table->players);

                // Move table index to new table
                tableIndex = (input.dirY + tableIndex + clientState.tables.count) % clientState.tables.count;

                // Visually select new table

                table = &clientState.tables.table[tableIndex];
                j = table->name[0]; // Reference table so cmoc 0.1.96 optimizer does not corrupt it
                j = 9 + tableIndex * 2;

                drawIcon(LMAR - 2, j, ICON_MARK);
                drawText(LMAR, j, table->name);
                drawText(RMAR - 5, j, table->players);

                soundCursor();

                // Housekeeping - allows platform specific housekeeping, like stopping Attract/screensaver mode in Atari
                housekeeping();

                shownCursor = 1;
            }
        }

        if (input.trigger)
        {
            soundSelect();

            // Clear screen and write server name
            resetScreen();
            centerText(15, clientState.tables.table[tableIndex].name);

            strcpy(query, "?table=");
            strcat(query, clientState.tables.table[tableIndex].table);

            // Combine server endpoint and query for final base url
            strcpy(tempBuffer, serverEndpoint);
            strcat(tempBuffer, query);

            //  Update server app key in case of reboot
            write_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, strlen(tempBuffer), tempBuffer);
        }
    }

    centerTextAlt(17, "connecting to server");
    progressAnim(19);

    // Append player name to query
    strcat(query, "&player=");
    strcat(query, playerName);

    // Replace space with + for pshowWelcomScreenlayer name
    i = (uint8_t)strlen(query);
    while (--i)
        if (query[i] == ' ')
            query[i] = '+';

    // Reset the game state
    clearRenderState();
    state.waitingOnEndGameContinue = false;

    // Join table
    apiCall("state");

    // Reduce wait count for an immediate call
    state.apiCallWait = 0;

    state.inGame = true;
}

/// @brief shows in-game menu
void showInGameMenuScreen()
{
    static uint8_t y, i, wait;

    saveScreen();
    state.inGame = false;
    i = wait = 1;
    while (i)
    {

        resetScreen();
        y = HEIGHT / 2 - 3;
        drawTextAlt(INGAME_MENU_X, y, "  Q: quit table");
        drawTextAlt(INGAME_MENU_X, y += 2, "  H: how to play");
        if (prefs.colorMode)
            drawTextAlt(INGAME_MENU_X, y += 2, "  C: color mode");

        drawTextAlt(INGAME_MENU_X, y += 2, prefs.disableSound ? "  S: sound OFF" : "  S: sound ON");

        drawBox(INGAME_MENU_X - 2, HEIGHT / 2 - 5, 19, y - (HEIGHT / 2 - 5) + 1);

        centerTextAlt(HEIGHT - 1, "press TRIGGER/SPACE to close");

        // centerTextAlt(y + 6, tempBuffer);
        clearCommonInput();
        i = 1;
        while (i == 1)
        {
            waitvsync();
            readCommonInput();
            if (input.trigger)
            {
                i = 0;
                break;
            }
            switch (input.key)
            {
            case 's':
            case 'S':
                prefs.disableSound = !prefs.disableSound;
                drawTextAlt(INGAME_MENU_X, y, prefs.disableSound ? "  S: sound OFF" : "  S: sound ON ");
                soundSelect();
                savePrefs();
                break;
            case 'c':
            case 'C':
                cycleNextColor();
                savePrefs();
#ifdef COLOR_CYCLE_REQUIRES_REDRAW
                i = 2;
#endif
                break;

            case 'h':
            case 'H':
                showHelpScreen();
            case 'q':
            case 'Q':
                resetScreen();
                centerText(10, "please wait");

                //  Clear server app key in case of reboot
                write_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, 0, (char *)"");

                // Inform server player is leaving
                apiCall("leave");

                progressAnim(12);

                // Clear query so a new table will be selected
                strcpy(query, "");
                showTableSelectionScreen();

                return;
            }
        }
    }

    // Show game screen again before returning

    clearCommonInput();

    state.inGame = true;
    if ((!state.drawBoard && !restoreScreen()) || state.drawBoard)
    {
        clearRenderState();
        processStateChange();
    }
}
