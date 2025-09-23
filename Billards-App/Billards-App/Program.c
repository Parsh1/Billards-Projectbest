#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


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

	// The above makes sure we recieved the correct number of lines for parameters

	for (int i = 5; i < 29; i++) {
		if (argv[i] < 0 || argv[i] > 255) {
			printf("There is an error in the color parameter values");
			exit(1);
		}
	}
	
	FILE *fp = fopen("pixmap.bin", "rb"); // Reads file and puts the file reader in binary mode 
	if (!fp) {						// Makes sure file is there 
		perror("File open failed");	// Message to tell us where the error is
		return 1;
	}

	int Table[2];
	size_t rd = 0;
	(rd = fread(Table, sizeof(int), 2, fp)
	// Add the table from memory allocation here instead but using this tentatively
	if (rd != 2) {	// Checks to make sure it has read 2 elements 
		perror ("Failed to read the width and length");  // Error code
		fclose(fp);										// Closes file to do further checks before proceeding however, 
		return 1;										// this can be combined with the next step later if slow
	}
	int Largeur = Table [0];			// Replace with the 0th entry of the malloc table we make
	int Hauteur = Table [1];			

		if (Largeur > 1000 || Largeur < 100 || Hauteur > 1000 || Hauteur < 100) {
			printf (" The bounds provided are invalid so the program cannot continue");
			exit(1);
		}
		if (Largeur < 0 || Hauteur < 0) {
			printf("The bounds are negative so the program cannot continue");
			exit(1);
		}

		// The above lines check the bounds for the height and width

	// Miguels function calling:
	unsigned int *Pix = NULL; // Fichier ou se stockent tous les pixels
	if (read_all_pixels(fp, W, H, &Pix) != 0) {
		fclose(fp);
		return 1;
	}
	fclose(fp); // Imporant: Je ferme le fichier pixmap.bin puisque tout est lu est stocké


	// Après la partie de Bogdan, important: il faut faire free(Pix) !!!

}
