#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define score_min 15

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
	int ball_size) {

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

	FILE* fp = fopen("Pos.txt", "w");   // <-- was fopen(posPath, "w")
	if (!fp) {
		perror("Pos.txt");              // <-- was perror(posPath)
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


	int values[29];
for (int i = 1; i < 30; i++) {
	values[i - 1] = atoi(argv[i]);
	if (values[i - 1] > 255 || values[i - 1] <= 0) // This makes sure that our atoi is not reading some erroneous values
}
if ( values[28] > 20 || values[28] < 5) { 
	printf("The ball diameter is wrong impossible to continue");
	exit(1);
}
// Values[28] refers to the parameter of Ball_Diameter
			
FILE* fp = fopen("Pixmap.bin", "rb"); // Reads file and puts the file reader in binary mode 
		if (!fp) {						// Makes sure file is there 
			perror("File open failed");	// Message to tell us where the error is
				return 1;
		}
		unsigned int Table[2]; // Fills Table with the values for Largeur and Hauteur
		if (fread(Table, sizeof(unsigned int), 2, fp) != 2) {
			fprintf(stderr, "Error: Could not read the values for Largeur & Hauteur");
			fclose(fp);
			return 1;
		}
	int Largeur = Table[0];			// Read from the first 2 values of the binary file
	int Hauteur = Table[1];			
	int expected = Largeur * Hauteur;
	int count = 0;
	unsigned int temporary; // unsigned to avoid inconsistencies with the counting

	while (fread(&temporary, sizeof(unsigned int), 1, fp) == 1) { // The pointer has already read the first 2 elements so count is accurate
		count++;
	}

	fseek(fp, 2 * sizeof(unsigned int), SEEK_SET);

	// The above line resets the file pointer to make it go again to the 2nd entry since we read the number of pixels again in the function of read all pixels
	// This way it should read it properly and not interfere with the second function

	if (count > expected) {
		printf("You have too many pixels and in this case only %d of them will be read and used\n", expected); // line to make sure correct number of readings
	};
	if (count < expected) {
		printf("Not enough pixels to run the program so it will end we needed %d pixels and instead there are only %d\n",expected, count);
		exit(1);
	}

	// The above makes sure we have the correct number of pixels
		if (Largeur > 1000 ||  Largeur < 100 || Hauteur > 1000 || Hauteur < 100) {
			printf (" The bounds provided are invalid, so the program cannot continue");
			exit(1);
		}

		// The above lines check the bounds for the height and width

	// Miguels function calling:
	unsigned int *Pix = NULL; // Fichier ou se stockent tous les pixels
	if (read_all_pixels(fp, Largeur, Hauteur, &Pix) != 0) {
		fclose(fp);
		return 1;
	}
	fclose(fp); // Imporant: Je ferme le fichier pixmap.bin puisque tout est lu et stocké

etape_4(Largeur,Pix,values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], 
	values[8], values[9], values[10], values[11], values[12], values[13], values[14], values[15], 
	values[16], values[17], values[18], values[19], values[20], values[21], values[28]);

free(Pix);

}