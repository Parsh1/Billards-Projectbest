/*
 * Pix2Pos.c  —  EPFL PPI 2025 — Part C
 * Authors: <your names here>
 * Target: Windows (Visual Studio) and macOS/Linux (clang/gcc)
 *
 * Summary:
 *   Reads "pixmap.bin" (unsigned int width, height, then width*height pixels in 0x00RRGGBB),
 *   searches (inside the board rectangle) the BallSize×BallSize window with the highest score
 *   for each color (Red, Yellow, White), and writes "pos.txt" with "top-left" (X,Y) and Score.
 *
 * Key specs implemented:
 *   - CLI: exactly 29 params (board Lmin Lmax Cmin Cmax, 6×Red, 6×Yellow, 6×White, 6×BlueBG, BallDiameter)
 *   - Errors (to stderr + non-zero exit): wrong CLI count; BallDiameter out of [5..20];
 *       cannot open/read input; width/height out of [100..1000]; not enough pixels;
 *       cannot write output (permissions).
 *   - Warnings (to stderr, continue): too many pixels (extra ignored); fewer than 3 balls found.
 *   - Constants: BallminScore = 15 (a ball is valid only if best window score >= 15).
 *   - File names EXACT: "pixmap.bin", "pos.txt".
 *   - No VLA; clear, readable C; functions & comments.
 *
 * Coordinate system:
 *   X = column, Y = row, origin at top-left (0,0).
 *
 * Notes:
 *   - BallSize == BallDiameter (per project statement).
 *   - BlueBG ranges are parsed (required by CLI) but not used in the C search itself.
 *
 * © 2025 — Course "Programmation pour Ingénieur"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

 /* ----------------------------- Types & Constants ----------------------------- */

typedef struct { unsigned int rmin, rmax, gmin, gmax, bmin, bmax; } ColorRange;
/* Board rectangle given by LabVIEW: rows [Lmin..Lmax], cols [Cmin..Cmax] */
typedef struct { unsigned int Lmin, Lmax, Cmin, Cmax; } BoardRect;

typedef struct {
    BoardRect board;
    ColorRange red, yellow, white, blueBG;
    unsigned int ballSize; /* == BallDiameter */
} Params;

typedef struct { int x, y; unsigned int score; } Detection;

enum {
    BALL_MIN_SCORE = 15,   /* Minimal score to accept a ball */
    CLI_PARAM_COUNT = 29   /* Number of user params, argv must be 1+29 */
};

/* ----------------------------- RGB Helpers ---------------------------------- */

/* Pixel format: 0x00RRGGBB in an unsigned int (32-bit) */
static inline unsigned int getR(unsigned int p) { return (p >> 16) & 0xFF; }
static inline unsigned int getG(unsigned int p) { return (p >> 8) & 0xFF; }
static inline unsigned int getB(unsigned int p) { return (p) & 0xFF; }

static inline int in_range(unsigned int p, const ColorRange* cr) {
    const unsigned int R = getR(p), G = getG(p), B = getB(p);
    return (R >= cr->rmin && R <= cr->rmax) &&
        (G >= cr->gmin && G <= cr->gmax) &&
        (B >= cr->bmin && B <= cr->bmax);
}

/* ----------------------------- CLI Parsing ---------------------------------- */

/* Parse exactly 29 CLI parameters, fill Params; return 0 if OK, <0 on error. */
static int parse_args(int argc, char** argv, Params* P) {
    if (argc != 1 + CLI_PARAM_COUNT) {
        fprintf(stderr, "ERROR: wrong number of CLI parameters (expected %d, got %d).\n",
            CLI_PARAM_COUNT, argc - 1);
        return -1;
    }
    /* Board rectangle: Lmin Lmax Cmin Cmax */
    P->board.Lmin = (unsigned int)strtoul(argv[1], NULL, 10);
    P->board.Lmax = (unsigned int)strtoul(argv[2], NULL, 10);
    P->board.Cmin = (unsigned int)strtoul(argv[3], NULL, 10);
    P->board.Cmax = (unsigned int)strtoul(argv[4], NULL, 10);

    /* Helper macro to load a 6-tuple for a ColorRange */
#define LOAD_CR(off, cr) do{ \
        (cr).rmin = (unsigned int)strtoul(argv[(off)+0], NULL, 10); \
        (cr).rmax = (unsigned int)strtoul(argv[(off)+1], NULL, 10); \
        (cr).gmin = (unsigned int)strtoul(argv[(off)+2], NULL, 10); \
        (cr).gmax = (unsigned int)strtoul(argv[(off)+3], NULL, 10); \
        (cr).bmin = (unsigned int)strtoul(argv[(off)+4], NULL, 10); \
        (cr).bmax = (unsigned int)strtoul(argv[(off)+5], NULL, 10); \
    } while(0)

    LOAD_CR(5, P->red);
    LOAD_CR(11, P->yellow);
    LOAD_CR(17, P->white);
    LOAD_CR(23, P->blueBG);
