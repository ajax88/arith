/*
 *      rgb_to_cv.h
 *      Alex Jackson (ajacks06) & Kalina Allen (kallen07)
 *      Assignment 4 - arith
 *      10/12/15
 *          The interface for a set of functions that convert pixels in rgb 
 *          format to pixels in component video format.
 */

#ifndef RGB_TO_CV_INCLUDED
#define RGB_TO_CV_INCLUDED

#include "uarray2b.h"
#include "pnm.h"

/* colored pixel (component video format) */
typedef struct Cv_pixel {
        float Y, Pb, Pr;
} *Cv_pixel;

typedef struct Cv_pixel_array {
        int width, height;
        UArray2b_T array;  /* an array of 'struct Cv_pixel' elements */
} *Cv_pixel_array;


extern Cv_pixel_array rgb_to_cv(Pnm_ppm rgb_image, int blocksize);
extern Pnm_ppm cv_to_rgb(Cv_pixel_array cv_image);
extern void cv_pixels_free(Cv_pixel_array *cv_image);

#endif