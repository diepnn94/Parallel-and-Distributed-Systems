#include "mpi.h"
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
const static int ARRAY_SIZE = 130000;
using Lines = char[ARRAY_SIZE][16];


// To remove punctuations
struct letter_only: std::ctype<char> 
{
    letter_only(): std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const* get_table()
    {
        static std::vector<std::ctype_base::mask> 
            rc(std::ctype<char>::table_size,std::ctype_base::space);

        std::fill(&rc['A'], &rc['z'+1], std::ctype_base::alpha);
        return &rc[0];
    }
};

void DoOutput(std::string word, int result)
{
    std::cout << "Word Frequency: " << word << " -> " << result << std::endl;
}

//***************** Add your functions here *********************


int calculate(char partial_lines[][16], int size, const char* search_word){
	int partial_count = 0;
	
	for (int i = 0; i < size; i++){
		if (strcmp(partial_lines[i], search_word)==0)
			partial_count++;
	}
	return partial_count;
}

int main(int argc, char* argv[])
{
    int processId;
    int num_processes;
    int *to_return = NULL;
    double start_time, end_time;
	int count = 0;
    // Setup MPI
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &processId);
    MPI_Comm_size( MPI_COMM_WORLD, &num_processes);
 
    // Three arguments: <input file> <search word> <part B1 or part B2 to execute>
    if(argc != 4)
    {
        if(processId == 0)
        {
            std::cout << "ERROR: Incorrect number of arguments. Format is: <filename> <word> <b1/b2>" << std::endl;
        }
        MPI_Finalize();
        return 0;
    }
	std::string word = argv[2];
 
    Lines lines;
	// Read the input file and put words into char array(lines)
    if (processId == 0) {
        std::ifstream file;
		file.imbue(std::locale(std::locale(), new letter_only()));
		file.open(argv[1]);
		std::string workString;
		int i = 0;
		while(file >> workString){
			memset(lines[i], '\0', 16);
			memcpy(lines[i++], workString.c_str(), workString.length());
			count ++;
		}
    }
	
//	***************** Add code as per your requirement below ***************** 
	
	start_time=MPI_Wtime();
	int chunkSize = count/num_processes;
	int leftOver = count % num_processes;
	int local_info=0;
	int partial_sum = 0;
	int sum = 0;
	int passing_sum = 0;
	if (processId == 0){
		for (int i = 1; i < num_processes; i++){
			MPI_Send(&chunkSize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
		}
		local_info = chunkSize;	
	}
	else
	{
		MPI_Recv(&local_info, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
	}
	
	char recv_buf[local_info][16];
	
	MPI_Scatter(&lines, local_info*16, MPI_CHAR, &recv_buf, local_info*16, MPI_CHAR, 0, MPI_COMM_WORLD);
	partial_sum = calculate(recv_buf, local_info, word.c_str());
	
	if( strcmp(argv[3], "b1") == 0 )
	{
		// Reduction for Part B1
		MPI_Reduce(&partial_sum, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);	
	}
	else {
		// Point-To-Point communication for Part B2
		if (processId == 0){
			MPI_Send(&partial_sum, 1, MPI_INT, processId + 1, 0, MPI_COMM_WORLD);
			MPI_Recv(&passing_sum, 1, MPI_INT, num_processes-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			sum = passing_sum;
		
		}
		else{
			MPI_Recv(&passing_sum, 1, MPI_INT, processId-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			partial_sum+= passing_sum;
			if(processId == num_processes-1)
				MPI_Send(&partial_sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
			else
				MPI_Send(&partial_sum, 1, MPI_INT, processId+1, 0, MPI_COMM_WORLD);
		}
	}
	
    if(processId == 0)
    {
		
        // Output the search word's frequency here
		if (leftOver != 0){
			int starting_index = chunkSize*num_processes;
			char leftOverChunk[leftOver][16];
			int leftOverSum = 0;
			memcpy(leftOverChunk, lines + starting_index, leftOver*sizeof(char*));
			leftOverSum = calculate(leftOverChunk, leftOver+1, word.c_str());
			sum+= leftOverSum;
		}
		DoOutput(word, sum);
		end_time=MPI_Wtime();
        std::cout << "Time: " << ((double)end_time-start_time) << std::endl;
    }
 
    MPI_Finalize();
 
    return 0;
}