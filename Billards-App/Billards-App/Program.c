#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


int main(int argc, char* argv[]) {

	if (argc != 30) {
		printf("Expected to see 29 parameters instead there are %d\n", argc - 1);
		exit(1);
	}

	// The above makes sure we recieved the correct number of lines for parameters

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

	int Table[2];						// Add the table from memory allocation here instead but using this tentatively
	if (fread(Table, sizeof(int), 2, fp != 2) {	// Checks to make sure it has read 2 elements 
		perror ("Failed to read the width and length");		// Error code
		fclose(fp);										// Closes file to do further checks before proceeding however, 
		return 1;										// this can be combined with the next step later if slow
	}
	int Largeur = Table [0];			// Replace with the 0th entry of the malloc table we make
	int Hauteur = Table [1];			

		if (Largeur > 1000 || 0 <= Largeur <= 100 || Hauteur > 100 || 0 <= Hauteur <= 100) {
			printf (" The bounds provided are invalid so the program cannot continue");
			exit(1);
		}
		if (Largeur < 0 || Hauteur < 0) {
			printf("The bounds are negative so the program cannot continue");
			exit(1);
		}

		// The above lines check the bounds for the height and width

}
