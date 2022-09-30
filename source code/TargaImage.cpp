///////////////////////////////////////////////////////////////////////////////
//
//      TargaImage.cpp                          Author:     Stephen Chenney
//                                              Modified:   Eric McDaniel
//                                              Date:       Fall 2004
//
//      Implementation of TargaImage methods.  You must implement the image
//  modification functions.
//
///////////////////////////////////////////////////////////////////////////////

#include "Globals.h"
#include "TargaImage.h"
#include "libtarga.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <random>

using namespace std;

// constants
const int           RED = 0;                // red channel
const int           GREEN = 1;                // green channel
const int           BLUE = 2;                // blue channel
const unsigned char BACKGROUND[3] = { 0, 0, 0 };      // background color


// Computes n choose s, efficiently
double Binomial(int n, int s)
{
    double        res;

    res = 1;
    for (int i = 1; i <= s; i++)
        res = (n - i + 1) * res / i;

    return res;
}// Binomial


///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage() : width(0), height(0), data(NULL)
{}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h) : width(w), height(h)
{
    data = new unsigned char[width * height * 4];
    ClearToBlack();
}// TargaImage



///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables to values given.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h, unsigned char* d)
{
    int i;

    width = w;
    height = h;
    data = new unsigned char[width * height * 4];

    for (i = 0; i < width * height * 4; i++)
        data[i] = d[i];
}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Copy Constructor.  Initialize member to that of input
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(const TargaImage& image)
{
    width = image.width;
    height = image.height;
    data = NULL;
    if (image.data != NULL) {
        data = new unsigned char[width * height * 4];
        memcpy(data, image.data, sizeof(unsigned char) * width * height * 4);
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Destructor.  Free image memory.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::~TargaImage()
{
    if (data)
        delete[] data;
}// ~TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Converts an image to RGB form, and returns the rgb pixel data - 24 
//  bits per pixel. The returned space should be deleted when no longer 
//  required.
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::To_RGB(void)
{
    unsigned char* rgb = new unsigned char[width * height * 3];
    int		    i, j;

    if (!data)
        return NULL;

    // Divide out the alpha
    for (i = 0; i < height; i++)
    {
        int in_offset = i * width * 4;
        int out_offset = i * width * 3;

        for (j = 0; j < width; j++)
        {
            RGBA_To_RGB(data + (in_offset + j * 4), rgb + (out_offset + j * 3));
        }
    }

    return rgb;
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Save the image to a targa file. Returns 1 on success, 0 on failure.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Save_Image(const char* filename)
{
    TargaImage* out_image = Reverse_Rows();

    if (!out_image)
        return false;

    if (!tga_write_raw(filename, width, height, out_image->data, TGA_TRUECOLOR_32))
    {
        cout << "TGA Save Error: %s\n", tga_error_string(tga_get_last_error());
        return false;
    }

    delete out_image;

    return true;
}// Save_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Load a targa image from a file.  Return a new TargaImage object which 
//  must be deleted by caller.  Return NULL on failure.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Load_Image(char* filename)
{
    unsigned char* temp_data;
    TargaImage* temp_image;
    TargaImage* result;
    int		        width, height;

    if (!filename)
    {
        cout << "No filename given." << endl;
        return NULL;
    }// if

    temp_data = (unsigned char*)tga_load(filename, &width, &height, TGA_TRUECOLOR_32);
    if (!temp_data)
    {
        cout << "TGA Error: %s\n", tga_error_string(tga_get_last_error());
        width = height = 0;
        return NULL;
    }
    temp_image = new TargaImage(width, height, temp_data);
    free(temp_data);

    result = temp_image->Reverse_Rows();

    delete temp_image;

    return result;
}// Load_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Convert image to grayscale.  Red, green, and blue channels should all 
//  contain grayscale value.  Alpha channel shoould be left unchanged.  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::To_Grayscale()
{
    unsigned char* p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            float Luminance = 0.30 * (float)p_data[0] + 0.59 * (float)p_data[1] + 0.11 * (float)p_data[2];
            p_data[0] = p_data[1] = p_data[2] = (unsigned char)Luminance;
            p_data = p_data + 4;
        }
    }

    return false;
}// To_Grayscale


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using uniform quantization.  Return 
//  success of operation. (R  R  R  G  G  G  B  B)
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Uniform()
{
    uint8_t* p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_data[0] = (p_data[0] >> 5) * 36.42857; // 1110_0000
            p_data[1] = (p_data[1] >> 5) * 36.42857; // 1110_0000
            p_data[2] = (p_data[2] >> 6) * 85; // 1100_0000
            p_data = p_data + 4;
        }
    }

    return false;
}// Quant_Uniform


