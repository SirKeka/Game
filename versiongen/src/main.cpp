#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, const char** argv) {
    if (argc < 2) {
        printf("Moon Version Generator Utility\n    usage: 'versiongen <major> <minor>'\n    example: 'versiongen 1 3' generates something like '1.3.22278.12345'.");
        return 1;
    }

    int major = atoi(argv[1]);
    int minor = atoi(argv[2]);

    time_t timer;
    struct tm* tmInfo;

    timer = time(0);
    tmInfo = localtime(&timer);

    // MAJOR.MINOR.BUILD.REV
    // build = last 2 of year and day of year
    // rev = number of seconds since midnight
    int revision = (tmInfo->tm_hour * 60 * 60) + (tmInfo->tm_min * 60) + tmInfo->tm_sec;
    printf("%d.%d.%02d%02d.%05d", major, minor, tmInfo->tm_year % 100, tmInfo->tm_yday, revision);

    return 0;
}