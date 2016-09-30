/*
 *      compress40.c
 *      Alex Jackson (ajacks06) & Kalina Allen (kallen07)
 *      Assignment 4 - arith
 *      10/10/15
 *              The implementation for a set of functions that compress or 
 *              decompress a .ppm image
 */

#include <stdio.h>
#include <stdlib.h>
#include "assert.h"
#include "pnm.h"
#include "a2plain.h"
#include "a2blocked.h"

#include "compress40.h"
#include "rgb_to_cv.h"
#include "cv_to_word.h"

/* reads in a ppm and trims the width and/or height to ensure that they are 
   both even numbers */
static Pnm_ppm get_ppm(FILE *fp); 


void compress40(FILE *fp)
{
        Pnm_ppm ppm_image = get_ppm(fp);
        Cv_pixel_array cv_pixel_array = rgb_to_cv(ppm_image, 2);
        Pnm_ppmfree(&ppm_image);
        Compressed_img compressed_img = cv_to_word(cv_pixel_array);
        cv_pixels_free(&cv_pixel_array);
        print_words(compressed_img);
        compressed_img_free(&compressed_img);

        return;
}


void decompress40(FILE *fp)
{
        Compressed_img compressed_img = read_words(fp);
        Cv_pixel_array cv_pixel_array = word_to_cv(compressed_img);
        compressed_img_free(&compressed_img);
        Pnm_ppm ppm_image = cv_to_rgb(cv_pixel_array);
        cv_pixels_free(&cv_pixel_array);
        Pnm_ppmwrite(stdout, ppm_image);
        Pnm_ppmfree(&ppm_image);

        return;
}



/*************************** HELPER FUNCTIONS *********************************/

static Pnm_ppm get_ppm(FILE *fp) 
{
        Pnm_ppm image;
        A2Methods_T methods;
        
        /* default to UArray2 methods */
        methods = uarray2_methods_blocked; 
        assert(methods != NULL);

        image = Pnm_ppmread(fp, methods);

        image->width = image->width % 2 == 0 ? image->width : image->width - 1;
        image->height = image->height % 2 == 0 ? 
                                             image->height : image->height - 1;
                                             
        return image;
}