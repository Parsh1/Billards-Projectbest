#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include<stdio.h>

#define score_min 15
#define ball_size 13

#define GET_R(px) (unsigned int)(((px) & 0x00FF0000u) >> 16)
#define GET_G(px) (unsigned int)(((px) & 0x0000FF00u) >> 8)
#define GET_B(px) (unsigned int)((px) & 0x000000FFu)

typedef struct { int r, y, w; } scores;

scores check_score(int x, int y, const unsigned int *a, int largeur,
	int Rrmin, int Rrmax, int Rgmin, int Rgmax, int Rbmin, int Rbmax,
	int Yrmin, int Yrmax, int Ygmin, int Ygmax, int Ybmin, int Ybmax,
	int Wrmin, int Wrmax, int Wgmin, int Wgmax, int Wbmin, int Wbmax, int ball_size) {

	scores s= { 0,0,0 };
	int r = 0;
	int g =0;
	int b=0;
	for (int i = x; i < x + ball_size; i++) {
		for (int j = y; j < y + ball_size; j++) {

			unsigned int px = a[j * largeur + i];
			 r = GET_R(px);
			 g = GET_G(px);
			 b = GET_B(px);

			if (Rrmin <= r && r <= Rrmax && Rgmin <= g && g <= Rgmax && Rbmin <= b && b <= Rbmax) s.r++;
			if (Yrmin <= r && r <= Yrmax && Ygmin <= g && g <= Ygmax && Ybmin <= b && b <= Ybmax) s.y++;
			if (Wrmin <= r && r <= Wrmax && Wgmin <= g && g <= Wgmax && Wbmin <= b && b <= Wbmax) s.w++;

		}
	}
	return s;
}

void etape_4(int largeur, const unsigned int* a, int Lmin, int Lmax, int Cmin, int Cmax, 
	int Rrmin, int Rrmax, int Rgmin, int Rgmax, int Rbmin, int Rbmax,
	int Yrmin, int Yrmax, int Ygmin, int Ygmax, int Ybmin, int Ybmax,
	int Wrmin, int Wrmax, int Wgmin, int Wgmax, int Wbmin, int Wbmax,
	const char* posPath,int ball_size) {

	int red[3] = { 0,0,0 };
	int yellow[3] = { 0,0,0 };
	int white[3] = { 0,0,0 };

	for (int i = Cmin; i <= Cmax-ball_size+1; i++) {
		for (int j = Lmin; j <= Lmax-ball_size+1; j++) {

			scores score = check_score(i,j,a, largeur, 
				 Rrmin, Rrmax, Rgmin, Rgmax, Rbmin, Rbmax,
				 Yrmin, Yrmax, Ygmin, Ygmax, Ybmin, Ybmax,
				 Wrmin, Wrmax, Wgmin, Wgmax, Wbmin, Wbmax, ball_size);

			if (score.r > red[2]) {
				red[2] = score.r;
				red[1] = j;
				red[0] = i;
			}
			if (score.y > yellow[2]) {
				yellow[2] = score.y;
				yellow[1] = j;
				yellow[0] = i;
			}
			if (score.w > white[2]) {
				white[2] = score.w;
				white[1] = j;
				white[0] = i;
			}
		}
	}

	if (red[2] < score_min) {

		red[2] = 0;
		red[1] = -1;
		red[0] = -1;
		
	}
	if (yellow[2] < score_min) {

		yellow[2] = 0;
		yellow[1] = -1;
		yellow[0] = -1;

	}
	if (white[2] < score_min) {

		white[2] = 0;
		white[1] = -1;
		white[0] = -1;

	}

	
	FILE* fp = fopen(posPath, "w");
	if (!fp) {
		perror(posPath);
		return;
	}

	
	fprintf(fp, "Red: %d, %d, %d\n", red[0], red[1], red[2]);
	fprintf(fp, "Yellow: %d, %d, %d\n", yellow[0], yellow[1], yellow[2]);
	fprintf(fp, "White: %d, %d, %d\n", white[0], white[1], white[2]);

	fclose(fp);
}

