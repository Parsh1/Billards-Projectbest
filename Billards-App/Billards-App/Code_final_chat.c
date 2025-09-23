#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

/* ---------------- constants & helpers ---------------- */

#define SCORE_MIN 15

#define GET_R(px) (unsigned int)(((px) & 0x00FF0000u) >> 16)
#define GET_G(px) (unsigned int)(((px) & 0x0000FF00u) >> 8)
#define GET_B(px) (unsigned int)((px) & 0x000000FFu)

typedef struct { int r, y, w; } scores;

/* safe string->int conversion with range check */
static int parse_int(const char *s, int lo, int hi, const char *name, int *out) {
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno || end==s || *end!='\0') {
        fprintf(stderr, "ERR: bad integer for %s: '%s'\n", name, s);
        return 1;
    }
    if (v < lo || v > hi) {
        fprintf(stderr, "ERR: %s out of range [%d..%d]: %ld\n", name, lo, hi, v);
        return 1;
    }
    *out = (int)v;
    return 0;
}

/* read W*H pixels into malloc'd buffer; fp positioned after header */
static int read_all_pixels(FILE *fp, unsigned int W, unsigned int H, unsigned int **outPix) {
    if (!fp || !outPix) {
        fprintf(stderr, "ERR: invalid args to read_all_pixels\n");
        return 1;
    }
    size_t total = (size_t)W * (size_t)H;
    unsigned int *buf = (unsigned int*)malloc(total * sizeof(unsigned int));
    if (!buf) {
        fprintf(stderr, "ERR: out of memory for %zu pixels\n", total);
        return 1;
    }
    size_t n = fread(buf, sizeof(unsigned int), total, fp);
    if (n < total) {
        fprintf(stderr, "ERR: not enough pixels: got %zu need %zu\n", n, total);
        free(buf);
        return 1;
    }
    /* detect extra pixels (ignore) */
    unsigned int extra;
    if (fread(&extra, sizeof(unsigned int), 1, fp) == 1) {
        fprintf(stderr, "WARN: extra pixels detected (ignored)\n");
    }
    *outPix = buf;
    return 0;
}

/* ---------------- scoring & search ---------------- */

scores check_score(int x, int y, const unsigned int *a, int largeur,
                   int Rrmin,int Rrmax,int Rgmin,int Rgmax,int Rbmin,int Rbmax,
                   int Yrmin,int Yrmax,int Ygmin,int Ygmax,int Ybmin,int Ybmax,
                   int Wrmin,int Wrmax,int Wgmin,int Wgmax,int Wbmin,int Wbmax,
                   int ball_size) {
    scores s = (scores){0,0,0};
    const int x_end = x + ball_size;
    const int y_end = y + ball_size;
    for (int j = y; j < y_end; ++j) {
        const int row = j * largeur;
        for (int i = x; i < x_end; ++i) {
            unsigned int px = a[row + i];
            unsigned int R = GET_R(px), G = GET_G(px), B = GET_B(px);
            if (R >= (unsigned)Rrmin && R <= (unsigned)Rrmax &&
                G >= (unsigned)Rgmin && G <= (unsigned)Rgmax &&
                B >= (unsigned)Rbmin && B <= (unsigned)Rbmax) ++s.r;
            if (R >= (unsigned)Yrmin && R <= (unsigned)Yrmax &&
                G >= (unsigned)Ygmin && G <= (unsigned)Ygmax &&
                B >= (unsigned)Ybmin && B <= (unsigned)Ybmax) ++s.y;
            if (R >= (unsigned)Wrmin && R <= (unsigned)Wrmax &&
                G >= (unsigned)Wgmin && G <= (unsigned)Wgmax &&
                B >= (unsigned)Wbmin && B <= (unsigned)Wbmax) ++s.w;
        }
    }
    return s;
}

void etape_4(int largeur, int hauteur, const unsigned int* a,
             int Lmin, int Lmax, int Cmin, int Cmax,
             int Rrmin,int Rrmax,int Rgmin,int Rgmax,int Rbmin,int Rbmax,
             int Yrmin,int Yrmax,int Ygmin,int Ygmax,int Ybmin,int Ybmax,
             int Wrmin,int Wrmax,int Wgmin,int Wgmax,int Wbmin,int Wbmax,
             const char* posPath, int ball_size) {

    if (!a || largeur <= 0 || hauteur <= 0) return;

    /* clamp search box to image bounds */
    if (Lmin < 0) Lmin = 0;
    if (Cmin < 0) Cmin = 0;
    if (Lmax >= hauteur) Lmax = hauteur - 1;
    if (Cmax >= largeur) Cmax = largeur - 1;

    const int last_x = Cmax - ball_size + 1;
    const int last_y = Lmax - ball_size + 1;

    int red[3]    = { -1, -1, 0 };
    int yellow[3] = { -1, -1, 0 };
    int white[3]  = { -1, -1, 0 };

    if (last_x >= Cmin && last_y >= Lmin) {
        for (int j = Lmin; j <= last_y; ++j) {
            for (int i = Cmin; i <= last_x; ++i) {
                scores sc = check_score(i, j, a, largeur,
                                        Rrmin,Rrmax,Rgmin,Rgmax,Rbmin,Rbmax,
                                        Yrmin,Yrmax,Ygmin,Ygmax,Ybmin,Ybmax,
                                        Wrmin,Wrmax,Wgmin,Wgmax,Wbmin,Wbmax,
                                        ball_size);
                if (sc.r > red[2])     { red[0]=i; red[1]=j; red[2]=sc.r; }
                if (sc.y > yellow[2])  { yellow[0]=i; yellow[1]=j; yellow[2]=sc.y; }
                if (sc.w > white[2])   { white[0]=i; white[1]=j; white[2]=sc.w; }
            }
        }
    }

    if (red[2]    < SCORE_MIN) { red[0]    = -1; red[1]    = -1; red[2]    = 0; }
    if (yellow[2] < SCORE_MIN) { yellow[0] = -1; yellow[1] = -1; yellow[2] = 0; }
    if (white[2]  < SCORE_MIN) { white[0]  = -1; white[1]  = -1; white[2]  = 0; }

    FILE *fp = fopen(posPath, "w");
    if (!fp) { perror(posPath); return; }
    fprintf(fp, "Red: %d, %d, %d\n",    red[0],    red[1],    red[2]);
    fprintf(fp, "Yellow: %d, %d, %d\n", yellow[0], yellow[1], yellow[2]);
    fprintf(fp, "White: %d, %d, %d\n",  white[0],  white[1],  white[2]);
    fclose(fp);
}