///////////////////////////////////////////////////////////////////////////////
//
//      Convert the image to an 8 bit image using populosity quantization.  
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Populosity()
{
    uint8_t* p_data;

    // histogram
    std::vector<unsigned int> histogram;
    std::vector<uint16_t> indices;
    histogram.resize(32768);
    indices.resize(32768);
    for (unsigned int i = 0; i < 32768; i++) {
        histogram[i] = 0; // value
        indices[i] = i; // index
    }

    p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            // uniform quantization
            uint8_t color_r = p_data[0] >> 3;
            uint8_t color_g = p_data[1] >> 3;
            uint8_t color_b = p_data[2] >> 3;
            // count histogram
            uint16_t color = (color_r << 10) | (color_g << 5) | (color_b << 0);
            histogram[color] = histogram[color] + 1;
            p_data = p_data + 4;
        }
    }

    // sort
    std::sort(indices.begin(), indices.end(),
        [&histogram](size_t i1, size_t i2) {return histogram[i1] > histogram[i2]; });

    //
    p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            float inColorR = (float)p_data[0] / 255.0f;
            float inColorG = (float)p_data[1] / 255.0f;
            float inColorB = (float)p_data[2] / 255.0f;

            uint16_t minDistanceIndex = 0;// color, idx of histogram
            float minDistance = FLT_MAX;
            for (unsigned int i = 0; i < 256; i++) {
                float targetColorR = (float)((indices[i] >> 10) & 0x001f) / 31.0f;
                float targetColorG = (float)((indices[i] >> 5) & 0x001f) / 31.0f;
                float targetColorB = (float)((indices[i] >> 0) & 0x001f) / 31.0f;

                float newDistance = powf((targetColorR - inColorR), 2) +
                    powf((targetColorG - inColorG), 2) +
                    powf((targetColorB - inColorB), 2);

                if (newDistance < minDistance) {
                    minDistance = newDistance;
                    minDistanceIndex = i;
                }
            }

            uint16_t newColor = indices[minDistanceIndex];
            p_data[0] = ((newColor >> 10) & 0x001f) << 3;
            p_data[1] = ((newColor >> 5) & 0x001f) << 3;
            p_data[2] = ((newColor >> 0) & 0x001f) << 3;

            p_data = p_data + 4;
        }
    }

    return false;
}// Quant_Populosity


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image using a threshold of 1/2.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Threshold()
{
    To_Grayscale();

    const uint8_t threshold = 128;
    uint8_t* p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_data[0] = (p_data[0] > threshold) ? 255 : 0;
            p_data[1] = (p_data[1] > threshold) ? 255 : 0;
            p_data[2] = (p_data[2] > threshold) ? 255 : 0;
            p_data = p_data + 4;
        }
    }
    return false;
}// Dither_Threshold


///////////////////////////////////////////////////////////////////////////////
//
//      Dither image using random dithering.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Random()
{
    To_Grayscale();

    const uint8_t threshold = 128;
    const int uniformRange = 256 * 0.2;
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(-uniformRange, uniformRange);
    uint8_t* p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int randVal = distribution(generator);
            p_data[0] = (((int)p_data[0] + randVal) > threshold) ? 255 : 0;
            p_data[1] = (((int)p_data[1] + randVal) > threshold) ? 255 : 0;
            p_data[2] = (((int)p_data[2] + randVal) > threshold) ? 255 : 0;
            p_data = p_data + 4;
        }
    }
    return false;
}// Dither_Random


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS()
{
    To_Grayscale();

    float* fdata = new float[width * height * 3];

    float* p_fdata = fdata;
    uint8_t* p_data = data;

    // convert to float array
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_fdata[0] = p_data[0] / 255.0f;
            p_fdata[1] = p_data[1] / 255.0f;
            p_fdata[2] = p_data[2] / 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    // Floyd-Steinberg

    const float threshold = 0.5f;
    for (int color_i = 0; color_i < 3; color_i++) {
        float* p_fdata = fdata + color_i;
        int y = 0, x = 0;
        int stepX = 1;
        while (1) {
            // next pixel
            if (x == width - 1) {
                stepX = -1;
                y = y + 1;
            }
            if (x == 0) {
                stepX = 1;
                y = y + 1;
            }
            if (y >= height) break;

            x = x + stepX;



            //
            unsigned int pixelIdx = y * width + x;
            float newColor = (p_fdata[pixelIdx * 3] > threshold) ? 1.0f : 0.0f;
            float error = p_fdata[pixelIdx * 3] - newColor;

            unsigned int neighborIdx = pixelIdx + stepX;
            if (neighborIdx < width * height)
                p_fdata[neighborIdx * 3] += 7.0f / 16.0f * error;
            neighborIdx = pixelIdx + stepX + width;
            if (neighborIdx < width * height)
                p_fdata[neighborIdx * 3] += 1.0f / 16.0f * error;
            neighborIdx = pixelIdx + width;
            if (neighborIdx < width * height)
                p_fdata[neighborIdx * 3] += 5.0f / 16.0f * error;
            neighborIdx = pixelIdx - stepX + width;
            if (neighborIdx < width * height)
                p_fdata[neighborIdx * 3] += 3.0f / 16.0f * error;

            p_fdata[pixelIdx * 3] = newColor;
        }
    }


    // copy data
    p_fdata = fdata;
    p_data = data;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            p_data[0] = p_fdata[0] * 255.0f;
            p_data[1] = p_fdata[1] * 255.0f;
            p_data[2] = p_fdata[2] * 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    delete[]fdata;
    return false;
}// Dither_FS


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image while conserving the average brightness.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Bright()
{
    To_Grayscale();

    uint8_t* p_data = data;

    // histogram
    for (unsigned int color_i = 0; color_i < 3; color_i++) {
        double avgVal = 0.0;

        p_data = data;
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                avgVal = avgVal + p_data[color_i];
                p_data = p_data + 4;
            }
        }
        avgVal = avgVal / (float)(width * height);

        // threshold
        unsigned int threshold = avgVal;

        std::cout << "threshold : " << threshold << std::endl;

        // dither
        p_data = data;
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                p_data[color_i] = (p_data[color_i] > threshold) ? 255 : 0;
                p_data = p_data + 4;
            }
        }
    }
    return false;
}// Dither_Bright


