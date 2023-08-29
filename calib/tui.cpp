#include <iostream>
#include <string>
#include <array>

#include "calib.hpp"

using std::string;
using std::array;

const string bgChar   = "`";
const string cellChar = "#";
const string prompt   = "\033[34m>\033[0m ";
unsigned step=1;
unsigned char zoomWidth=1;
unsigned char zoomHeight=1;

const char argSeparator = ' ';

enum argerror{
	none,
	first,
	second,
	both
};

void usage(){
	std::cerr << "Usage: tui [number of threads]\n";
}

void print_n_times(const string str, const unsigned char n){
	for (unsigned char i=0; i<n; i++)
		std::cout << str;
}

void print_grid(calib::Calib &ca){
	const array <unsigned long, 2> gridSize = ca.get_size();
	for (unsigned y=0; y<gridSize[1]; y++){
		for (unsigned char repeat=0; repeat<zoomHeight; repeat++){
			for (unsigned x=0; x<gridSize[0]; x++){
				if (ca.get_state(x,y))
					print_n_times(cellChar,zoomWidth);
				else
					print_n_times(bgChar,zoomWidth);
			}
			std::cout << '\n';
		}
	}
}

string get_nth_word(const string str, const unsigned char n){
	int lastSeparatorIdx=-1;
	unsigned nthSeparator=0;
	for (unsigned i=0; i<str.size(); i++){
		if (nthSeparator==n){
			const string lastPart=str.substr(lastSeparatorIdx+1, str.size()-lastSeparatorIdx);
			return lastPart.substr(0, lastPart.find(argSeparator));
		}

		if (str[i]==argSeparator){
			++nthSeparator;
			lastSeparatorIdx=i;
		}

	}
	return str;
}

bool parse_and_run_cmd(const string cmd, calib::Calib &ca){
	const string baseCommand = get_nth_word(cmd, 0);
	const string arg1        = get_nth_word(cmd, 1);
	const string arg2        = get_nth_word(cmd, 2);

	unsigned arg1Num;
	unsigned arg2Num;
	argerror argNumError=none;
	try {arg1Num=std::stoi(arg1);} catch(std::invalid_argument){arg1Num=1; argNumError=first;}
	try {arg2Num=std::stoi(arg2);} catch(std::invalid_argument){arg2Num=1; argNumError=argerror(argNumError|2);}

	if (baseCommand == "show"){
		print_grid(ca);
	} else if (baseCommand == "step"){
		if (argNumError&1){
			for (unsigned long long i=0; i<step; i++)
				ca.update_using_threads();
		} else {
			step=arg1Num;
		}
	} else if (baseCommand == "stepnaive"){
		if (argNumError&1){
			for (unsigned long long i=0; i<step; i++)
				ca.update_naively();
		} else {
			step=arg1Num;
		}
	} else if (baseCommand == "draw"){
		if (argNumError) return 0;
		ca.set_state(arg1Num,arg2Num,1);
	} else if (baseCommand == "random"){
		ca.fill_grid_randomly();
	} else if (baseCommand == "zoom"){
		if (argNumError&1) return 0;
		if (argNumError==none){
			zoomWidth=arg1Num; zoomHeight=arg2Num;
		} else{
			zoomWidth=arg1Num; zoomHeight=arg1Num;
		}
	} else if (baseCommand =="resize"){
		if (argNumError) return 0;
		ca.set_size(arg1Num,arg2Num);
	} else if (baseCommand == "run"){
		for (unsigned long long i=0; i<step; i++){
			print_grid(ca);
			print_n_times("\n", 2);
			ca.update_using_threads();
		}
	} else if (baseCommand == "runnaive"){
		for (unsigned long long i=0; i<step; i++){
			print_grid(ca);
			print_n_times("\n", 2);
			ca.update_naively();
		}
	} else if (baseCommand == "help"){
		std::cout << "Parentheses mean an optional argument, square brackets for necessary\n";
		std::cout << "__________________________________________________\n";
		std::cout << "\033[4m"; // Underline
		std::cout << "Command   | Arguments                | Description\033[0m\n";
		std::cout << "\033[0m";
		std::cout << "show      |                          | Show the grid\n";
		std::cout << "step      | (new step count)         | No arguments iterates the grid by the specified step. An argument will set the step value\n";
		std::cout << "stepnaive | (new step count)         | Same as step, but iterates using naivelife\n";
		std::cout << "draw      | [x] [y]                  | Sets cell at x,y on\n";
		std::cout << "random    |                          | Fill grid with random assortment of cells\n";
		std::cout << "zoom      | [width] (height)         | Sets zoom level. Only a first argument will set the zoom to width*width, if both, set width and height separately\n";
		std::cout << "resize    | [width] [height]         | Resizes the grid\n";
		std::cout << "run       |                          | Iterates and draws repeatedly\n";
		std::cout << "runnaive  |                          | Iterates naively and draws repeatedly\n";
	} else if (baseCommand == "q"){
		return 1;
	} else if (!baseCommand.empty()){
		std::cout << '\"' << baseCommand << "\" is not a command. Run help for a list of commands.\n";
	}

	return 0;
}

int main(int argc, char *argv[]){
	calib::Calib ca(30,15);
	const array <unsigned long, 2> gridSize = ca.get_size();

	// Command-line argument checking
	unsigned numThreads;
	if (argc > 1){
		try{
			numThreads = std::stoi(argv[1]);
		} catch (std::invalid_argument){
			usage();
			numThreads = 1;
		}
		if (gridSize[0] % numThreads != 0)
			std::cout << "WARNING: Width is not divisible by number of threads. Output will be wrong\n";
	} else {
		usage();
		numThreads = 1;
	}

	ca.set_num_threads(numThreads);
	std::cerr << "Using " << numThreads << " thread" << (numThreads>1?"s":"") << ".\n";

	bool exit=false;
	while (!exit){
		std::cout << prompt;
		string input;
		std::getline(std::cin, input);
		exit = parse_and_run_cmd(input, ca);
	}
}
