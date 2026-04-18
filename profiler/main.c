#include "velfox_modes.h"

static int run_profiler_mode(int mode) {
    if (mode < 0 || mode > 3) return 0;
    perfcommon();
    if (mode == 1) esport_mode();
    else if (mode == 2) balanced_mode();
    else if (mode == 3) efficiency_mode();
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mode>\n", argv[0]);
        printf("Modes:\n");
        printf("  0 - Common tweaks\n");
        printf("  1 - Esport mode\n");
        printf("  2 - Balanced mode\n");
        printf("  3 - Efficiency mode\n");
        return 1;
    }

    read_configs();
    int mode = atoi(argv[1]);
    if (!run_profiler_mode(mode)) {
        printf("Invalid mode: %d\n", mode);
        return 1;
    }
    return 0;
}
