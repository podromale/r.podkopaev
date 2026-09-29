#include <stdio.h>
#include <unistd.h>

static void print_ids(void)
{
    printf("uid=%lu euid=%lu\n", (unsigned long)getuid(), (unsigned long)geteuid());
}

static void read_file(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror(path);
        return;
    }
    puts("file opened");
    if (fclose(file) == EOF)
        perror("fclose");
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "data.txt";
    print_ids();
    read_file(path);

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return 1;
    }
    print_ids();
    read_file(path);
    return 0;
}
