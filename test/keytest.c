#include <stdio.h>

int main(int argc,char **argv)
{
    int     key = 0;

    do
    {
        key = getchar();

        printf("%d\n\r",key);

        break;
    }
    while(1);

    return 0;
}
