/*
 *      cv_to_word.c
 *      Alex Jackson (ajacks06) & Kalina Allen (kallen07)
 *      Assignment 4 - arith
 *      10/12/15
 *              The implementation for compressing an array of component video
 *              pixels to an array of bit-packed words
 */

#include <stdlib.h>
#include "assert.h"
#include "uarray.h"
#include "bitpack.h"
#include "arith40.h"
#include "rgb_to_cv.h"
#include "cv_to_word.h"

#define BLOCKSIZE 2
#define BRIGHTNESS_DEGREE_LIM 0.3
#define WORD_WIDTH 32


/* defines the order of each component in a codeword */
enum Order{ PR, PB, D, C, B, A };

/* an array of the widths of each component in a codeword
   in the order defined above */
const int WIDTH[6] = {4, 4, 6, 6, 6, 6};


struct Compressed_img {
        int width, height;
        UArray_T array;  /* array of Cv_pixel */
};

typedef struct Codeword_vals {
        uint64_t a, Pb, Pr;
        int64_t b, c, d;
} *Codeword_vals;

typedef struct Closure {
        Compressed_img compressed_img;
        UArray_T temp_array;  /* array of Cv_pixels */
        int img_index, array_counter;
} *Closure;



/**** helper functions for compression ****/
static void process_array(int col, int row, UArray2b_T array, void *elem, 
                                                              void *cl);
static void avg_P_vals(Codeword_vals codeword_vals, Closure cl);
static void transform_Y_vals(Codeword_vals codeword_vals, Closure cl);
static int64_t scale_brightness_degree(float val, enum Order width_index);

static uint64_t pack_word(Codeword_vals word_vals);
static unsigned get_lsb(enum Order width_index);


/**** helper functions for decompression ****/
static void process_words(int col, int row, UArray2b_T array, void *elem, 
                                                              void *cl);
static Codeword_vals unpack_word(uint64_t word);
static void get_P_vals(Codeword_vals codeword_vals, Closure cl);
static void get_Y_vals(Codeword_vals codeword_vals, Closure cl);
static float unscale_brightness_degree(int64_t val, enum Order width_index);



/* returns an array of unsigned 32 bit "words"  
   therefore compressing 4 pixels into one word */
extern Compressed_img cv_to_word(Cv_pixel_array cv_image)
{
        int length;
        Compressed_img return_img;

        assert(UArray2b_blocksize(cv_image->array) == BLOCKSIZE);

        Closure cl = malloc( sizeof(*cl) );
        assert(cl != NULL);

        cl->array_counter = 0;
        cl->img_index = 0;
        cl->temp_array = UArray_new(BLOCKSIZE * BLOCKSIZE, 
                                    sizeof(struct Cv_pixel));
        
        cl->compressed_img = malloc( sizeof(*(cl->compressed_img)) );
        assert(cl->compressed_img != NULL);

        cl->compressed_img->width = cv_image->width;;
        cl->compressed_img->height = cv_image->height;

        length = (cv_image->width * cv_image->height) / 
                     (BLOCKSIZE * BLOCKSIZE);
        cl->compressed_img->array = UArray_new(length, sizeof(uint64_t));

        UArray2b_map(cv_image->array, process_array, cl);

        return_img = cl->compressed_img;
        
        UArray_free(&(cl->temp_array));
        free(cl);

        return return_img;
}


/* decompresses a word into 4 component video pixels */
extern Cv_pixel_array word_to_cv(Compressed_img image)
{
        assert(image != NULL);

        Cv_pixel_array pixel_array = malloc(sizeof(*pixel_array));
        assert(pixel_array != NULL);
        pixel_array->width = image->width;
        pixel_array->height = image->height;
        pixel_array->array = UArray2b_new(pixel_array->width, 
                       pixel_array->height, sizeof(struct Cv_pixel), BLOCKSIZE);

        Closure cl = malloc(sizeof(*cl));
        assert(cl != NULL);
        cl->img_index = 0;
        cl->array_counter = 0;
        cl->compressed_img = image;
        cl->temp_array = UArray_new(BLOCKSIZE * BLOCKSIZE, 
                                    sizeof(struct Cv_pixel));

        UArray2b_map(pixel_array->array, process_words, cl);

        UArray_free(&(cl->temp_array));
        free(cl);

        return pixel_array;
}


/* prints out each word in row major order */
extern void print_words(Compressed_img image)
{
        int length, byte_index, mod_val;
        uint64_t temp_word;

        assert(image != NULL);
        printf("COMP40 Compressed image format 2\n%u %u\n", image->width, 
                                                            image->height);

        length = UArray_length(image->array);

        byte_index = WORD_WIDTH;
        mod_val = byte_index % 8;

        /* makes the index of the most significant byte is a multiple of 8 */
        if (mod_val != 0) {
                byte_index -= mod_val;
        }
        else {
                byte_index -= 8;
        }

        for (int i = 0; i < length; i++) {
                temp_word = *((uint64_t*)UArray_at(image->array, i));
                for (int j = byte_index; j >= 0; j -= 8) {
                        putchar(temp_word >> j);
                }
        }

        return;
}


