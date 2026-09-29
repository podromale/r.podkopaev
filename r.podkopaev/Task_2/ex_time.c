#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t now = time(NULL) - 8 * 60 * 60;
    struct tm *california = gmtime(&now);
    char output[32];

    if (california == NULL || strftime(output, sizeof(output),
                                       "%Y-%m-%d %H:%M:%S PST", california) == 0) {
        fputs("could not format time\n", stderr);
        return 1;
    }
    puts(output);
    return 0;
}