#undef LOAD_CR

    P->ballSize = (unsigned int)strtoul(argv[29], NULL, 10);
    if (P->ballSize < 5 || P->ballSize > 20) {
        fprintf(stderr, "ERROR: BallDiameter out of bounds [5..20], got %u.\n", P->ballSize);
        return -2;
    }
    /* Blue BG not used by the algorithm, still parsed as requested */
    (void)P->blueBG;
    return 0;
}

/* ----------------------------- File I/O ------------------------------------- */

/* Read pixmap.bin (lowercase names per specs); allocates *outPix with W*H pixels.
 * Returns 0 on success, <0 on error. Prints errors/warnings on stderr as per specs. */
static int read_pixmap(const char* path,
    unsigned int** outPix,
    unsigned int* W, unsigned int* H,
    int* outTooManyPixels /* 1 if extra pixels present */)
{
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "ERROR: cannot open input file '%s'.\n", path); return -10; }

    unsigned int w = 0, h = 0;
    if (fread(&w, sizeof(unsigned int), 1, f) != 1 ||
        fread(&h, sizeof(unsigned int), 1, f) != 1) {
        fclose(f);
        fprintf(stderr, "ERROR: cannot read width/height from '%s'.\n", path);
        return -11;
    }

    /* Validate width/height in [100..1000] (this slide "fait foi") */
    if (w < 100 || w > 1000 || h < 100 || h > 1000) {
        fclose(f);
        fprintf(stderr, "ERROR: width/height out of bounds [100..1000] (W=%u, H=%u).\n", w, h);
        return -12;
    }

    /* Safe allocation */
    size_t n = (size_t)w * (size_t)h;
    unsigned int* pix = (unsigned int*)malloc(n * sizeof(unsigned int));
    if (!pix) { fclose(f); fprintf(stderr, "ERROR: out of memory for pixel buffer.\n"); return -13; }

    size_t got = fread(pix, sizeof(unsigned int), n, f);
    if (got < n) {
        free(pix); fclose(f);
        fprintf(stderr, "ERROR: not enough pixels in '%s' (expected %zu, got %zu).\n", path, n, got);
        return -14;
    }

    /* Check for extra pixels (warning, continue): try to read one more */
    unsigned int dummy;
    size_t extra = fread(&dummy, sizeof(unsigned int), 1, f);
    if (extra == 1) {
        /* Possibly more than one. Don't care how many, spec says: ignore extras but warn. */
        *outTooManyPixels = 1;
    }
    else {
        *outTooManyPixels = 0;
    }

    fclose(f);
    *outPix = pix; *W = w; *H = h;
    return 0;
}

/* ----------------------------- Scanning & Scoring --------------------------- */

/* Compute clamped search bounds for top-left of the BallSize×BallSize window.
 * Returns 1 if there is at least one valid top-left position, 0 otherwise. */
static int compute_search_bounds(unsigned int W, unsigned int H,
    const BoardRect* B, unsigned int sz,
    unsigned int* xStart, unsigned int* xEnd,
    unsigned int* yStart, unsigned int* yEnd)
{
    /* Normalize potentially inverted bounds (be lenient, not an error). */
    unsigned int Lmin = (B->Lmin <= B->Lmax) ? B->Lmin : B->Lmax;
    unsigned int Lmax = (B->Lmin <= B->Lmax) ? B->Lmax : B->Lmin;
    unsigned int Cmin = (B->Cmin <= B->Cmax) ? B->Cmin : B->Cmax;
    unsigned int Cmax = (B->Cmin <= B->Cmax) ? B->Cmax : B->Cmin;

    /* Clamp to image; then convert to top-left feasible range (<= W-sz / H-sz). */
    if (W < sz || H < sz) return 0; /* Impossible anyway */

    if (Cmin > W - 1) Cmin = W - 1;
    if (Cmax > W - 1) Cmax = W - 1;
    if (Lmin > H - 1) Lmin = H - 1;
    if (Lmax > H - 1) Lmax = H - 1;

    unsigned int xS = (Cmin <= (W - sz)) ? Cmin : (W - sz);
    unsigned int yS = (Lmin <= (H - sz)) ? Lmin : (H - sz);
    unsigned int xE = (Cmax >= (sz - 1)) ? ((Cmax >= (W - 1)) ? (W - sz) : (Cmax - (sz - 1))) : 0;
    unsigned int yE = (Lmax >= (sz - 1)) ? ((Lmax >= (H - 1)) ? (H - sz) : (Lmax - (sz - 1))) : 0;

    if (xS > xE || yS > yE) return 0;

    *xStart = xS; *xEnd = xE; *yStart = yS; *yEnd = yE;
    return 1;
}

