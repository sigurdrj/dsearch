#include <iostream>
#include <string>
#include <vector>
#include <thread>

#include "searchers.hpp"

using std::string;
using std::vector;
using std::array;

bool exitSearch=false;
bool quiet=false;

void usage(){
	std::cerr << "Usage: dsearch [iteration count] [batch size (e.g 400)] [file to store results] OPTIONS\n";
	std::cerr << "\tOPTIONS:\n";
	std::cerr << "\t--rule=STRING             \tSet rulestring (default b3/s23)\n";
	std::cerr << "\t--percent=NUMBER          \tSet percent of alive cells in the soups\n";
	std::cerr << "\t--soupsize=NUMBER         \tSet soup size to NUMBER x NUMBER (default 16)\n";
	std::cerr << "\t--quiet                   \tNo output to stdout\n";
}

void run_search_once(DeathSearcher &searcher, const unsigned long long i){
	searcher.run_search_batch();
	const unsigned resultSize=searcher.get_result_size();
	if (!quiet){
		if (resultSize) std::cout << "\033[32m";
		std::cout << "Finished batch " << i << ". Logging " << resultSize << " objects... ";
	}

	searcher.log_result();

	if (!quiet)
		std::cout << "Logged\033[0m\n";
}

void run_search(DeathSearcher &searcher, const unsigned batchSize){
	for (unsigned long long i=0; !exitSearch; i++){
		if (!quiet){
			if (i%20==0)
				std::cout << "\033[31mPress q + enter to quit\033[0m\n"; // Just a reminder
		}

		run_search_once(searcher, i+1); // Start at 1

		if (!quiet){
			if ((i+1)%10==0)
				std::cout << "Searched " << batchSize*(i+1) << " soups\n";
		}
	}
}

bool starts_with(const string str, const string b){
	if (b.size() > str.size()) return false;
	return str.substr(0,b.size()) == b;
}

int main(int argc, char *argv[]){
	if (argc < 4){ // Must have atleast 3 arguments
		usage();
		return 1;
	}

	const string resultFilename = argv[3];
	unsigned nIters, batchSize, soupSize=16;
	unsigned char soupPercentAlive=50;
	string ruleString="b3/s23";

	try{
		nIters                = std::stoi(argv[1]);
		batchSize             = std::stoi(argv[2]);
	} catch(std::invalid_argument){
		usage();
		return 2;
	}

	if (argc>4){ // There are OPTIONS specified
		for (unsigned i=4; i<unsigned(argc); i++){
			const string option = argv[i];
			if (starts_with(option, "--rule=")){
				const unsigned flagLength = string("--rule=").size();
				const string value = option.substr(flagLength, option.size()-flagLength);
				ruleString = value;
			} else if (starts_with(option, "--percent=")){
				const unsigned flagLength = string("--percent=").size();
				const string value = option.substr(flagLength, option.size()-flagLength);
				try{
					soupPercentAlive = std::stoi(value);
				} catch(std::invalid_argument){
					usage();
					return 3;
				}
			} else if (starts_with(option, "--soupsize=")){
				const unsigned flagLength = string("--soupsize=").size();
				const string value = option.substr(flagLength, option.size()-flagLength);
				try{
					soupSize = std::stoi(value);
				} catch(std::invalid_argument){
					usage();
					return 4;
				}
				
			} else if (option == "--quiet"){
				quiet=true;
			}
		}
	}

	if (soupPercentAlive>100){usage(); return 5;} // Make sure percentAliveCells is in the range 0-100
	DeathSearcher searcher(ruleString, nIters, soupSize, batchSize, resultFilename, soupPercentAlive);

	if (!quiet)
		std::cout << "Running search on rulestring " << searcher.get_rulestring() << '\n';
	std::thread searchThread(run_search, std::ref(searcher), batchSize);

	// This is just a loop for exiting the program safely
	while (!exitSearch){
		string input;
		std::getline(std::cin, input);
		if (input == "q") exitSearch=true;
	}

	// Just to be safe
	if (searchThread.joinable())
		searchThread.join();
}
