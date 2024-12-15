#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void PrintUse() {
    printf("Moon Version Generator Utility\n    usage: 'versiongen -n|<major> <minor>'\n    example: 'versiongen 1 3' generates something like '1.3.22278.12345', while 'versiongen -n' generates something like '2227812345'.");
}

int main(int argc, const char** argv) {
    int bNumericMode = 0;
    if (argc == 2) {
        bNumericMode = 1;
        if ((argv[1][0] != '-') || (argv[1][1] != 'n')) {
            PrintUse();
            return 1;
        }
    } else if (argc < 3) {
        PrintUse();
        return 1;
    }

    time_t timer;
    struct tm* tmInfo;

    timer = time(0);
    tmInfo = localtime(&timer);

    int revision = (tmInfo->tm_hour * 60 * 60) + (tmInfo->tm_min * 60) + tmInfo->tm_sec;

    if (bNumericMode) {
        // BUILDREV
        // build = последние 2 цифры года и дня года
        // rev = количество секунд с полуночи
        printf("%02d%02d%05d", tmInfo->tm_year % 100, tmInfo->tm_yday, revision);
    } else {
        // MAJOR.MINOR.BUILD.REV
        // build = последние 2 цифры года и дня года
        // rev = количество секунд с полуночи
        int major = atoi(argv[1]);
        int minor = atoi(argv[2]);
        printf("%d.%d.%02d%02d.%05d", major, minor, tmInfo->tm_year % 100, tmInfo->tm_yday, revision);
    }
    return 0;
}