///////////////////////////////////////////////////////////////////////////////
//
//      Perform clustered differing of the image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Cluster()
{
    To_Grayscale();

    const uint8_t mask[4][4] = { {180, 90, 150, 60},
                                 {15, 240, 210, 105},
                                 {120, 195, 225, 30},
                                 {45, 135, 75, 165},
    };
    uint8_t* p_data = data;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            p_data[0] = (p_data[0] > mask[y % 4][x % 4]) ? 255 : 0;
            p_data[1] = (p_data[1] > mask[y % 4][x % 4]) ? 255 : 0;
            p_data[2] = (p_data[2] > mask[y % 4][x % 4]) ? 255 : 0;
            p_data = p_data + 4;
        }
    }
    return false;
}// Dither_Cluster


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using Floyd-Steinberg dithering over
//  a uniform quantization - the same quantization as in Quant_Uniform.
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Color()
{
    float* fdata = new float[width * height * 3];

    float* p_fdata = fdata;
    uint8_t* p_data = data;

    // convert to float array
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_fdata[0] = p_data[0] / 255.0f;
            p_fdata[1] = p_data[1] / 255.0f;
            p_fdata[2] = p_data[2] / 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    // Floyd-Steinberg

    const float threshold = 0.5f;
    for (int color_i = 0; color_i < 3; color_i++) {
        float* p_fdata = fdata + color_i;
        int y = 0, x = 0;
        int stepX = 1;
        while (1) {
            // next pixel
            if (x == width - 1) {
                stepX = -1;
                y = y + 1;
            }
            if (x == 0) {
                stepX = 1;
                y = y + 1;
            }
            if (y >= height) break;

            x = x + stepX;



            //
            unsigned int pixelIdx = y * width + x;
            float newColor = p_fdata[pixelIdx * 3];
            if (color_i == 0)//R, 3bits
                newColor = floor(newColor * 8.0f) / 8.0f;
            if (color_i == 1)//G, 3bits
                newColor = floor(newColor * 8.0f) / 8.0f;
            if (color_i == 2)//B, 2bits
                newColor = floor(newColor * 4.0f) / 4.0f;
            float error = p_fdata[pixelIdx * 3] - newColor;

            unsigned int neighborIdx = pixelIdx + stepX;
            if (neighborIdx < width * height)
                p_fdata[neighborIdx * 3] += 7.0f / 16.0f * error;
            neighborIdx = pixelIdx + stepX + width;
            if (neighborIdx < width * height)
                p_fdata[neighborIdx * 3] += 1.0f / 16.0f * error;
            neighborIdx = pixelIdx + width;
            if (neighborIdx < width * height)
                p_fdata[neighborIdx * 3] += 5.0f / 16.0f * error;
            neighborIdx = pixelIdx - stepX + width;
            if (neighborIdx < width * height)
                p_fdata[neighborIdx * 3] += 3.0f / 16.0f * error;

            p_fdata[pixelIdx * 3] = newColor;
        }
    }


    // copy data
    p_fdata = fdata;
    p_data = data;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            p_data[0] = p_fdata[0] * 255.0f;
            p_data[1] = p_fdata[1] * 255.0f;
            p_data[2] = p_fdata[2] * 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    delete[]fdata;
    return false;
}// Dither_Color


