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
	int Wrmin, int Wrmax, int Wgmin, int Wgmax, int Wbmin, int Wbmax) {

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
	const char* posPath) {

	int red[3] = { 0,0,0 };
	int yellow[3] = { 0,0,0 };
	int white[3] = { 0,0,0 };

	for (int i = Cmin; i <= Cmax-ball_size+1; i++) {
		for (int j = Lmin; j <= Lmax-ball_size+1; j++) {

			scores score = check_score(i,j,a, largeur, 
				 Rrmin, Rrmax, Rgmin, Rgmax, Rbmin, Rbmax,
				 Yrmin, Yrmax, Ygmin, Ygmax, Ybmin, Ybmax,
				 Wrmin, Wrmax, Wgmin, Wgmax, Wbmin, Wbmax);

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
}

