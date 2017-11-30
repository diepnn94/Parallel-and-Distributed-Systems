#include "mpi.h"
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

//***************** Add/Change the functions(including processImage) here ********************* 

// NOTE: Some arrays are linearized in the skeleton below to ease the MPI Data Communication 
// i.e. A[x][y] become A[x*total_number_of_columns + y]
int* processImage(int* inputImage, int processId, int num_processes, int image_height, int image_width){
     

	int x, y, sum, sumx, sumy;
     int GX[3][3], GY[3][3];
     /* 3x3 Sobel masks. */
     GX[0][0] = -1; GX[0][1] = 0; GX[0][2] = 1;
     GX[1][0] = -2; GX[1][1] = 0; GX[1][2] = 2;
     GX[2][0] = -1; GX[2][1] = 0; GX[2][2] = 1;
     
     GY[0][0] =  1; GY[0][1] =  2; GY[0][2] =  1;
     GY[1][0] =  0; GY[1][1] =  0; GY[1][2] =  0;
     GY[2][0] = -1; GY[2][1] = -2; GY[2][2] = -1;
	//chunkSize is the number of rows to be computed by this process 

	int chunkSize, start_of_chunk, end_of_chunk;
	//chunkSize = (image_height * image_width)/num_processes;
	int chunk = image_height/num_processes;
	chunkSize = chunk*image_width;
    	end_of_chunk = chunkSize/image_width;
	int* partialOutputImage;
	partialOutputImage = new int[chunkSize];
    	start_of_chunk = 0;
	int begin[image_width];
	int end[image_width];
	memcpy(end, inputImage+chunkSize-image_width, image_width*sizeof(int));
	memcpy(begin, inputImage+ image_width, image_width*sizeof(int));
	int recv_top[image_width];
	int recv_bottom[image_width];

	if (processId == 0){
		MPI_Send(end, image_width, MPI_INT,processId+1, 0, MPI_COMM_WORLD); 
		MPI_Recv(recv_bottom, image_width, MPI_INT, processId+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	else if (processId == num_processes-1){
		MPI_Recv(recv_top, image_width, MPI_INT, processId-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Send(begin, image_width, MPI_INT, processId-1, 0, MPI_COMM_WORLD);
	}
	else{
		MPI_Recv(recv_top, image_width, MPI_INT, processId-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Send(end,image_width, MPI_INT, processId+1,0, MPI_COMM_WORLD);
		MPI_Recv(recv_bottom, image_width, MPI_INT, processId+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Send(begin, image_width, MPI_INT, processId-1, 0, MPI_COMM_WORLD);
					
	}
	int* newInputImage;
	if (processId == 0){
		newInputImage = new int[chunkSize+image_width];
		memcpy(newInputImage, inputImage, chunkSize*sizeof(int));
		memcpy(newInputImage+chunkSize, recv_bottom, image_width*sizeof(int));
		start_of_chunk = 0;
	}
	else if (processId == num_processes-1){
		start_of_chunk = 1;
		end_of_chunk++;
		newInputImage = new int[chunkSize+image_width];
		memcpy(newInputImage, recv_top, image_width*sizeof(int));
		memcpy(newInputImage + image_width, inputImage, chunkSize*sizeof(int));
	}

	else{
		start_of_chunk = 1;
		newInputImage = new int[chunkSize +image_width + image_width];
		memcpy(newInputImage, recv_top, image_width*sizeof(int));
		memcpy(newInputImage+image_width,inputImage, chunkSize*sizeof(int));
		memcpy(newInputImage+image_width+chunkSize, recv_bottom, image_width*sizeof(int));
		end_of_chunk = end_of_chunk +2;
	}
	
	for( x = start_of_chunk; x < end_of_chunk; x++ ){
		 for( y = 0; y < image_width; y++ ){
			 sumx = 0;
			 sumy = 0;
			//Change boundary cases as required

			if((x == 0 && processId == 0) || (x== end_of_chunk-1 && processId == num_processes-1 ))
				sum = 0;
			if (x == 0 || y == 0 || y == image_width-1)
				sum = 0;
			

			else{

				 for(int i=-1; i<=1; i++)  {
					 for(int j=-1; j<=1; j++){						
						 sumx += (newInputImage[(x+i)*image_width + (y+j)] * GX[i+1][j+1]);
					 }
				 }

				 for(int i=-1; i<=1; i++)  {
					 for(int j=-1; j<=1; j++){
						 sumy += (newInputImage[(x+i)*image_width+ (y+j)] * GY[i+1][j+1]);
					 }
				 }

				 sum = (abs(sumx) + abs(sumy));
			 }
			if (processId == 0 ){			 
				if (sum <= 0){					
					partialOutputImage[x*image_width + y] = 0;
				
				}
				else if (sum >= 255){
					partialOutputImage[x*image_width + y] = 255;
				}
				else{
					partialOutputImage[x*image_width + y] = sum;
				}
			}
			else if (processId == num_processes-1 && x < end_of_chunk-1){
				
				if (sum <= 0){					
					partialOutputImage[(x-1)*image_width + y] = 0;
				
				}
				else if (sum >= 255){
					partialOutputImage[(x-1)*image_width + y] = 255;
				}
				else{
					partialOutputImage[(x-1)*image_width + y] = sum;
				}	
			}
			else{
				if(x < end_of_chunk-1){
					if (sum <= 0){					
						partialOutputImage[(x-1)*image_width + y] = 0;
				
					}
					else if (sum >= 255){
						partialOutputImage[(x-1)*image_width + y] = 255;
					}
					else{
						partialOutputImage[(x-1)*image_width + y] = sum;
					}
				}	
				
			}
		 }
	}
	return partialOutputImage;
}

int main(int argc, char* argv[])
{
	int processId, num_processes, image_height, image_width, image_maxShades;
	int *inputImage, *outputImage;
	int* recv_buf; 
  	
	
	// Setup MPI
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &processId);
    MPI_Comm_size( MPI_COMM_WORLD, &num_processes);
	
    if(argc != 3)
    {
		if(processId == 0)
			std::cout << "ERROR: Incorrect number of arguments. Format is: <Input image filename> <Output image filename>" << std::endl;
		MPI_Finalize();
        return 0;
    }
	
	if(processId == 0)
	{
		std::ifstream file(argv[1]);
		if(!file.is_open())
		{
			std::cout << "ERROR: Could not open file " << argv[1] << std::endl;
			MPI_Finalize();
			return 0;
		}

		std::cout << "Detect edges in " << argv[1] << " using " << num_processes << " processes\n" << std::endl;

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
		inputImage = new int[image_height*image_width];

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
					inputImage[(i*image_width)+j] = pixel_val;
				}
			} else {
				continue;
			}
		}
	} // Done with reading image using process 0
	
//	***************** Add code as per your requirement below *********************
	int local_info[3];
	//int chunkSize = (image_height*image_width)/num_processes;
	int chunk = (image_height/num_processes);
	int chunkSize = chunk*image_width;
	int leftOver = image_height % num_processes;
	int info[3] = {image_height, image_width, chunkSize};
	int* to_return;
	if (processId == 0){	
	
		for (int i = 1; i < num_processes; i++)
		{
			(MPI_Send(&info, 3, MPI_INT,i, 0, MPI_COMM_WORLD)== MPI_SUCCESS);					
		}
		local_info[0] = image_height;
		local_info[1] = image_width;
		local_info[2] = chunkSize;
		outputImage = new int[image_height * image_width];  		
	}

	else{
		MPI_Recv(&local_info, 3, MPI_INT, 0,0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	
	recv_buf = new int[local_info[2]];
	MPI_Scatter(inputImage, local_info[2], MPI_INT, recv_buf, local_info[2], MPI_INT, 0, MPI_COMM_WORLD);
	to_return = processImage(recv_buf, processId, num_processes, local_info[0], local_info[1]);
	MPI_Gather(to_return, local_info[2], MPI_INT, outputImage, local_info[2], MPI_INT, 0, MPI_COMM_WORLD);
	
	
	if(processId == 0)
	{
		// Start writing output to your file 
		if (leftOver != 0){
			int GX[3][3], GY[3][3];
			GX[0][0] = -1; GX[0][1] = 0; GX[0][2] = 1;
     			GX[1][0] = -2; GX[1][1] = 0; GX[1][2] = 2;
     			GX[2][0] = -1; GX[2][1] = 0; GX[2][2] = 1;
     
    			GY[0][0] =  1; GY[0][1] =  2; GY[0][2] =  1;
     			GY[1][0] =  0; GY[1][1] =  0; GY[1][2] =  0;
     			GY[2][0] = -1; GY[2][1] = -2; GY[2][2] = -1;
			int* leftOverImage;
			int* partialLeftOverImage;
			leftOverImage = new int[(leftOver*image_width)+image_width + image_width];
			partialLeftOverImage = new int[(leftOver*image_width)+image_width];

			memcpy(leftOverImage, inputImage + ((chunk*num_processes)*image_width) - (2*image_width), ((leftOver*image_width) + (2*image_width))*sizeof(int));
			int sum = 0;
			for (int x = 1; x < leftOver + 2; x++)
			{
				int sumx;
				int sumy;
				for (int y = 0; y < image_width; y++)
				{
					sumx = 0;
					sumy = 0;
					if (y == 0 || y == image_width-1)
						sum = 0;
					else{
						for(int i=-1; i<=1; i++)  {
					 		for(int j=-1; j<=1; j++){
						 		sumx += (leftOverImage[(x+i)*image_width+ (y+j)] * GX[i+1][j+1]);
						 		sumy += (leftOverImage[(x+i)*image_width+ (y+j)] * GY[i+1][j+1]);
							}
						}
					sum = (abs(sumx) + abs(sumy));

					if(sum <= 0)
				 		partialLeftOverImage[(x-1)*image_width + y] = 0;
				 	if(sum >= 255)
				 		partialLeftOverImage[(x-1)*image_width + y] = 255;
				 	else
				 		partialLeftOverImage[(x-1)*image_width + y] = sum;


				 }
			  }
			}
				
			int starting_index = ((chunk*num_processes) * image_width) - image_width;
			memcpy(outputImage + starting_index, partialLeftOverImage, ((leftOver*image_width)+image_width)*sizeof(int));
			outputImage[(image_height*image_width)- (2*image_width)] = 0;
			for (int i = (image_height*image_width) - image_width; i < image_height*image_width; i++){
				
				outputImage[i] = 0;
			}
		}				
		
		std::ofstream ofile(argv[2]);
		if( ofile.is_open() )
		{
	 		ofile << "P2" << "\n" << image_width << " " << image_height << "\n" << image_maxShades << "\n";
			for( int i = 0; i < image_height; i++ )
			{
				for( int j = 0; j < image_width; j++ ){
					ofile << outputImage[(i*image_width)+j] << " ";
				}
				ofile << "\n";
			}
		} else {
			std::cout << "ERROR: Could not open output file " << argv[2] << std::endl;
			return 0;
		}
	std:: cout << "SUCESS" << std::endl;	
	}
	
    	MPI_Finalize();
    	return 0;
}