///////////////////////////////////////////////////////////////////////////////
//
//      Composite the current image over the given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Over(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Over: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Over


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "in" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_In(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_In: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_In


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "out" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Out(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Out: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Out


///////////////////////////////////////////////////////////////////////////////
//
//      Composite current image "atop" given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Atop(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Atop: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Atop


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image with given image using exclusive or (XOR).  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Xor(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Xor: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Xor


///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the difference bewteen this imag and the given one.  Image 
//  dimensions must be equal.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Difference(TargaImage* pImage)
{
    if (!pImage)
        return false;

    if (width != pImage->width || height != pImage->height)
    {
        cout << "Difference: Images not the same size\n";
        return false;
    }// if

    for (int i = 0; i < width * height * 4; i += 4)
    {
        unsigned char        rgb1[3];
        unsigned char        rgb2[3];

        RGBA_To_RGB(data + i, rgb1);
        RGBA_To_RGB(pImage->data + i, rgb2);

        data[i] = abs(rgb1[0] - rgb2[0]);
        data[i + 1] = abs(rgb1[1] - rgb2[1]);
        data[i + 2] = abs(rgb1[2] - rgb2[2]);
        data[i + 3] = 255;
    }

    return true;
}// Difference


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 box filter on this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Box()
{
    float mask[5][5] = { {1, 1, 1, 1, 1},
                         {1, 1, 1, 1, 1},
                         {1, 1, 1, 1, 1},
                         {1, 1, 1, 1, 1},
                         {1, 1, 1, 1, 1},
    };
    // normalize filter
    float maskSum = 0.0;
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            maskSum += mask[y][x];
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            mask[y][x] = mask[y][x] / maskSum;

    float* fdata = new float[width * height * 3];

    float* p_fdata = fdata;
    uint8_t* p_data = data;

    // convert to float array
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_fdata[0] = p_data[0] / 255.0f;
            p_fdata[1] = p_data[1] / 255.0f;
            p_fdata[2] = p_data[2] / 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    for (int color_i = 0; color_i < 3; color_i++) {
        // for each pixel
        p_fdata = fdata + color_i;
        p_data = data + color_i;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {

                float newVal = 0.0;
                // by element multiply 
                for (int mask_y = -2; mask_y <= 2; mask_y++) {
                    if ((y + mask_y) >= height || (y + mask_y) < 0) continue;
                    for (int mask_x = -2; mask_x <= 2; mask_x++) {
                        if ((x + mask_x) >= width || (x + mask_x) < 0) continue;
                        unsigned int idx = (y + mask_y) * width + (x + mask_x);
                        newVal += p_fdata[idx * 3] * mask[mask_y + 2][mask_x + 2];
                    }
                }

                unsigned int idx = y * width + x;
                p_data[idx * 4] = newVal * 255.0f;
            }
        }

    }

    delete[]fdata;
    return false;
}// Filter_Box


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett()
{
    float mask[5][5] = { {1, 2, 3, 2, 1},
                         {2, 4, 6, 4, 2},
                         {3, 6, 9, 6, 3},
                         {2, 4, 6, 4, 2},
                         {1, 2, 3, 2, 1},
    };
    // normalize filter
    float maskSum = 0.0;
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            maskSum += mask[y][x];
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            mask[y][x] = mask[y][x] / maskSum;

    float* fdata = new float[width * height * 3];

    float* p_fdata = fdata;
    uint8_t* p_data = data;

    // convert to float array
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_fdata[0] = p_data[0] / 255.0f;
            p_fdata[1] = p_data[1] / 255.0f;
            p_fdata[2] = p_data[2] / 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    for (int color_i = 0; color_i < 3; color_i++) {
        // for each pixel
        p_fdata = fdata + color_i;
        p_data = data + color_i;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {

                float newVal = 0.0;
                // by element multiply 
                for (int mask_y = -2; mask_y <= 2; mask_y++) {
                    if ((y + mask_y) >= height || (y + mask_y) < 0) continue;
                    for (int mask_x = -2; mask_x <= 2; mask_x++) {
                        if ((x + mask_x) >= width || (x + mask_x) < 0) continue;
                        unsigned int idx = (y + mask_y) * width + (x + mask_x);
                        newVal += p_fdata[idx * 3] * mask[mask_y + 2][mask_x + 2];
                    }
                }

                unsigned int idx = y * width + x;
                p_data[idx * 4] = newVal * 255.0f;
            }
        }

    }

    delete[]fdata;
    return false;
}// Filter_Bartlett


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Gaussian()
{
    float mask[5][5] = { {1, 4, 7, 4, 1},
                         {4, 16, 26, 16, 4},
                         {7, 26, 41, 26, 7},
                         {4, 16, 26, 16, 4},
                         {1, 4, 7, 4, 1},
    };
    // normalize filter
    float maskSum = 0.0;
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            maskSum += mask[y][x];
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 5; x++)
            mask[y][x] = mask[y][x] / maskSum;

    float* fdata = new float[width * height * 3];

    float* p_fdata = fdata;
    uint8_t* p_data = data;

    // convert to float array
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_fdata[0] = p_data[0] / 255.0f;
            p_fdata[1] = p_data[1] / 255.0f;
            p_fdata[2] = p_data[2] / 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    for (int color_i = 0; color_i < 3; color_i++) {
        // for each pixel
        p_fdata = fdata + color_i;
        p_data = data + color_i;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {

                float newVal = 0.0;
                // by element multiply 
                for (int mask_y = -2; mask_y <= 2; mask_y++) {
                    if ((y + mask_y) >= height || (y + mask_y) < 0) continue;
                    for (int mask_x = -2; mask_x <= 2; mask_x++) {
                        if ((x + mask_x) >= width || (x + mask_x) < 0) continue;
                        unsigned int idx = (y + mask_y) * width + (x + mask_x);
                        newVal += p_fdata[idx * 3] * mask[mask_y + 2][mask_x + 2];
                    }
                }

                unsigned int idx = y * width + x;
                p_data[idx * 4] = newVal * 255.0f;
            }
        }

    }

    delete[]fdata;
    return false;
}// Filter_Gaussian