/* reads words from a file pointer to a COMP40 compresed image format 2 */
extern Compressed_img read_words(FILE *fp)
{
        int read, c, length, bytes_in_word, byte_index, mod_val, curr_lsb;
        uint64_t temp_word;

        unsigned height, width;
        read = fscanf(fp, "COMP40 Compressed image format 2\n%u %u", 
                                                               &width, &height);
        assert(read == 2);
        c = getc(fp);
        assert(c == '\n');

        length = width * height / 4;

        Compressed_img compressed_img = malloc(sizeof(*compressed_img));
        assert(compressed_img != NULL);
        compressed_img->width = width;
        compressed_img->height = height;
        compressed_img->array = UArray_new(length, sizeof(uint64_t));

        bytes_in_word = (WORD_WIDTH + 8 - 1) / 8; /* get the ceiling */

        byte_index = WORD_WIDTH;
        mod_val = byte_index % 8;

        /* makes the index of the most significant byte is a multiple of 8 */
        if (mod_val != 0) {
                byte_index -= mod_val;
        }
        else {
                byte_index -= 8;
        }

        for (int img_index = 0; img_index < length; img_index++) {
            curr_lsb = byte_index;
            temp_word = 0;
            for (int byte_count = 0; byte_count < bytes_in_word; byte_count++) {
                temp_word = Bitpack_newu(temp_word, 8, curr_lsb, 
                                                       (uint64_t)getc(fp));
                curr_lsb -= 8;
            }

            *((uint64_t*)(UArray_at(compressed_img->array, img_index))) = 
                                                                    temp_word;
        }


        return compressed_img;
}


extern void compressed_img_free(Compressed_img *image)
{
        assert(image != NULL && *image != NULL);
        UArray_free(&((*image)->array));
        free(*image);

        return;
}




/*******************   COMPRESSION HELPER FUNCTIONS   *************************/

static void process_array(int col, int row, UArray2b_T array, void *elem, 
                                                              void *cl)
{       
        (void)col;
        (void)row;
        (void)array;
        
        assert(elem != NULL && cl != NULL);
        Closure c = cl;

        *((Cv_pixel)UArray_at(c->temp_array, c->array_counter)) = 
                                                           *((Cv_pixel)elem);
        c->array_counter += 1;
        if (c->array_counter == 4) {
                Codeword_vals word_vals = malloc(sizeof(*word_vals));
                assert(word_vals != NULL);

                avg_P_vals(word_vals, c);
                transform_Y_vals(word_vals, c);

                *((uint64_t*)UArray_at(c->compressed_img->array, 
                                        c->img_index)) = pack_word(word_vals); 

                free(word_vals);
                c->array_counter = 0;
                c->img_index += 1;
        }

        return;
}


static void avg_P_vals(Codeword_vals codeword_vals, Closure cl)
{
        assert(codeword_vals != NULL && cl != NULL);
        assert(cl->array_counter == 4);

        float avg_Pb = 0;
        float avg_Pr = 0;

        for (int i = 0; i < cl->array_counter; i++) {
            avg_Pb += ((Cv_pixel)UArray_at(cl->temp_array, i))->Pb;
            avg_Pr += ((Cv_pixel)UArray_at(cl->temp_array, i))->Pr;
        }

        avg_Pb /= cl->array_counter;
        avg_Pr /= cl->array_counter;

        codeword_vals->Pb = Arith40_index_of_chroma(avg_Pb);
        codeword_vals->Pr = Arith40_index_of_chroma(avg_Pr);

        return;
}


static void transform_Y_vals(Codeword_vals codeword_vals, Closure cl)
{
        assert(codeword_vals != NULL && cl != NULL);
        assert(cl->array_counter == 4);

        float a, b, c, d;
        float Y[5];

        for (int i = 0; i < cl->array_counter; i++) {
                Y[i + 1] = ((Cv_pixel)UArray_at(cl->temp_array, i))->Y;
        }

        a = (Y[4] + Y[3] + Y[2] + Y[1]) / 4.0;
        b = (Y[4] + Y[3] - Y[2] - Y[1]) / 4.0;
        c = (Y[4] - Y[3] + Y[2] - Y[1]) / 4.0;
        d = (Y[4] - Y[3] - Y[2] + Y[1]) / 4.0;

        assert(a >= 0 && a <= 1);
        assert(b >= -0.5 && b <= 0.5);
        assert(c >= -0.5 && c <= 0.5);
        assert(d >= -0.5 && d <= 0.5);

        codeword_vals->a = (uint64_t)(a * (((uint64_t)1 << WIDTH[A]) - 1));

        codeword_vals->b = scale_brightness_degree(b, B);
        codeword_vals->c = scale_brightness_degree(c, C);
        codeword_vals->d = scale_brightness_degree(d, D);

        return;
}


