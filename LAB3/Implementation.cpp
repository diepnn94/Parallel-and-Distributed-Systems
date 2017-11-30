#include <omp.h>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
 
/* Global variables, Look at their usage in main() */
int image_height;
int image_width;
int image_maxShades;
int inputImage[1000][1000];
int outputImage[1000][1000];
int chunkSize;
int start_row = 0;
int maskX[3][3];
int maskY[3][3];


/* ****************Change and add functions below ***************** */



	
void compute_sobel_static(int index)
{
	int sum, sumx, sumy;
	
            for (int y = 0; y < image_width; y++)
            {
                sumx = 0;
                sumy = 0;
                /* For handling image boundaries */
                if( index == 0 || index == (image_height-1) || y == 0 || y == (image_width-1))
                    sum = 0;
                else{
                    /* Gradient calculation in X Dimension */
                    for(int i=-1; i<=1;i++) {
                        for( int j = -1; j <= 1; j++ ){
                            sumx += (inputImage[index+i][y+j] * maskX[i+1][j+1]);
                        }
                    }
                    /* Gradient calculation in Y Dimension */
                    for(int i=-1; i<=1; i++) {
                        for(int j=-1; j<=1; j++){
                            sumy += (inputImage[index+i][y+j] * maskY[i+1][j+1]);
                        }
                    }
                    /* Gradient magnitude */
                    sum = (abs(sumx) + abs(sumy));
				}
                #pragma omp critical
				{
                if (sum <= 0)
                    outputImage[index][y] = 0;
                else if(sum >= 255)
                    outputImage[index][y] = 255;
                else
                    outputImage[index][y] = sum;
				}
				
			}
}





 
void compute_sobel_dynamic( int index)
{
	
        int sum, sumx, sumy;
		 for (int y = 0; y < image_width; y++)
         {
                sumx = 0;
                sumy = 0;
                /* For handling image boundaries */
                if( index == 0 || index == (image_height-1) || y == 0 || y == (image_width-1))
                    sum = 0;
                else{
                    /* Gradient calculation in X Dimension */
                    for(int i=-1; i<=1;i++) {
                        for( int j = -1; j <= 1; j++ ){
                            sumx += (inputImage[index+i][y+j] * maskX[i+1][j+1]);
                        }
                    }
                    /* Gradient calculation in Y Dimension */
                    for(int i=-1; i<=1; i++) {
                        for(int j=-1; j<=1; j++){
                            sumy += (inputImage[index+i][y+j] * maskY[i+1][j+1]);
                        }
                    }
                    /* Gradient magnitude */
					sum = (abs(sumx) + abs(sumy)); 
				}
		
		#pragma omp critical
		{
		if(sum <= 0)
			outputImage[index][y] = 0;
		if(sum >= 255)
			outputImage[index][y] = 255;
		else
			outputImage[index][y] = sum;
		}
	}
}

/* **************** Change the function below if you need to ***************** */

