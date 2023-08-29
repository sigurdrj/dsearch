#ifndef SEARCHERS_HPP
#define SEARCHERS_HPP

#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <fstream>
#include <cstdlib> // rand()
#include <chrono>
#include <cmath> // std::ceil()

#include "calib/calib.hpp"

using std::string;
using std::vector;
using std::array;

typedef array <unsigned, 2> Position;
typedef vector <Position> Object;
typedef vector <bool> ruleType;

template <class T>
string to_str(const T val){
	std::ostringstream convert;
	convert << val;
	return convert.str();
}

class DeathSearcher{
	unsigned nIters;
	unsigned batchSize;
	unsigned char soupPercentAlive;
	string resultFilename;
	vector <Object> result;

	// Their sizes are size*size, meaning it's just a square
	// Easier to deal with them this way because of the dynamic resizing of the grid
	unsigned soupSize=16; // This has to be divisible by 2 (unless you want inaccurate results)
	unsigned initialGridSize=soupSize;

	unsigned sizeDiff=0;

	calib::Calib caTemplate;

	Object get_random_soup(){
		Object out;
		for (unsigned y=0; y<soupSize; y++){
			for (unsigned x=0; x<soupSize; x++){
				if (unsigned((double(rand())/RAND_MAX * 100)) < soupPercentAlive)
					out.push_back({x,y});
			}
		}
		return out;
	}

	public:

	DeathSearcher(const string ruleString, const unsigned newNIters, const unsigned newSoupSize, const unsigned newBatchSize, const string newResultFilename, const unsigned newSoupPercentAlive){
		caTemplate.set_rule(calib::Calib::rulestring_to_rule(ruleString));
		nIters=newNIters; soupSize=newSoupSize; batchSize=newBatchSize; resultFilename=newResultFilename; soupPercentAlive=newSoupPercentAlive;

		if (nIters<=6) // nIters is very small so resizing the grid wouldn't really be necessary
			initialGridSize = soupSize + (nIters << 1);
		else if(nIters >= 66) // nIters is very large, so I want to resize the grid many times to speed it up
			sizeDiff = std::ceil(nIters/20.0);
		else
			sizeDiff = std::ceil(nIters/12.0);

		initialGridSize += sizeDiff << 1; // Add the size before setting the cas size so I don't have to resize the grid
		caTemplate.set_size(initialGridSize, initialGridSize);
		caTemplate.fill_grid(0);

		const unsigned seed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		srand(seed);
	}

	void set_n_iters(const unsigned newNIters){nIters=newNIters;}
	unsigned get_result_size(){return result.size();}

	void set_batch_size(const unsigned newBatchSize){
		batchSize=newBatchSize;
		result.resize(batchSize);
		result.shrink_to_fit();
	}
	void set_soup_percent_alive(const unsigned char newSoupPercentAlive){soupPercentAlive=newSoupPercentAlive;}
	void set_result_filename(){}
	void set_soup_size(const unsigned newSoupSize){soupSize=newSoupSize;}
	void set_rule(const std::pair <ruleType,ruleType> newRule){caTemplate.set_rule(newRule);}
	string get_rulestring(){return calib::Calib::rule_to_rulestring(caTemplate.get_rule());}

	void run_one_search(calib::Calib ca){
		const Object soup = get_random_soup();
		const unsigned soupOffset = initialGridSize/2 - soupSize/2; // To place the soup in the middle of the grid
		ca.draw_object_to_grid(soup, soupOffset, soupOffset);

		for (unsigned i=0; i<nIters-1; i++){
			// TODO replace all this with a if (i%sizeDiff == 0) ca.add_size_all_sides(sizeDiff); or something
			const unsigned long gridSize = ca.get_width(); // width = height
			if (i+1 > gridSize - soupSize)
				ca.add_size_all_sides(sizeDiff);

			ca.update(false);
		}

		if (ca.update(true) == 0) // Found result!
			result.push_back(soup);
	}

	void run_search_batch(){
		vector <std::thread> searchThreads(batchSize);

		for (std::thread &thread : searchThreads)
			thread = std::thread(&DeathSearcher::run_one_search, this, caTemplate);

		for (std::thread &thread : searchThreads)
			thread.join();
	}

	void log_result(){
		if (result.size() == 0) return;

		std::ofstream resultFile(resultFilename, std::ofstream::app);

		for (Object obj : result)
			resultFile << calib::Calib::object_to_rle(obj, caTemplate.get_rule(), soupSize, soupSize) << "\n#Pattern found using dsearch (nIters:" << nIters << ")\n\n"; // Empty newline separates objects in the file

		resultFile.close();
		result.clear();
	}
};

#endif // SEARCHERS_HPP