/* ---------------- main: parse, read, run ---------------- */

int main(int argc, char *argv[]) {
    /* program + 2 filenames + 29 numeric params = 32 */
    if (argc != 32) {
        fprintf(stderr, "ERR: expected 31 arguments, got %d\n", argc - 1);
        return 1;
    }

    const char *inPath  = argv[1];
    const char *outPath = argv[2];

    /* open input */
    FILE *fp = fopen(inPath, "rb");
    if (!fp) { perror(inPath); return 1; }

    /* read header: width, height as unsigned int */
    unsigned int W = 0, H = 0;
    if (fread(&W, sizeof(unsigned int), 1, fp) != 1 ||
        fread(&H, sizeof(unsigned int), 1, fp) != 1) {
        fprintf(stderr, "ERR: failed to read width/height header\n");
        fclose(fp);
        return 1;
    }

    /* basic bounds as per spec [100..1000] */
    if (W < 100 || W > 1000 || H < 100 || H > 1000) {
        fprintf(stderr, "ERR: width/height out of bounds [100..1000]: W=%u H=%u\n", W, H);
        fclose(fp);
        return 1;
    }

    /* read pixels */
    unsigned int *Pix = NULL;
    if (read_all_pixels(fp, W, H, &Pix) != 0) { fclose(fp); return 1; }
    fclose(fp);

    /* parse numeric params: argv[3]..argv[31] */
    int Lmin,Lmax,Cmin,Cmax;
    if (parse_int(argv[3],  0, (int)H-1, "Lmin", &Lmin) ||
        parse_int(argv[4],  0, (int)H-1, "Lmax", &Lmax) ||
        parse_int(argv[5],  0, (int)W-1, "Cmin", &Cmin) ||
        parse_int(argv[6],  0, (int)W-1, "Cmax", &Cmax)) {
        free(Pix); return 1;
    }

    int Rrmin,Rrmax,Rgmin,Rgmax,Rbmin,Rbmax;
    int Yrmin,Yrmax,Ygmin,Ygmax,Ybmin,Ybmax;
    int Wrmin,Wrmax,Wgmin,Wgmax,Wbmin,Wbmax;
    int Brmin,Brmax,Bgmin,Bgmax,Bbmin,Bbmax;
    int BallSize;

    /* helper to shorten code */
    #define PAR(i,lo,hi,name,var) do{ if(parse_int(argv[(i)],(lo),(hi),(name),&(var))){ free(Pix); return 1; } }while(0)

    PAR(7,  0,255,"Rrmin",Rrmin); PAR(8,  0,255,"Rrmax",Rrmax);
    PAR(9,  0,255,"Rgmin",Rgmin); PAR(10, 0,255,"Rgmax",Rgmax);
    PAR(11, 0,255,"Rbmin",Rbmin); PAR(12, 0,255,"Rbmax",Rbmax);

    PAR(13, 0,255,"Yrmin",Yrmin); PAR(14, 0,255,"Yrmax",Yrmax);
    PAR(15, 0,255,"Ygmin",Ygmin); PAR(16, 0,255,"Ygmax",Ygmax);
    PAR(17, 0,255,"Ybmin",Ybmin); PAR(18, 0,255,"Ybmax",Ybmax);

    PAR(19, 0,255,"Wrmin",Wrmin); PAR(20, 0,255,"Wrmax",Wrmax);
    PAR(21, 0,255,"Wgmin",Wgmin); PAR(22, 0,255,"Wgmax",Wgmax);
    PAR(23, 0,255,"Wbmin",Wbmin); PAR(24, 0,255,"Wbmax",Wbmax);

    PAR(25, 0,255,"Brmin",Brmin); PAR(26, 0,255,"Brmax",Brmax);
    PAR(27, 0,255,"Bgmin",Bgmin); PAR(28, 0,255,"Bgmax",Bgmax);
    PAR(29, 0,255,"Bbmin",Bbmin); PAR(30, 0,255,"Bbmax",Bbmax);

    PAR(31, 5,20,"BallDiameter",BallSize);
    #undef PAR

    /* run step 4; we don't use Blue ranges here, but you may need them elsewhere */
    etape_4((int)W, (int)H, Pix,
            Lmin,Lmax,Cmin,Cmax,
            Rrmin,Rrmax,Rgmin,Rgmax,Rbmin,Rbmax,
            Yrmin,Yrmax,Ygmin,Ygmax,Ybmin,Ybmax,
            Wrmin,Wrmax,Wgmin,Wgmax,Wbmin,Wbmax,
            outPath, BallSize);

    free(Pix);
    return 0;
}