int main(int argc, char* argv[])
{
    if(argc != 5)
    {
        std::cout << "ERROR: Incorrect number of arguments. Format is: <Input image filename> <Output image filename> <Chunk size> <a1/a2>" << std::endl;
        return 0;
    }
 
    std::ifstream file(argv[1]);
    if(!file.is_open())
    {
        std::cout << "ERROR: Could not open file " << argv[1] << std::endl;
        return 0;
    }
    chunkSize  = std::atoi(argv[3]);

    std::cout << "Detect edges in " << argv[1] << " using OpenMP threads\n" << std::endl;

    /* ******Reading image into 2-D array below******** */

    std::string workString;
    /* Remove comments '#' and check image format */ 
    while(std::getline(file,workString))
    {
        if( workString.at(0) != '#' ){
            if( workString.at(1) != '2' ){
                std::cout << "Input image is not a valid PGM image" << std::endl;
                return 0;
            } else {
                break;
            }       
        } else {
            continue;
        }
    }
    /* Check image size */ 
    while(std::getline(file,workString))
    {
        if( workString.at(0) != '#' ){
            std::stringstream stream(workString);
            int n;
            stream >> n;
            image_width = n;
            stream >> n;
            image_height = n;
            break;
        } else {
            continue;
        }
    }

    /* Check image max shades */ 
    while(std::getline(file,workString))
    {
        if( workString.at(0) != '#' ){
            std::stringstream stream(workString);
            stream >> image_maxShades;
            break;
        } else {
            continue;
        }
    }
    /* Fill input image matrix */ 
    int pixel_val;
    for( int i = 0; i < image_height; i++ )
    {
        if( std::getline(file,workString) && workString.at(0) != '#' ){
            std::stringstream stream(workString);
            for( int j = 0; j < image_width; j++ ){
                if( !stream )
                    break;
                stream >> pixel_val;
                inputImage[i][j] = pixel_val;
            }
        } else {
            continue;
        }
    }

    /************ Call functions to process image *********/
    std::string opt = argv[4];
    //int maskX[3][3]; 
//    int maskY[3][3];
	/* 3x3 Sobel mask for X Dimension. */
	
	maskX[0][0] = -1; 
	maskX[0][1] = 0; 
	maskX[0][2] = 1;
	maskX[1][0] = -2; 
	maskX[1][1] = 0; 
	maskX[1][2] = 2;
	maskX[2][0] = -1; 
	maskX[2][1] = 0; 
	maskX[2][2] = 1;
	/* 3x3 Sobel mask for Y Dimension. */
	maskY[0][0] = 1; 
	maskY[0][1] = 2; 
	maskY[0][2] = 1;
	maskY[1][0] = 0; 
	maskY[1][1] = 0; 
	maskY[1][2] = 0;
	maskY[2][0] = -1; 
	maskY[2][1] = -2; 
	maskY[2][2] = -1;
	
    if( !opt.compare("a1") )
    {    
        double dtime_static = omp_get_wtime();
		int total_processes;
		//parallel
		#pragma omp parallel
		{
		chunkSize = image_height/omp_get_num_threads();
		}
		#pragma omp parallel for schedule(static, chunkSize) shared(maskX, maskY, inputImage, outputImage, chunkSize)
		for(start_row = 0; start_row < image_height; start_row++){
			if (start_row % chunkSize==0){
				#pragma omp critical
				std:: cout<< omp_get_thread_num() << " start at " << start_row << std::endl;
			}
			compute_sobel_static(start_row);
			
		}
		dtime_static = omp_get_wtime() - dtime_static;
		std:: cout << "Total Time for Static: " << dtime_static << std::endl;
        
    } 
	else {
        double dtime_dyn = omp_get_wtime();
		#pragma omp parallel for schedule(dynamic, chunkSize) shared(maskX, maskY, inputImage, outputImage)
		for (start_row = 0; start_row < image_height; start_row++){
			if (start_row % chunkSize == 0)
			{
				#pragma omp critical
				{
					std:: cout << "processID " << omp_get_thread_num() << " start at " << start_row << std::endl;
				}
			}
			compute_sobel_dynamic(start_row);
		}

    	
        dtime_dyn = omp_get_wtime() - dtime_dyn;
		std:: cout << "Total Time for Dynamic: " << dtime_dyn << std::endl;
    }

    /* ********Start writing output to your file************ */
    std::ofstream ofile(argv[2]);
    if( ofile.is_open() )
    {
        ofile << "P2" << "\n" << image_width << " " << image_height << "\n" << image_maxShades << "\n";
        for( int i = 0; i < image_height; i++ )
        {
            for( int j = 0; j < image_width; j++ ){
                ofile << outputImage[i][j] << " ";
            }
            ofile << "\n";
        }
    } else {
        std::cout << "ERROR: Could not open output file " << argv[2] << std::endl;
        return 0;
    }
    return 0;
}