int read_all_pixels(FILE *fp, unsigned int W, unsigned int H, unsigned int **outPix)
{
/*
 Reads W*H pixels from the binary file.
 Important: 'fp' is open in "rb" mode and positioned after reading Weight (W) and Height (H).

It:
   - Allocates a buffer with 'malloc' for W*H pixels
   - Fills it in a single fread call
   - Stores the pointer into *outPix and returns 0
If there's an error:
   - Prints an error message in stderr
   - Frees the allocated memory
   - Returns 1
Also:
   - If the file contains extra pixel, prints a warning
     and ignores the extras.
 */

    // Checks if fp and outPix exist
    if (!fp || !outPix) {
        fprintf(stderr, "ERR: invalid arguments to read_all_pixels\n");
        return 1;
    }

    // Compute tge total number of pixels
    size_t total_pixels = (size_t)W * (size_t)H;

    // Allocate a dynamic array of size_t using malloc
    unsigned int *buffer = (unsigned int*)malloc(total_pixels * sizeof(unsigned int));
    if (!buffer) { // checks malloc error
        fprintf(stderr, "ERR: out of memory for %zu pixels\n", total_pixels);
        return 1;
    }

    // Read all pixels
    size_t pixels_read = fread(buffer, sizeof(unsigned int), total_pixels, fp);
    if (pixels_read < total_pixels) { // checks if there are enough pixels
        fprintf(stderr, "ERR: not enough pixels: got %zu need %zu\n", pixels_read, total_pixels);
        free(buffer);
        return 1; 
    }

    // Detect extra pixels
    // If this succeeds, there are extra values in the file
    unsigned int extra_probe;
    if (fread(&extra_probe, sizeof(unsigned int), 1, fp) == 1) {
        fprintf(stderr, "WARN: extra pixels detected (ignored)\n");
    }

    // Deliver the array with al the values by linkning it to the address *outPix
    *outPix = buffer;
    return 0;
}

int main(int argc, char* argv[]) {

	if (argc != 30) {
		printf("Expected to see 29 parameters instead there are %d\n", argc - 1);
		exit(1);
	}

	// The above makes sure we received the correct number of lines for parameters

	for (int i = 5, i < 29, i++) {
		if (argv[i] < 0 || argv[i] > 255) {
			printf("There is an error in the color parameter values");
			exit(1);
		}
	}
			
	FILE *fp = fopen("pixmap.bin", "rb") // Reads file and puts the file reader in binary mode 
		if (!fp) {						// Makes sure file is there 
			perror("File open failed");	// Message to tell us where the error is
				return 1;
		}

	int Table[2];						// Add the table from memory allocation here instead, but using this tentatively
	if (fread(Table, sizeof(int), 2, fp != 2) {	// Checks to make sure it has read 2 elements 
		perror ("Failed to read the width and length");		// Error code
		fclose(fp);										// Closes file to do further checks before proceeding, however, 
		return 1;										// this can be combined with the next step later if slow
	}
	int Largeur = Table [0];			// Replace with the 0th entry of the malloc table we make
	int Hauteur = Table [1];			

		if (Largeur > 1000 || (0 <= Largeur && Largeur <= 100) || Hauteur > 100 || (0 <= Hauteur && Hauteur <= 100) {
			printf (" The bounds provided are invalid, so the program cannot continue");
			exit(1);
		}
		if (Largeur < 0 || Hauteur < 0) {
			printf("The bounds are negative so the program cannot continue");
			exit(1);
		}

		// The above lines check the bounds for the height and width

		long size = ftell(fp);
		if (size != (4 * Largeur * Hauteur)) {
			printf("You have a mismatched number of pixels vs the size")
		}
		// Check the number of elements
	// Miguels function calling:
	unsigned int *Pix = NULL; // Fichier ou se stockent tous les pixels
	if (read_all_pixels(fp, W, H, &Pix) != 0) {
		fclose(fp);
		return 1;
	}
	fclose(fp); // Imporant: Je ferme le fichier pixmap.bin puisque tout est lu est stocké

 etape_4(Largeur,Pix,argv[3],argv[4],argv[5],argv[6],argv[7],argv[8],argv[9],argv[10],argv[11],argv[12],argv[13],argv[14],argv[15],argv[16],
         argv[17],argv[18],argv[19],argv[20],argv[21],argv[22],argv[23],argv[24],argv[2],argv[31]);

free(Pix);


	// Après la partie de Bogdan, important: il faut faire free(Pix) !!!

}

