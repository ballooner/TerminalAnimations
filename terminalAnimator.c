//Includes
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>


//Defines
#define CTRL_KEY(k) ((k) & 0x1f)


//Data
enum ScreenStates {
    MAIN = 1,
    ANIMATE = 2
};

struct terminalInfo {
    struct termios originalParams;
    struct termios currParams;
    int rowSize;
    int colSize;
    int cursorX;
    int cursorY;
    enum ScreenStates screenState;
};

struct terminalInfo termInfo;
struct sigaction windowSizeAction;


//Terminal Functions
void die(const char *s)
{
    perror(s);
    exit(-1);
}

void leaveRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termInfo.originalParams);
}

void enterRawMode()
{
    tcgetattr(STDIN_FILENO, &termInfo.originalParams);
    termInfo.currParams = termInfo.originalParams;

    atexit(leaveRawMode);

    termInfo.currParams.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | ICRNL
                                     | IGNCR | ICRNL | IXON);
    termInfo.currParams.c_oflag &= ~OPOST;
    termInfo.currParams.c_lflag &= ~ (ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termInfo.currParams.c_cflag &= ~(CSIZE | PARENB);
    termInfo.currParams.c_cflag |= CS8;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termInfo.currParams);
}

void getCursorPosition()
{
    //Request position report
    if (write(STDIN_FILENO, "\x1b[6n", 4) == -1) die("getCursorPosition report");

    char buffer[32];
    unsigned int index = 0;

    //Read cursor position report
    while (read(STDIN_FILENO, &buffer[index], 1) == 1)
    {
        if (index > sizeof(buffer) - 1) break;
        if (buffer[index++] == 'R') break;
    }

    buffer[index] = '\0';

    //Process the report
    if (buffer[0] != '\x1b' && buffer[1] != '[') die("getCursorPosition process");
    sscanf(&buffer[2], "%d;%d", &termInfo.cursorY, &termInfo.cursorX);
}

int getTerminalSize(int *row, int *col)
{
    //Move cursor to bottom right and request cursor position
    if (write(STDIN_FILENO, "\x1b[999B\x1b[999C", 12) == -1) return -1;

    getCursorPosition();

    *col = termInfo.cursorX;
    *row = termInfo.cursorY;

    return 0;
}

void windowResizeHandler(int signum)
{
    getTerminalSize(&termInfo.rowSize, &termInfo.colSize);
}


//Render functions
void clearScreen()
{
    //Clear the screen
    if (write(STDIN_FILENO, "\x1b[2J", 4) == -1) die("clearScreen");

    //Set the cursor to home
    if (write(STDIN_FILENO, "\x1b[0H", 4) == -1) die("clearScreen set cursor");
}

    //Render the main menu
void renderMainMenu()
{
    printf("1. Animate\r\n");
    printf("2. Quit\r\n");
}

    //Render the animation overlay to show keybinds
void renderAnimationOverlay()
{
    int topOverlayRow = termInfo.rowSize - (termInfo.rowSize / 4);

    //Set cursor to home position
    if (write(STDIN_FILENO, "\x1b[0H", 4) == -1) die("renderOverlay set home");

    char buffer[32];

    //Move down 3/4 of the screen
    sprintf(buffer, "\x1b[%dB", topOverlayRow);
    if (write(STDIN_FILENO, buffer, strlen(buffer)) == -1)
        die("renderOverlay move cursor");

    //Set bg of future text to white
    if (write(STDIN_FILENO, "\x1b[107m", 6) == -1)
        die("renderOverlay set bg to white");

    //Set 3/4 line to white
    for (int i = 0; i < termInfo.colSize; i++)
    {
        printf(" ");
    }

    printf("\r\n");
    if (write(STDIN_FILENO, "\x1b[49m", 5) == -1)
        die("renderOverlay set bg color to default");
    if (write(STDIN_FILENO, "\x1b[0H", 4) == -1) 
        die("renderOverlay set cursor to home");
}


//Input
void processMainInput()
{
    char c;
    read(STDIN_FILENO, &c, 1);
    switch (c)
    {
        case CTRL_KEY('q'): case '2':
            clearScreen();
            exit(0);
            break;
        case '1':
            termInfo.screenState = ANIMATE;
            break;
    }
}

void processAnimateInput()
{
    char c;
    read(STDIN_FILENO, &c, 1);
    switch (c)
    {
        case CTRL_KEY('q'):
            clearScreen();
            exit(0);
            break;
    }
}

void processScreen()
{
    while (1)
    {
        clearScreen();
        getCursorPosition();

        //Decide what menu to render
        switch (termInfo.screenState)
        {
            case MAIN:
                renderMainMenu();
                processMainInput();
                break;
            case ANIMATE:
                renderAnimationOverlay();
                processAnimateInput();
                break;
        }

    }
}

//Init
int main(void)
{
    termInfo.screenState = MAIN;
    enterRawMode();
    
    //Set up a custom signal handler to update terminal size variables when
    //window size is changed
    sigemptyset(&windowSizeAction.sa_mask);
    windowSizeAction.sa_flags = 0;
    windowSizeAction.sa_handler = windowResizeHandler;

    sigaction(SIGWINCH, &windowSizeAction, NULL);

    if (getTerminalSize(&termInfo.rowSize, &termInfo.colSize) == -1) 
        die("getTerminalSize");

    processScreen();

    return 0;
}
