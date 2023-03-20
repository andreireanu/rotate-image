#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>
 
void setPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
// from https://stackoverflow.com/questions/20070155/how-to-set-a-pixel-in-a-sdl-surface
{
  Uint32 * const targetPixel = (Uint32 *) ((Uint8 *) surface->pixels
                                             + y * surface->pitch
                                             + x * surface->format->BytesPerPixel);
  *targetPixel = pixel;
}
 
Uint8 getPixel(SDL_Surface *surface, int x, int y)
// adapted from https://stackoverflow.com/questions/53033971/how-to-get-the-color-of-a-specific-pixel-from-sdl-surface
{
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x ;
    return *p;
}

Uint8 bilinearInterpolation(SDL_Surface *surface, float x, float y )
{ 
    // get neighbour coordinates
    int x1 = floor(x);
    int x2 = ceil(x);
    int y1 = floor(y);
    int y2 = ceil(y);

    // get neighbour pixel values
    Uint8 x1y1 = getPixel(surface, x1, y1);
    Uint8 x1y2 = getPixel(surface, x1, y2);
    Uint8 x2y1 = getPixel(surface, x2, y1);
    Uint8 x2y2 = getPixel(surface, x2, y2);

    // get distances
    double xDist = x - x1;  
    double yDist = y - y1; 

    // linear interpolation x Axis
    double interpY1 = x1y1 * (1 - xDist) + x2y1 * xDist;  
    double interpY2 = x1y2 * (1 - xDist) + x2y2 * xDist;  

    // linear interpolation Y Axis
    double interp = interpY1 * (1 - yDist) + interpY2 * yDist;  

    return interp;
}

void displayImage(SDL_Surface* image) {

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Rotated Image", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, image->w, image->h, SDL_WINDOW_SHOWN);
    
    if (!window) {
        std::cout << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(image);
        SDL_Quit();
    }
    
    SDL_Surface* surface = SDL_GetWindowSurface(window);
    SDL_BlitSurface(image, NULL, surface, NULL);
    SDL_UpdateWindowSurface(window);

    // Wait for user to close the window
    bool quit = false;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }
    }
    
    // Clean up 
    SDL_FreeSurface(image);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
 
SDL_Surface* loadImage(const char* srcImg ) {
    SDL_Surface* image = SDL_LoadBMP(srcImg);
    if (!image) {
        std::cout << "Failed to load image: " << SDL_GetError() << std::endl;
        SDL_Quit();
    }
    std::cout << "Loaded image: " << srcImg << std::endl;
    return image;
}

SDL_Surface* rotate(SDL_Surface* inputImage, int srcWidth, int srcHeight, double theta)
{

    theta = fmod(theta, 2 * M_PI); // force theta between 0 and 2 Pi
    int quadr = theta / M_PI_2 + 1; // get quadrant
    double quadrAngle = fmod(theta, M_PI_2); // get quadrant offset angle
    std::cout << "Rotating by: " << theta * 180 / M_PI 
              << " degrees (" << theta << " radians)" << std::endl;


    // calculate offset in original image based on quadrant
    double offsetX = 0; 
    double offsetY = 0;

    switch(quadr) {
        case 1:
            offsetX =             -cos(quadrAngle) * sin(quadrAngle) * srcHeight;
            offsetY =              sin(quadrAngle) * sin(quadrAngle) * srcHeight;
            break;
        case 2:
            offsetX =              sin(quadrAngle) * sin(quadrAngle) * srcWidth ;
            offsetY =  srcHeight + cos(quadrAngle) * sin(quadrAngle) * srcWidth ;
            break;
        case 3:
            offsetX =  srcWidth  + cos(quadrAngle) * sin(quadrAngle) * srcHeight;
            offsetY =  srcHeight - sin(quadrAngle) * sin(quadrAngle) * srcHeight;
            break;
        case 4:
            offsetX =  srcWidth  - sin(quadrAngle) * sin(quadrAngle) * srcWidth ;
            offsetY =            - cos(quadrAngle) * sin(quadrAngle) * srcWidth ;
            break;
    }

    float cosTheta = cos(theta);
    float sinTheta = sin(theta);

    // new image coordinates
    int dstWidth  =  ceil(abs(sinTheta * srcHeight) + abs(cosTheta * srcWidth)) ;
    int dstHeight =  ceil(abs(cosTheta * srcHeight) + abs(sinTheta * srcWidth)) ;

    // create new image
    SDL_Surface* out = SDL_CreateRGBSurface(0, dstWidth, dstHeight, 32, 0, 0, 0, 0);

    Uint16  i, j;
    float iF , jF, xSrc, ySrc;
    Uint8 grayValue;
    Uint32 sdlGrayValue; 

    for (i=0; i < dstWidth ; i++) {
        for (j=0; j < dstHeight ; j++) {    

            iF = static_cast<float>(i) ;
            jF = static_cast<float>(j)  ;

            // get corresponding pixel in original image by rotating and offsetting
            xSrc =  iF * cosTheta + jF * sinTheta + offsetX ;
            ySrc = -iF * sinTheta + jF * cosTheta + offsetY ;

            // do nothing for pixels outside the image
            if ( (xSrc < 0 || ySrc < 0) || (xSrc > srcWidth || ySrc > srcHeight) ) {
                continue;
            }

            // interpolate and create SDL type pixel
            grayValue = bilinearInterpolation(inputImage, xSrc, ySrc );
            sdlGrayValue = SDL_MapRGB(out->format, grayValue, grayValue, grayValue);
            
            // write pixel in new image
            setPixel(out, i, j, sdlGrayValue);
        }
    }
    return out;
}

int main() {
    
    ///// Input data
    unsigned char bmpName [] = "lena_crop.bmp";
    double theta = 42;
    /////

    unsigned char* srcImg = &bmpName[0];
    SDL_Surface* inputImage = loadImage(reinterpret_cast<const char*>(srcImg));
    int srcWidth = inputImage->w;
    int srcHeight = inputImage->h;

    SDL_Surface* outputImage; 
    outputImage = rotate(inputImage, srcWidth, srcHeight, theta);
    displayImage(outputImage);
    return 1;
 
}