///////////////////////////////////////////////////////////////////////////////
//
//      Perform NxN Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Gaussian_N(unsigned int N)
{
    std::vector<std::vector <float>> pascal;
    pascal.push_back(std::vector<float>(1, 1.0f));
    pascal.push_back(std::vector<float>(2, 1.0f));

    for (int i = 2; i <= N - 1 ; i++) {
        std::vector <float> newRow;
        newRow.push_back(1.0f);
        for (int j = 1; j < i; j++) {
            newRow.push_back(pascal[i-1][j] + pascal[i-1][j - 1]);
        }
        newRow.push_back(1.0f);
        pascal.push_back(newRow);
    }

    //for (std::vector <float> row : pascal) {
    //    for (float val : row) {
    //        std::cout << val << ",\t";
    //    }
    //    std::cout << std::endl;
    //}
/*
0 |1
1 |1  1
2 |1  2  1
3 |1  3  3  1
4 |1  4  6  4  1
5 |1  5 10 10  5  1
6 |1  6 15 20 15  6  1
7 |1  7 21 35 35 21  7  1
8 |1  8 28 56 70 56 28  8  1
...
*/

    float* mask = new float[N * N];
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            mask[i * N + j] = pascal[N - 1][i] * pascal[N - 1][j];
        }
    }

    //for (int i = 0; i < N; i++) {
    //    for (int j = 0; j < N; j++) {
    //        std::cout << mask[i * N + j] << "\t";
    //    }
    //    std::cout << std::endl;
    //}

    // normalize filter
    float maskSum = 0.0;
    for (int i = 0; i < N * N; i++)
        maskSum += mask[i];
    for (int i = 0; i < N * N; i++)
        mask[i] /= maskSum;


    // copy data to float array
    float* fdata = new float[width * height * 3];

    float* p_fdata = fdata;
    uint8_t* p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_fdata[0] = p_data[0] / 255.0f;
            p_fdata[1] = p_data[1] / 255.0f;
            p_fdata[2] = p_data[2] / 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    // convolution 2d
    float* result = new float[width * height * 3];
    for (int colori = 0; colori < 3; colori++) { // for each color
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixelIdx = (y * width + x);

                float newVal = 0.0f;
                for (int i = 0; i < N; i++) {
                    int maxk_y = i - (N / 2);
                    int inIdxY = y + maxk_y;
                    if (inIdxY < 0 || inIdxY >= height) continue;
                    for (int j = 0; j < N; j++) {
                        int maxk_x = j - (N / 2);

                        int inIdxY = y + maxk_y;
                        int inIdxX = x + maxk_x;
                        if (inIdxX < 0 || inIdxX >= width) continue;

                        int inIdx = (inIdxY * width + inIdxX);
                        newVal += fdata[inIdx * 3 + colori] * mask[i * N + j];
                    }
                }
                result[pixelIdx * 3 + colori] = newVal;
            }
        }
    }

    // copy data
    p_fdata = result;
    p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_data[0] = p_fdata[0] * 255.0f;
            p_data[1] = p_fdata[1] * 255.0f;
            p_data[2] = p_fdata[2] * 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    delete[] mask;
    delete[] result;
    delete[] fdata;

    return false;
}// Filter_Gaussian_N


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 edge detect (high pass) filter on this image.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Edge()
{
    int N = 5;
    float mask[5 * 5] = {1, 4, 7, 4, 1,
                         4, 16, 26, 16, 4,
                         7, 26, 41, 26, 7,
                         4, 16, 26, 16, 4,
                         1, 4, 7, 4, 1,
    };
    // normalize filter
    float maskSum = 0.0;
    for (int i = 0; i < N * N; i++)
        maskSum += mask[i];
    for (int i = 0; i < N * N; i++)
        mask[i] /= maskSum;
    // cvt heigh filter
    for (int i = 0; i < N * N; i++)
        mask[i] = -mask[i];
    mask[N * N / 2] += 1.0f;

    // copy data to float array
    float* fdata = new float[width * height * 3];

    float* p_fdata = fdata;
    uint8_t* p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_fdata[0] = p_data[0] / 255.0f;
            p_fdata[1] = p_data[1] / 255.0f;
            p_fdata[2] = p_data[2] / 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    // convolution 2d
    float* result = new float[width * height * 3];
    for (int colori = 0; colori < 3; colori++) { // for each color
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixelIdx = (y * width + x);

                float newVal = 0.0f;
                for (int i = 0; i < N; i++) {
                    int maxk_y = i - (N / 2);
                    int inIdxY = y + maxk_y;
                    if (inIdxY < 0 || inIdxY >= height) continue;
                    for (int j = 0; j < N; j++) {
                        int maxk_x = j - (N / 2);

                        int inIdxY = y + maxk_y;
                        int inIdxX = x + maxk_x;
                        if (inIdxX < 0 || inIdxX >= width) continue;

                        int inIdx = (inIdxY * width + inIdxX);
                        newVal += fdata[inIdx * 3 + colori] * mask[i * N + j];
                    }
                }
                newVal = newVal + 0.5f;
                if (newVal < 0.0f) newVal = 0.0f;
                if (newVal > 1.0f) newVal = 1.0f;
                result[pixelIdx * 3 + colori] = newVal;
            }
        }
    }

    // copy data
    p_fdata = result;
    p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            for (int colori = 0; colori < 3; colori++) {
                int newVal = p_fdata[0 + colori] * 255.0f;
                p_data[0 + colori] = newVal;
            }
            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    delete[] result;
    delete[] fdata;

    return false;
}// Filter_Edge


