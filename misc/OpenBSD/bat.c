/* bat - lookup remaining battery on my laptop with a minumum of fuss.
 * also sysctl(2) practice; inspect the sys/{sysctl,sensors}.h include
 * files and study the source for sysctl(8) to adapt this code */

// NOTE clang-format may sort these which can be bad
#include <sys/time.h>
#include <sys/types.h>

#include <sys/sysctl.h>

#include <sys/sensors.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    enum sensor_type type;
    int dev, mib[5], numt;
    struct sensordev snsrdev;
    size_t sdlen = sizeof(snsrdev);

    struct sensor snsr;
    size_t slen = sizeof(snsr);

    if (pledge("stdio", NULL) == -1) err(1, "pledge failed");

    mib[0] = CTL_HW;
    mib[1] = HW_SENSORS;

    // find where acpibat0 is
    for (dev = 0;; dev++) {
        mib[2] = dev;
        if (sysctl(mib, 3, &snsrdev, &sdlen, NULL, 0) == -1) {
            if (errno == ENXIO) continue;
            // instead throw an error? OTOH a desktop system might not
            // have acpibat0
            if (errno == ENOENT) goto DONE;
        }
        if (strcmp("acpibat0", snsrdev.xname) == 0) break;
    }

    // find watthour ID
    for (type = 0; type < SENSOR_MAX_TYPES; type++)
        if (strcmp("watthour", sensor_type_s[type]) == 0) break;
    if (type == SENSOR_MAX_TYPES) errx(1, "no watthour??");
    mib[3] = type;

    mib[4] = 3; // remaining capacity

    if (sysctl(mib, 5, &snsr, &slen, NULL, 0) == -1) err(1, "sysctl failed");
    if (slen > 0 && (snsr.flags & SENSOR_FINVALID) == 0) {
        if (snsr.type != SENSOR_WATTHOUR) errx(1, "not watthour type??");
        fprintf(stderr, "%.2f%s\n", snsr.value / 1000000.0,
                snsr.value < 1330000 ? " !!" : "");
    } else {
        // battery is not attached, probably
    }

DONE:
    exit(EXIT_SUCCESS);
}
