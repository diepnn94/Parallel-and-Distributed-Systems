#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
 
/* Global variables, Look at their usage in main() */
int image_height;
int image_width;
int image_maxShades;
int inputImage[1000][1000];
int outputImage[1000][1000];
int num_threads; 
int chunkSize;
int maxChunk;

/* ****************Change and add functions below ***************** */
int start = 0;
int init_threads;
std::mutex chunk;

/* ****************Change and add functions below ***************** */
int maskX[3][3];
int maskY[3][3];

int get_next_index( int code, int starting )
{
    std::lock_guard<std::mutex> guard(chunk);
    std::thread::id this_id = std::this_thread::get_id();
    //This part is to print out the initial processes that were given a starting point in the dispatch_thread
    if (code == 0 && starting < image_height)
    {
        std:: cout << "**Process " << this_id << " is running from index: " << starting << " to ";
        int end;
        if (image_height - starting < chunkSize)
        {
            end = starting + (image_height - starting);
            std:: cout << end-1 << std::endl;
        }
        else{
            end = starting + chunkSize;
            std:: cout << end-1 << std::endl;
        }
        return -1;
    }
    //This part is to print out when a process finish with it indexes, asking for next available index to start and update the next available index
    if (start < image_height){
        
        int temp = start;
        std:: cout << "Process " << this_id << " is running from index: " << temp << " to ";
        if (image_height - start < chunkSize)
        {
            start = start + (image_height - start);
            std:: cout << start-1 << std::endl;
            maxChunk = maxChunk -1;
            return temp;
        }
        else{
            start = start+chunkSize;
            std:: cout << start-1 << std::endl;
            maxChunk = maxChunk-1;
            return temp;
        }
    }
    return image_height;
}

void function(int begin)
{
    std::thread::id this_id = std::this_thread::get_id();
    int starting_chunk ;
    int ending_chunk;
    int flag = 0;
    while(1){
        
        if (maxChunk == 0)
            break;
        //after a process finish its process asking for the next available index
        if (flag == 1){
            starting_chunk = get_next_index(flag, 0);
        }
        //process that should execute the index given in the argument
        if (flag == 0){
            starting_chunk = chunkSize*begin;
            get_next_index(flag, starting_chunk);
            flag = 1;
        }
        
        if (starting_chunk >= image_height)
            break;
        
        
        if (image_height - starting_chunk  < chunkSize){
            ending_chunk = starting_chunk + (image_height - starting_chunk) ;
        }
        
        else{
            ending_chunk = starting_chunk + chunkSize;
        }
        int sumx, sumy, sum;
        
        for (int x = starting_chunk; x < ending_chunk; ++x)
        {
            for (int y = 0; y < image_width; y++)
            {
                sumx = 0;
                sumy = 0;
                /* For handling image boundaries */
                if( x == 0 || x == (image_height-1) || y == 0 || y == (image_width-1))
                    sum = 0;
                else{
                    /* Gradient calculation in X Dimension */
                    for(int i=-1; i<=1;i++) {
                        for( int j = -1; j <= 1; j++ ){
                            sumx += (inputImage[x+i][y+j] * maskX[i+1][j+1]);
                        }
                    }
                    /* Gradient calculation in Y Dimension */
                    for(int i=-1; i<=1; i++) {
                        for(int j=-1; j<=1; j++){
                            sumy += (inputImage[x+i][y+j] * maskY[i+1][j+1]);
                        }
                    }
                    /* Gradient magnitude */
                    sum = (abs(sumx) + abs(sumy)); }
                
                if (sum <= 0)
                    outputImage[x][y] = 0;
                else if(sum >= 255)
                    outputImage[x][y] = 255;
                else
                    outputImage[x][y] = sum;
            }
        }
    }
}

void dispatch_threads()
{
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

    start = num_threads *chunkSize;
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++)
    {
        threads.push_back(std::thread(&function, i));
    }
    
    for (auto& th: threads)
    {
        th.join();
    }
}

/* ****************Need not to change the function below ***************** */

int main(int argc, char* argv[])
{
    if(argc != 5)
    {
        std::cout << "ERROR: Incorrect number of arguments. Format is: <Input image filename> <Output image filename> <Threads#> <Chunk size>" << std::endl;
        return 0;
    }
 
    std::ifstream file(argv[1]);
    if(!file.is_open())
    {
        std::cout << "ERROR: Could not open file " << argv[1] << std::endl;
        return 0;
    }
    num_threads = std::atoi(argv[3]);
    chunkSize  = std::atoi(argv[4]);

    std::cout << "Detect edges in " << argv[1] << " using " << num_threads << " threads\n" << std::endl;

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

    /* maxChunk is total number of chunks to process */
    maxChunk = ceil((float)image_height/chunkSize);

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

    /************ Function that creates threads and manage dynamic allocation of chunks *********/
    dispatch_threads();

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
