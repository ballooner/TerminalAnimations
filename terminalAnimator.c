//Includes
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>


//Defines
#define CTRL_KEY(k) ((k) & 0x1f)


//Data
struct terminalInfo {
    struct termios originalParams;
    struct termios currParams;
    int rowSize;
    int colSize;
};

struct terminalInfo termInfo;


//Terminal Functions
void kill(const char *s)
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

int getTerminalSize(int *row, int *col)
{
    if (write(STDIN_FILENO, "\x1b[999B\x1b[999C\x1b[6n", 16) == -1) return -1;

    printf("\r\n");
    char buffer[32];
    unsigned int index = 0;

    while (read(STDIN_FILENO, &buffer[index], 1) == 1)
    {
        if (index > sizeof(buffer) - 1) break;
        if (buffer[index++] == 'R') break;
    }

    buffer[index] = '\0';

    if (buffer[0] != '\x1b' && buffer[1] != '[') return -1;
    sscanf(&buffer[2], "%d;%d", row, col);

    return 0;
}

//Input


//Init
int main(void)
{
    enterRawMode();
    if (getTerminalSize(&termInfo.rowSize, &termInfo.colSize) == -1) 
        kill("getTerminalSize");
    printf("row: %d, col: %d\r\n", termInfo.rowSize, termInfo.colSize);

    char c;

    while (1)
    {
        read(STDIN_FILENO, &c, 1);

        if (c == CTRL_KEY('q'))
        {
            exit(0);
        } else
        {
            printf("%c: %d\r\n", c, c);
        }
    }

    return 0;
}