/* Score a sz×sz window at top-left (x,y) for a given color range. */
static unsigned int window_score(const unsigned int* pix,
    unsigned int W, unsigned int H,
    unsigned int x, unsigned int y,
    unsigned int sz, const ColorRange* cr)
{
    (void)H; /* H is not needed directly as we use row*W + col addressing */
    unsigned int s = 0;
    for (unsigned int j = 0; j < sz; ++j) {
        size_t row = (size_t)(y + j) * (size_t)W;
        for (unsigned int i = 0; i < sz; ++i) {
            unsigned int p = pix[row + (x + i)];
            s += (unsigned int)in_range(p, cr);
        }
    }
    return s;
}

/* Find the best window for one color inside the board area; returns (-1,-1,0) if no valid. */
static Detection find_best_for_color(const unsigned int* pix,
    unsigned int W, unsigned int H,
    const BoardRect* B,
    unsigned int sz,
    const ColorRange* cr)
{
    Detection d = { -1, -1, 0 };

    unsigned int xS = 0, xE = 0, yS = 0, yE = 0;
    if (!compute_search_bounds(W, H, B, sz, &xS, &xE, &yS, &yE)) {
        /* No feasible position inside the board; not an error, just "not found". */
        return d;
    }

    for (unsigned int y = yS; y <= yE; ++y) {
        for (unsigned int x = xS; x <= xE; ++x) {
            unsigned int s = window_score(pix, W, H, x, y, sz, cr);
            if (s > d.score) { d.score = s; d.x = (int)x; d.y = (int)y; }
        }
    }
    if (d.score < BALL_MIN_SCORE) { d.x = -1; d.y = -1; d.score = 0; }
    return d;
}

/* ----------------------------- Output --------------------------------------- */

static int write_pos(const char* path, Detection r, Detection y, Detection w) {
    FILE* f = fopen(path, "w");
    if (!f) { fprintf(stderr, "ERROR: cannot write output file '%s'.\n", path); return -20; }

    /* Exact format and order (Red, Yellow, White) */
    fprintf(f, "Red: %d, %d, %u\n", r.x, r.y, r.score);
    fprintf(f, "Yellow: %d, %d, %u\n", y.x, y.y, y.score);
    fprintf(f, "White: %d, %d, %u\n", w.x, w.y, w.score);

    fclose(f);
    return 0;
}

/* ----------------------------- Main ----------------------------------------- */

int main(int argc, char** argv) {
    Params P;
    if (parse_args(argc, argv, &P) != 0) {
        /* Error already printed */
        return 1;
    }

    unsigned int* pix = NULL, W = 0, H = 0;
    int tooManyPixels = 0;
    {
        int rc = read_pixmap("pixmap.bin", &pix, &W, &H, &tooManyPixels);
        if (rc != 0) return 2; /* Errors already printed */
        if (tooManyPixels) {
            fprintf(stderr, "WARNING: too many pixels in 'pixmap.bin' (extra values ignored).\n");
        }
    }

    /* Find best windows per color (inside board) */
    Detection dR = find_best_for_color(pix, W, H, &P.board, P.ballSize, &P.red);
    Detection dY = find_best_for_color(pix, W, H, &P.board, P.ballSize, &P.yellow);
    Detection dW = find_best_for_color(pix, W, H, &P.board, P.ballSize, &P.white);

    /* Warn if fewer than 3 balls found (score==0 means "not found") */
    int nbFound = (dR.score > 0) + (dY.score > 0) + (dW.score > 0);
    if (nbFound < 3) {
        fprintf(stderr, "WARNING: fewer than 3 balls detected (%d/3). Missing balls are reported as -1,-1,0.\n", nbFound);
    }

    /* Write output file in exact format and order */
    if (write_pos("pos.txt", dR, dY, dW) != 0) {
        free(pix);
        return 3;
    }

    free(pix);
    return 0;
}
