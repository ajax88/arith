# arith - Created by Alex Jackson & Kalina Allen
##arith
###10/22/15

Our progam compresses .ppm image into a COMP40 Compressed image format
2 and decompresses this file back into a .ppm image.

Solution Architecture:
40image.c handles command line arguments and calls compress40() or 
decompress40() accordingly. 
The functions in compress40.c are responsible for calling a series of functions,
each of which returns data that is used by the next function:
    + rgb_to_cv.h contains functions to convert between the rgb and component
      video pixel formats
    + cv_to_word.h contains functions that convert between component video
      pixels and an array of "words". This interface can also read/write
      these words in the COMP40 Compressed image format 2.
    + bitpack.h contains functions that manipulate bits

Note that we decided to use a denominator of 255 for our decompression. This is
because this denominator ensures that the decompressed file is not larger than 
the original uncompressed file. 255 is also the most commonly used denominator.