///////////////////////////////////////////////////////////////////////////////
//
//      Perform a 5x5 enhancement filter to this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Enhance()
{
    int N = 5;
    float mask[5 * 5] = { 1, 4, 7, 4, 1,
                         4, 16, 26, 16, 4,
                         7, 26, 41, 26, 7,
                         4, 16, 26, 16, 4,
                         1, 4, 7, 4, 1,
    };
    // normalize filter
    float maskSum = 0.0;
    for (int i = 0; i < N * N; i++)
        maskSum += mask[i];
    for (int i = 0; i < N * N; i++)
        mask[i] /= maskSum;
    // cvt heigh filter
    for (int i = 0; i < N * N; i++)
        mask[i] = -mask[i];
    mask[N * N / 2] += 2.0f;

    // copy data to float array
    float* fdata = new float[width * height * 3];

    float* p_fdata = fdata;
    uint8_t* p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_fdata[0] = p_data[0] / 255.0f;
            p_fdata[1] = p_data[1] / 255.0f;
            p_fdata[2] = p_data[2] / 255.0f;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    // convolution 2d
    float* result = new float[width * height * 3];
    for (int colori = 0; colori < 3; colori++) { // for each color
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixelIdx = (y * width + x);

                float newVal = 0.0f;
                for (int i = 0; i < N; i++) {
                    int maxk_y = i - (N / 2);
                    int inIdxY = y + maxk_y;
                    if (inIdxY < 0 || inIdxY >= height) continue;
                    for (int j = 0; j < N; j++) {
                        int maxk_x = j - (N / 2);

                        int inIdxY = y + maxk_y;
                        int inIdxX = x + maxk_x;
                        if (inIdxX < 0 || inIdxX >= width) continue;

                        int inIdx = (inIdxY * width + inIdxX);
                        newVal += fdata[inIdx * 3 + colori] * mask[i * N + j];
                    }
                }
                if (newVal < 0.0f) newVal = 0.0f;
                if (newVal > 1.0f) newVal = 1.0f;
                result[pixelIdx * 3 + colori] = newVal;
            }
        }
    }

    // copy data
    p_fdata = result;
    p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            for (int colori = 0; colori < 3; colori++) {
                int newVal = p_fdata[0 + colori] * 255.0f;
                p_data[0 + colori] = newVal;
            }
            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    delete[] result;
    delete[] fdata;

    return false;
}// Filter_Enhance