static int64_t scale_brightness_degree(float val, enum Order width_index)
{
        float range;
        float multiplier;

        if (val > BRIGHTNESS_DEGREE_LIM) {
                val = BRIGHTNESS_DEGREE_LIM;
        }
        if (val < -BRIGHTNESS_DEGREE_LIM) {
                val = -BRIGHTNESS_DEGREE_LIM;
        }

        range = ((uint64_t)1 << (WIDTH[width_index] - 1)) - 1;
        multiplier = range / BRIGHTNESS_DEGREE_LIM;

        return (int64_t)(val * multiplier);
}


static uint64_t pack_word(Codeword_vals word_vals)
{
        uint64_t word = 0;
        word = Bitpack_newu(word, WIDTH[PR], get_lsb(PR), word_vals->Pr);
        word = Bitpack_newu(word, WIDTH[PB], get_lsb(PB), word_vals->Pb);
        word = Bitpack_news(word, WIDTH[D], get_lsb(D), word_vals->d);
        word = Bitpack_news(word, WIDTH[C], get_lsb(C), word_vals->c);
        word = Bitpack_news(word, WIDTH[B], get_lsb(B), word_vals->b);
        word = Bitpack_newu(word, WIDTH[A], get_lsb(A), word_vals->a);
        return word;
}


static unsigned get_lsb(enum Order width_index)
{
        unsigned lsb = 0;
        for (unsigned i = 0; i < (unsigned)width_index; i++) {
                lsb+= WIDTH[i];
        } 
        return lsb;
}




/******************   DECOMPRESSION HELPER FUNCTIONS   ************************/

static void process_words(int col, int row, UArray2b_T array, void *elem, 
                                                              void *cl)
{
        (void)col;
        (void)row;
        (void)array;

        assert(elem != NULL && cl != NULL);

        Closure c = cl;


        if (c->array_counter == 0) {
                Codeword_vals word_vals = unpack_word(*((uint64_t*)
                            UArray_at(c->compressed_img->array, c->img_index)));
                c->img_index++;

                get_P_vals(word_vals, c);
                get_Y_vals(word_vals, c);

                free(word_vals);
        }

        *((Cv_pixel)elem) = *((Cv_pixel)UArray_at(c->temp_array, 
                                                    c->array_counter));

        c->array_counter += 1;

        if (c->array_counter == 4) {
                c->array_counter = 0;
        }

        return;
}


static Codeword_vals unpack_word(uint64_t word)
{
        Codeword_vals word_vals = malloc(sizeof(*word_vals));
        assert(word_vals != NULL);

        word_vals->Pr = Bitpack_getu(word, WIDTH[PR], get_lsb(PR));
        word_vals->Pb = Bitpack_getu(word, WIDTH[PB], get_lsb(PB));
        word_vals->d = Bitpack_gets(word, WIDTH[D], get_lsb(D));
        word_vals->c = Bitpack_gets(word, WIDTH[C], get_lsb(C));
        word_vals->b = Bitpack_gets(word, WIDTH[B], get_lsb(B));
        word_vals->a = Bitpack_getu(word, WIDTH[A], get_lsb(A));

        return word_vals;
}


static void get_P_vals(Codeword_vals codeword_vals, Closure cl)
{    
        assert(codeword_vals != NULL && cl != NULL);
        assert(cl->array_counter == 0);

        float avg_Pr = Arith40_chroma_of_index(codeword_vals->Pr);
        float avg_Pb = Arith40_chroma_of_index(codeword_vals->Pb);

        Cv_pixel curr_pixel;
        for (int i = 0; i < BLOCKSIZE * BLOCKSIZE; i++) {
                curr_pixel = (Cv_pixel)UArray_at(cl->temp_array, i);
                curr_pixel->Pr = avg_Pr;
                curr_pixel->Pb = avg_Pb;
        }

        return;
}


static void get_Y_vals(Codeword_vals codeword_vals, Closure cl)
{
        assert(codeword_vals != NULL && cl != NULL);
        assert(cl->array_counter == 0);

        float a = codeword_vals->a;

        int range = ((uint64_t)1 << WIDTH[A]) - 1;
        if (a > range) {
            a = range;
        }

        a = a / range;
        
        float b = unscale_brightness_degree(codeword_vals->b, B);
        float c = unscale_brightness_degree(codeword_vals->c, C);
        float d = unscale_brightness_degree(codeword_vals->d, D);

        float Y[5];
        Y[1] = a - b - c + d;
        Y[2] = a - b + c - d;
        Y[3] = a + b - c - d;
        Y[4] = a + b + c + d;

        Cv_pixel curr_pixel;
        for (int i = 0; i < BLOCKSIZE * BLOCKSIZE; i++) {
                curr_pixel = (Cv_pixel)UArray_at(cl->temp_array, i);
                curr_pixel->Y = Y[i + 1];
        }

        return;
}


static float unscale_brightness_degree(int64_t val, enum Order width_index)
{
        float range;
        float denominator;
        float return_val;

        range = ((uint64_t)1 << (WIDTH[width_index] - 1)) - 1;

        if (val > range) {
                val = range;
        }
        if (val < -range) {
                val = -range;
        }

        denominator = range / BRIGHTNESS_DEGREE_LIM;
        return_val = (val / denominator);

        return return_val;
}