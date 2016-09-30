/*
 *      cv_to_word.h
 *      Alex Jackson (ajacks06) & Kalina Allen (kallen07)
 *      Assignment 4 - arith
 *      10/12/15
 *              The interface for compressing an array of component video
 *              pixels to an array of bit-packed words
 */

#ifndef CV_TO_WORD_INCLUDED
#define CV_TO_WORD_INCLUDED

#include "rgb_to_cv.h"
#include "uarray.h"

typedef struct Compressed_img *Compressed_img;

/* returns an array of unsigned 32 bit "words"  
   therefore compressing 4 pixels into one word */
extern Compressed_img cv_to_word(Cv_pixel_array cv_image); 

/* decompresses a word into 4 component video pixels */
extern Cv_pixel_array word_to_cv(Compressed_img image);

/* prints out each word in row major order */
extern void print_words(Compressed_img image);

/* reads words from a file pointer to a COMP40 compresed image format 2 */
extern Compressed_img read_words(FILE *fp);

/* frees all data associated with a Compressed_img */
extern void compressed_img_free(Compressed_img *image);

#endif