///////////////////////////////////////////////////////////////////////////////
//
//      Run simplified version of Hertzmann's painterly image filter.
//      You probably will want to use the Draw_Stroke funciton and the
//      Stroke class to help.
// Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::NPR_Paint()
{

    float* fdata = new float[width * height * 3];
    float* p_fdata = fdata;
    uint8_t* p_data = data;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            p_fdata[0] = (float)p_data[0];
            p_fdata[1] = (float)p_data[1];
            p_fdata[2] = (float)p_data[2];

            p_data[0] = 0xff;
            p_data[1] = 0xff;
            p_data[2] = 0xff;

            p_data = p_data + 4;
            p_fdata = p_fdata + 3;
        }
    }

    // from Biggest brush to finest
    int brushSizeStep[] = { 100, 40, 10, 4 , 2};
    float drawBrushsProportion[] = { 1.0, 1.0, 1.0, 1.0 , 1.0};
    for (int brushSizeIdx = 0; brushSizeIdx < sizeof(brushSizeStep)/sizeof(brushSizeStep[0]); brushSizeIdx++) {
        int brushSize = brushSizeStep[brushSizeIdx];
        
        // averaging computation
        std::vector<Stroke> strokes;
        std::default_random_engine generator;
        std::uniform_int_distribution<int> PosDist(-10, 10);
        std::uniform_real_distribution<float> brushSizeDist(0.7, 1.2);
        for (int y = brushSize * 0.5; y < height; y += brushSize) {
            for (int x = brushSize * 0.5; x < width; x += brushSize) {
                int currBrushSize = brushSize * brushSizeDist(generator);
                // Randomize the averaging window position
                int xOffset = PosDist(generator);
                int yOffset = PosDist(generator);

                // summation
                const int summationRange = currBrushSize;
                int summationRangeBeginX = x + xOffset - summationRange;
                if (summationRangeBeginX < 0) summationRangeBeginX = 0;
                if (summationRangeBeginX > width) summationRangeBeginX = width;
                int summationRangeEndX = x + xOffset + summationRange;
                if (summationRangeEndX < 0) summationRangeEndX = 0;
                if (summationRangeEndX > width) summationRangeEndX = width;
                int summationRangeBeginY = y + yOffset - summationRange;
                if (summationRangeBeginY < 0) summationRangeBeginY = 0;
                if (summationRangeBeginY > height) summationRangeBeginY = height;
                int summationRangeEndY = y + yOffset + summationRange;
                if (summationRangeEndY < 0) summationRangeEndY = 0;
                if (summationRangeEndY > height) summationRangeEndY = height;
                float sumR = 0.0f;
                float sumG = 0.0f;
                float sumB = 0.0f;
                for (int sumIdxY = summationRangeBeginY; sumIdxY < summationRangeEndY; sumIdxY++) {
                    for (int sumIdxX = summationRangeBeginX; sumIdxX < summationRangeEndX; sumIdxX++) {
                        sumR += fdata[(sumIdxY * width + sumIdxX) * 3 + 0];
                        sumG += fdata[(sumIdxY * width + sumIdxX) * 3 + 1];
                        sumB += fdata[(sumIdxY * width + sumIdxX) * 3 + 2];
                    }
                }

                // averaging
                int totalPixelInRange = (summationRangeEndY - summationRangeBeginY) * (summationRangeEndX - summationRangeBeginX);
                float avgR = sumR / (float)totalPixelInRange;
                float avgG = sumG / (float)totalPixelInRange;
                float avgB = sumB / (float)totalPixelInRange;

                Stroke newStroke((unsigned int)currBrushSize, x + xOffset, y + yOffset,
                                 (unsigned char)avgR, (unsigned char)avgG, (unsigned char)avgB, 255);
                strokes.push_back(newStroke);
            }
        }

        // Randomize the order
        std::random_shuffle(strokes.begin(), strokes.end());
        int strokesRange = strokes.size();
        if (brushSizeIdx > 0) strokesRange = strokesRange * drawBrushsProportion[brushSizeIdx];
        for (int strokeIdx = 0; strokeIdx < strokesRange; strokeIdx++)
            Paint_Stroke(strokes[strokeIdx]);

    }
    
    delete[]fdata;

    return false;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Halve the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Half_Size()
{
    std::cout << width << "," << height << std::endl;
    float mask[3][3] = { {1 ,2 ,1},
                         {2 ,4 ,2},
                         {1 ,2 ,1}
    };
    // normalize filter
    float maskSum = 0.0;
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            maskSum += mask[y][x];
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 3; x++)
            mask[y][x] = mask[y][x] / maskSum;

    int newHeight = height / 2;
    int newWidth = width / 2;
    unsigned char* newData = new unsigned char[newWidth * newHeight * 4];

    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {

            // transform
            int srcY = y * 2;
            int srcX = x * 2;

            for (int color = 0; color < 3; color++) {
                float newVal = 0.0f;
                // convulation
                for (int maskY = 0; maskY < 3; maskY++) {
                    for (int maskX = 0; maskX < 3; maskX++) {
                        int idxY = srcY + maskY - 1;
                        int idxX = srcX + maskX - 1;
                        if (idxY<0 || idxY > height) continue;
                        if (idxX<0 || idxX > width) continue;
                        newVal += mask[maskY][maskX] * (float)data[(idxY * width + idxX) * 4 + color];
                    }
                }
                newData[(y * newWidth + x) * 4 + color] = newVal;
            }
            newData[(y * newWidth + x) * 4 + 3] = 255;
        }
    }

    delete[]data;
    width = newWidth;
    height = newHeight;
    data = newData;
    return false;
}// Half_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Double the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Double_Size()
{
    float filter3[3] = { 1 ,2 ,1 };
    float filter4[4] = { 1 ,3 ,3 ,1 };
    float filterNorm[3] = { 0.0625 , 0.03125 ,0.015625 };

    int newHeight = height * 2;
    int newWidth = width * 2;
    unsigned char* newData = new unsigned char[newWidth * newHeight * 4];

    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {

            // transform
            int srcY = y / 2;
            int srcX = x / 2;

            for (int color = 0; color < 3; color++) {
                float newVal = 0.0f;
                // convulation
                for (int maskY = 0; maskY < ((y%2==0)? 3:4); maskY++) {
                    for (int maskX = 0; maskX < ((x%2==0)? 3:4); maskX++) {
                        int idxY = srcY + maskY - 1;
                        int idxX = srcX + maskX - 1;
                        if (idxY < 0 || idxY >= height) continue;
                        if (idxX < 0 || idxX >= width) continue;
                        float falterVal = 1.0f;
                        falterVal *= (y % 2 == 0) ? filter3[maskY] : filter4[maskY];
                        falterVal *= (x % 2 == 0) ? filter3[maskX] : filter4[maskX];
                        newVal += falterVal * (float)data[(idxY * width + idxX) * 4 + color];
                    }
                }
                newVal = newVal * filterNorm[(y & 1) + (x & 1)];
                newData[(y * newWidth + x) * 4 + color] = newVal;
            }
            newData[(y * newWidth + x) * 4 + 3] = 255;
        }
    }

    delete[]data;
    width = newWidth;
    height = newHeight;
    data = newData;
    return false;
}// Double_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Scale the image dimensions by the given factor.  The given factor is 
//  assumed to be greater than one.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Resize(float scale)
{
    float mask[4][4] = { {1 ,3 ,3 ,1},
                         {3 ,9 ,9 ,3},
                         {3 ,9 ,9 ,3},
                         {1 ,3 ,3 ,1},
    };
    // normalize filter
    float maskSum = 0.0;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            maskSum += mask[y][x];
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            mask[y][x] = mask[y][x] / maskSum;

    int newHeight = height * scale;
    int newWidth = width * scale;
    unsigned char* newData = new unsigned char[newWidth * newHeight * 4];

    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {

            // transform
            int srcY = y / scale;
            int srcX = x / scale;

            for (int color = 0; color < 3; color++) {
                float newVal = 0.0f;
                // convulation
                for (int maskY = 0; maskY < 4; maskY++) {
                    for (int maskX = 0; maskX < 4; maskX++) {
                        int idxY = srcY + maskY - 1;
                        int idxX = srcX + maskX - 1;
                        if (idxY < 0 || idxY >= height) continue;
                        if (idxX < 0 || idxX >= width) continue;
                        newVal += mask[maskY][maskX] * (float)data[(idxY * width + idxX) * 4 + color];
                    }
                }
                newData[(y * newWidth + x) * 4 + color] = newVal;
            }
            newData[(y * newWidth + x) * 4 + 3] = 255;
        }
    }

    delete[]data;
    width = newWidth;
    height = newHeight;
    data = newData;
    return false;
}// Resize


