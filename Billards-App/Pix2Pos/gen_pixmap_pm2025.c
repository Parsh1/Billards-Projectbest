// gen_pixmap_pm2025.c — run once to create pixmap.bin next to your .vcxproj
#include <stdio.h>
#include "PM_2025.h"  // unsigned int myW=100, myH=80; unsigned int myPM[myW*myH];

int main(void) {
    FILE* f = fopen("pixmap.bin", "wb");
    if (!f) { perror("pixmap.bin"); return 1; }
    fwrite(&myW, sizeof(unsigned int), 1, f);
    fwrite(&myH, sizeof(unsigned int), 1, f);
    fwrite(myPM, sizeof(unsigned int), (size_t)myW * myH, f);
    fclose(f);
    return 0;
}
