//Includes
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>


//Defines
#define CTRL_KEY(k) ((k) & 0x1f)


//Data
struct terminalInfo {
    struct termios originalParams;
    struct termios currParams;
    int rowNum;
    int colNum;
};

struct terminalInfo termInfo;


//Terminal Functions
void kill(const char *s)
{
    perror(s);
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

    termInfo.currParams.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | ICRNL | IGNCR | ICRNL | IXON);
    termInfo.currParams.c_oflag &= ~OPOST;
    termInfo.currParams.c_lflag &= ~ (ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termInfo.currParams.c_cflag &= ~(CSIZE | PARENB);
    termInfo.currParams.c_cflag |= CS8;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termInfo.currParams);
}


//Input



//Init
int main(void)
{
    enterRawMode();

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
