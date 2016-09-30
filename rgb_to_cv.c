/*
 *      rgb_to_cv.c
 *      Alex Jackson (ajacks06) & Kalina Allen (kallen07)
 *      Assignment 4 - arith
 *      10/12/15
 *          The implementation for a set of functions that convert pixels in
 *          rgb format to pixels in component video format.
 */

#include <stdlib.h>
#include "assert.h"
#include "a2methods.h"
#include "a2blocked.h"
#include "rgb_to_cv.h"

/* denominator for conversion from cv to rgb format */
#define DENOMINATOR 255;

/* transformation matrix from rgb to cv */
const float matrix[3][3] = {
        {0.299, 0.587, 0.114},
        {-0.168736, -0.331264, 0.5},
        {0.5, -0.418688, -0.081312}
};

/* cv to rgb transformation matrix (inverse of the above matrix) */
const float inv_matrix[3][3] = {
        {1.0, 0.0, 1.402},
        {1.0, -0.344136, -0.714136},
        {1.0, 1.772, 0.0},
};

/**** helper functions ****/
static Cv_pixel get_cv(Pnm_rgb pixel, unsigned denominator);
static Pnm_rgb get_rgb(Cv_pixel pixel, unsigned denominator);


extern Cv_pixel_array rgb_to_cv(Pnm_ppm rgb_image, int blocksize)
{
        assert(rgb_image != NULL && blocksize > 0);
        Cv_pixel_array cv_image = malloc(sizeof(*cv_image));
        Pnm_rgb curr_rgb;
        Cv_pixel curr_cv;

        cv_image->width = rgb_image->width;
        cv_image->height = rgb_image->height;

        UArray2b_T cv_array = UArray2b_new(cv_image->width, 
                    cv_image->height, sizeof(struct Cv_pixel), blocksize); 

        for (int row = 0; row < cv_image->height; row++) {
                for (int col = 0; col < cv_image->width; col++) {
                        curr_rgb = rgb_image->methods->at(rgb_image->pixels, 
                                                          col, row);

                        curr_cv = get_cv(curr_rgb, rgb_image->denominator);

                        *((Cv_pixel)(UArray2b_at(cv_array, col, 
                                                 row))) = *curr_cv;
                        free(curr_cv);
                }
        }
        cv_image->array = cv_array;

        return cv_image;
}


extern Pnm_ppm cv_to_rgb(Cv_pixel_array cv_image)
{
        assert(cv_image != NULL);
        Pnm_ppm rgb_image = malloc(sizeof(*rgb_image));
        Pnm_rgb curr_rgb;
        Cv_pixel curr_cv;

        rgb_image->height = cv_image->height;
        rgb_image->width = cv_image->width;
        rgb_image->denominator = DENOMINATOR;
        rgb_image->methods = uarray2_methods_blocked;
        rgb_image->pixels = rgb_image->methods->new(rgb_image->width,
                                     rgb_image->height, sizeof(struct Pnm_rgb));

        for (int row = 0; row < (int)rgb_image->height; row++) {
            for (int col = 0; col < (int)rgb_image->width; col++ ) {
                curr_cv = UArray2b_at(cv_image->array, col, row);
                curr_rgb = get_rgb(curr_cv, rgb_image->denominator);

                *((Pnm_rgb)(rgb_image->methods->at(rgb_image->pixels, 
                                                   col, row))) = *curr_rgb;

                free(curr_rgb);
            }
        }

        return rgb_image;
}


extern void cv_pixels_free(Cv_pixel_array *cv_image) {
        assert(cv_image != NULL && *cv_image != NULL);
        UArray2b_free(&((*cv_image)->array));
        free(*cv_image);

        return;
}



/***************************** HELPER FUNCTIONS *******************************/

static Cv_pixel get_cv(Pnm_rgb pixel, unsigned denominator) 
{
        assert(pixel != NULL);
        float red = (pixel->red) / (float)denominator;
        float green = (pixel->green) / (float)denominator;
        float blue = (pixel->blue) / (float)denominator;

        Cv_pixel new_pixel = malloc(sizeof(*new_pixel));
        assert(new_pixel != NULL);

        new_pixel->Y = red * matrix[0][0] + green * matrix[0][1] + 
                       blue * matrix[0][2];
        new_pixel->Pb = red * matrix[1][0] + green * matrix[1][1] + 
                        blue * matrix[1][2];
        new_pixel->Pr = red * matrix[2][0] + green * matrix[2][1] + 
                        blue * matrix[2][2];

        return new_pixel; 
}


static Pnm_rgb get_rgb(Cv_pixel pixel, unsigned denominator)
{
        assert(pixel != NULL);

        float Y, Pb, Pr, temp_red, temp_green, temp_blue;

        Y = pixel->Y;
        Pb = pixel->Pb;
        Pr = pixel->Pr;

        Pnm_rgb new_pixel = malloc(sizeof(*new_pixel));
        assert(new_pixel != NULL);

        temp_red = (Y * inv_matrix[0][0] + Pb * inv_matrix[0][1] + 
                         Pr * inv_matrix[0][2]) * denominator;
        temp_green = (Y * inv_matrix[1][0] + Pb * inv_matrix[1][1] + 
                        Pr * inv_matrix[1][2]) * denominator;
        temp_blue = (Y * inv_matrix[2][0] + Pb * inv_matrix[2][1] + 
                        Pr * inv_matrix[2][2]) * denominator;

        if (temp_red < 0) {
            temp_red = 0;
        } else if (temp_red > denominator) {
            temp_red = denominator;
        }
        if (temp_green < 0) {
            temp_green = 0;
        } else if (temp_green > denominator) {
            temp_green = denominator;
        }
        if (temp_blue < 0) {
            temp_blue = 0;
        } else if (temp_blue > denominator) {
            temp_blue = denominator;
        }

        new_pixel->red = temp_red;
        new_pixel->green = temp_green;
        new_pixel->blue = temp_blue;

        return new_pixel; 
}


#undef DENOMINATOR