//////////////////////////////////////////////////////////////////////////////
//
//      Rotate the image clockwise by the given angle.  Do not resize the 
//  image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Rotate(float angleDegrees)
{
    float radian = angleDegrees * c_pi / 180.0f;
    float mask[4][4] = { {1 ,3 ,3 ,1},
                         {3 ,9 ,9 ,3},
                         {3 ,9 ,9 ,3},
                         {1 ,3 ,3 ,1},
    };
    // normalize filter
    float maskSum = 0.0;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            maskSum += mask[y][x];
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            mask[y][x] = mask[y][x] / maskSum;

    int newHeight = height;
    int newWidth = width;
    unsigned char* newData = new unsigned char[newWidth * newHeight * 4];

    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {

            // transform
            int srcX = x * cos(-radian) - y * sin(-radian);
            int srcY = x * sin(-radian) + y * cos(-radian);



            for (int color = 0; color < 3; color++) {
                float newVal = 0.0f;
                // convulation
                for (int maskY = 0; maskY < 4; maskY++) {
                    for (int maskX = 0; maskX < 4; maskX++) {
                        int idxY = srcY + maskY - 1;
                        int idxX = srcX + maskX - 1;
                        if (idxY < 0 || idxY >= height) continue;
                        if (idxX < 0 || idxX >= width) continue;
                        newVal += mask[maskY][maskX] * (float)data[(idxY * width + idxX) * 4 + color];
                    }
                }
                newData[(y * newWidth + x) * 4 + color] = newVal;
            }
            newData[(y * newWidth + x) * 4 + 3] = 255;
        }
    }

    delete[]data;
    width = newWidth;
    height = newHeight;
    data = newData;
    return false;
}// Rotate


//////////////////////////////////////////////////////////////////////////////
//
//      Given a single RGBA pixel return, via the second argument, the RGB
//      equivalent composited with a black background.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::RGBA_To_RGB(unsigned char* rgba, unsigned char* rgb)
{
    const unsigned char	BACKGROUND[3] = { 0, 0, 0 };

    unsigned char  alpha = rgba[3];

    if (alpha == 0)
    {
        rgb[0] = BACKGROUND[0];
        rgb[1] = BACKGROUND[1];
        rgb[2] = BACKGROUND[2];
    }
    else
    {
        float	alpha_scale = (float)255 / (float)alpha;
        int	val;
        int	i;

        for (i = 0; i < 3; i++)
        {
            val = (int)floor(rgba[i] * alpha_scale);
            if (val < 0)
                rgb[i] = 0;
            else if (val > 255)
                rgb[i] = 255;
            else
                rgb[i] = val;
        }
    }
}// RGA_To_RGB


///////////////////////////////////////////////////////////////////////////////
//
//      Copy this into a new image, reversing the rows as it goes. A pointer
//  to the new image is returned.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Reverse_Rows(void)
{
    unsigned char* dest = new unsigned char[width * height * 4];
    TargaImage* result;
    int 	        i, j;

    if (!data)
        return NULL;

    for (i = 0; i < height; i++)
    {
        int in_offset = (height - i - 1) * width * 4;
        int out_offset = i * width * 4;

        for (j = 0; j < width; j++)
        {
            dest[out_offset + j * 4] = data[in_offset + j * 4];
            dest[out_offset + j * 4 + 1] = data[in_offset + j * 4 + 1];
            dest[out_offset + j * 4 + 2] = data[in_offset + j * 4 + 2];
            dest[out_offset + j * 4 + 3] = data[in_offset + j * 4 + 3];
        }
    }

    result = new TargaImage(width, height, dest);
    delete[] dest;
    return result;
}// Reverse_Rows


///////////////////////////////////////////////////////////////////////////////
//
//      Clear the image to all black.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::ClearToBlack()
{
    memset(data, 0, width * height * 4);
}// ClearToBlack


///////////////////////////////////////////////////////////////////////////////
//
//      Helper function for the painterly filter; paint a stroke at
// the given location
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::Paint_Stroke(const Stroke& s) {
    int radius_squared = (int)s.radius * (int)s.radius;
    for (int x_off = -((int)s.radius); x_off <= (int)s.radius; x_off++) {
        for (int y_off = -((int)s.radius); y_off <= (int)s.radius; y_off++) {
            int x_loc = (int)s.x + x_off;
            int y_loc = (int)s.y + y_off;
            // are we inside the circle, and inside the image?
            if ((x_loc >= 0 && x_loc < width && y_loc >= 0 && y_loc < height)) {
                int dist_squared = x_off * x_off + y_off * y_off;
                if (dist_squared <= radius_squared) {
                    data[(y_loc * width + x_loc) * 4 + 0] = s.r;
                    data[(y_loc * width + x_loc) * 4 + 1] = s.g;
                    data[(y_loc * width + x_loc) * 4 + 2] = s.b;
                    data[(y_loc * width + x_loc) * 4 + 3] = s.a;
                }
                else if (dist_squared == radius_squared + 1) {
                    data[(y_loc * width + x_loc) * 4 + 0] =
                        (data[(y_loc * width + x_loc) * 4 + 0] + s.r) / 2;
                    data[(y_loc * width + x_loc) * 4 + 1] =
                        (data[(y_loc * width + x_loc) * 4 + 1] + s.g) / 2;
                    data[(y_loc * width + x_loc) * 4 + 2] =
                        (data[(y_loc * width + x_loc) * 4 + 2] + s.b) / 2;
                    data[(y_loc * width + x_loc) * 4 + 3] =
                        (data[(y_loc * width + x_loc) * 4 + 3] + s.a) / 2;
                }
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke() {}

///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke(unsigned int iradius, unsigned int ix, unsigned int iy,
    unsigned char ir, unsigned char ig, unsigned char ib, unsigned char ia) :
    radius(iradius), x(ix), y(iy), r(ir), g(ig), b(ib), a(ia)
{
}

