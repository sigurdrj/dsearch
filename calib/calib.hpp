#ifndef CALIB_HPP
#define CALIB_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <thread>
#include <functional>

using std::array;
using std::vector;
using std::string;

typedef vector <vector <bool>> gridType;
typedef vector <array <int, 2>> neighborhoodType;
typedef vector <bool> ruleType;

typedef array <unsigned, 2> Position;
typedef vector <Position> Object;

// For wrapping on the grid
unsigned modulo(const int a, const unsigned b){
	if (a>0) return a%b;
	return (b+a)%b;
}

namespace calib{
	template <class T>
	bool value_in_vector(const T elem, const vector <T> vec){
		for (T element : vec){
			if (element == elem)
				return true;
		}
		return false;
	}

	template <class T>
	string to_str(const T val){
		std::ostringstream convert;
		convert << val;
		return convert.str();
	}

	class Calib{
		// cgol rule
		//                   0 1 2 3 4 5 6 7 8
		ruleType   birthRule{0,0,0,1,0,0,0,0,0};
		ruleType surviveRule{0,0,1,1,0,0,0,0,0};
		unsigned numThreads=1, width=0, height=0;
		gridType grid;
		gridType tmpGrid;

		// These are relative positions that make up the neighborhood.
		neighborhoodType neighborhood{{-1,-1}, {0,-1}, {1,-1}, {-1,0}, {1,0}, {-1,1}, {0,1}, {1,1}}; // Moore

		void resize_grids(){
			if ((width==0) || (height==0)){
				std::cerr << "Grid width or height can't be 0 (width=" << width << ",height=" << height << ")\n";
				return;
			}

			grid.resize(width);
			grid.shrink_to_fit();
			for (unsigned i=0; i<grid.size(); i++){
				grid[i].resize(height);
				grid[i].shrink_to_fit();
			}

			// Maybe faster
			tmpGrid=grid;
			tmpGrid.shrink_to_fit();
			for (unsigned i=0; i<tmpGrid.size(); i++)
				tmpGrid[i].shrink_to_fit();
		}

		public:

		Calib(const unsigned newWidth, const unsigned newHeight){
			set_size(newWidth,newHeight);
			fill_grid(0);
		}

		Calib(){} // Empty constructor

		// Get, set
		std::pair <ruleType,ruleType> get_rule(){return std::make_pair(birthRule,surviveRule);}
		void set_rule(const std::pair <ruleType,ruleType> newRule){birthRule=newRule.first; surviveRule=newRule.second;}

		void set_size(const unsigned newWidth, const unsigned newHeight){width=newWidth; height=newHeight; resize_grids();}
		array <unsigned long, 2> get_size(){return {grid.size(), grid[0].size()};} // Unsigned long so the compiler doesn't complain about using just an unsigned
		unsigned long get_width(){return grid.size();}
		unsigned long get_height(){return grid[0].size();}
		bool get_state(const unsigned x, const unsigned y) {return grid[x][y];}
		void set_state(const unsigned x, const unsigned y, const bool state) {grid[x][y] = state;}
		gridType get_grid() {return grid;}
		void set_grid(const gridType newGrid) {grid = newGrid;}
		unsigned get_num_threads(){return numThreads;}
		void set_num_threads(const unsigned newNumThreads){numThreads = newNumThreads;}

		void add_size_all_sides(const unsigned size=1){
			for (unsigned i=0; i<grid.size(); i++){
				grid[i].insert(grid[i].begin(),size,0);
				grid[i].insert(grid[i].end(),  size,0);
			}

			const vector <bool> emptyRow(grid.size() + (size << 1), 0);
			grid.insert(grid.begin(),size,emptyRow);
			grid.insert(grid.end(),  size,emptyRow);

			width  += size<<1;
			height += size<<1;
		}

		unsigned get_num_neighbors_of_state(const unsigned x, const unsigned y, const bool state){
			unsigned sum=0;
			for (array <int, 2> relativeNeighborPos : neighborhood){
				unsigned newX=modulo(int(x)+relativeNeighborPos[0], grid.size());
				unsigned newY=modulo(int(y)+relativeNeighborPos[1], grid[0].size());

				sum += grid[newX][newY] == state;
			}
			return sum;
		}

		vector <array <unsigned, 2>> get_neighbor_positions(const unsigned x, const unsigned y){
			// TODO Use array with fixed size to improve speed
			vector <array <unsigned, 2>> out;
			for (array <int, 2> relativeNeighborPos : neighborhood){
				unsigned newX=modulo(int(x)+relativeNeighborPos[0], grid.size());
				unsigned newY=modulo(int(y)+relativeNeighborPos[1], grid[0].size());
				out.push_back({newX, newY});
			}
			return out;
		}

		unsigned update(const bool doSum=false){
			unsigned sum=0;
			gridType newGrid=grid;

			for (unsigned y=0; y < grid[0].size(); y++){
				for (unsigned x=0; x < grid.size(); x++){
					unsigned numNeighbors = get_num_neighbors_of_state(x,y,1);
					if (!grid[x][y])
						newGrid[x][y] = birthRule[numNeighbors];
					else
						newGrid[x][y] = surviveRule[numNeighbors];

					if (doSum) sum += newGrid[x][y];
				}
			}
			grid=newGrid;
			return sum;
		}

		unsigned update_using_threads(const bool doSum=false){
			vector <unsigned> sectionSums(numThreads, 0);
			vector <std::thread> sectionThreads(numThreads);

			for (unsigned sectionIndex=0; sectionIndex < numThreads; sectionIndex++)
				sectionThreads[sectionIndex] = std::thread(&Calib::update_section, this, sectionIndex, std::ref(sectionSums[sectionIndex]), doSum);

			for (std::thread &thread : sectionThreads)
				thread.join();

			grid = tmpGrid;
			unsigned fullSum=0;
			for (unsigned sum : sectionSums) fullSum += sum;
			return fullSum;
		}

		unsigned update_section(const unsigned sectionIndex, unsigned &sum, const bool doSum){
			// TODO Make this better so that the grid width doesn't have to be divisible by numThreads
			const unsigned sectionXStart = sectionIndex * (grid.size()/numThreads);
			const unsigned sectionXEnd   = sectionXStart + (grid.size()/numThreads);

			for (unsigned y=0; y < grid[0].size(); y++){
				for (unsigned x=sectionXStart; x < sectionXEnd; x++){
					unsigned numNeighbors = get_num_neighbors_of_state(x,y,1);
					if (!grid[x][y])
						tmpGrid[x][y] = birthRule[numNeighbors];
					else
						tmpGrid[x][y] = surviveRule[numNeighbors];

					if (doSum) sum += tmpGrid[x][y];
				}
			}
			return sum;
		}

		unsigned update_naively(const bool doSum=false){
			unsigned sum=0;

			for (unsigned y=0; y < grid[0].size(); y++){
				for (unsigned x=0; x < grid.size(); x++){
					unsigned numNeighbors = get_num_neighbors_of_state(x,y,1);
					if (!grid[x][y])
						grid[x][y] = birthRule[numNeighbors];
					else
						grid[x][y] = surviveRule[numNeighbors];

					if (doSum) sum += grid[x][y];
				}
			}

			return sum;
		}

		void fill_grid(const bool state){
			for (unsigned y=0; y < grid[0].size(); y++){
				for (unsigned x=0; x < grid.size(); x++)
					grid[x][y] = state;
			}
		}

		void fill_grid_randomly(){
			for (unsigned y=0; y<grid[0].size(); y++){
				for (unsigned x=0; x<grid.size(); x++){
					if (rand()&1)
						grid[x][y]=1;
				}
			}
		}

		void draw_object_to_grid(const Object obj, const unsigned offsetX, const unsigned offsetY){
			for (Position pos : obj)
				grid[pos[0] + offsetX][pos[1] + offsetY] = 1;
		}

		// EXPERIMENTAL (object finding function)
		// It calls that value_in_vector function every neighborhood it encounters, making it insanely slow
		Object get_object_cells(const unsigned x, const unsigned y){
			Object out = {{x,y}};

			for (unsigned cellIndex=0; cellIndex < out.size(); cellIndex++){
				auto cellPos = out[cellIndex];
				for (array <unsigned, 2> neighborPos : get_neighbor_positions(cellPos[0], cellPos[1])){
					if (value_in_vector(neighborPos, out)) // Cell is already in the object
						continue;

					if (grid[neighborPos[0]][neighborPos[1]])
						out.push_back(neighborPos); // Add cell to object
				}
			}
			return out;
		}

		static string str_n_times(const string str, const unsigned n){
			string out="";
			for (unsigned i=0; i<n; i++) out+=str;
			return out;
		}

		static string object_to_rle_object(const Object obj, const unsigned x, const unsigned y){
			string out="";
			vector <string> rows(y, str_n_times("b",x)); // b signifies an empty cell

			for (Position pos : obj)
				rows[pos[1]][pos[0]] = 'o'; // o Signifies an alive cell

			for (string row : rows)
				out += row + '$'; // $ Signifies a new "line" in the pattern

			return out+'!'; // ! Signifies the end of the pattern
		}

		static string object_to_rle(const Object obj, const std::pair <ruleType,ruleType> rule, const unsigned x, const unsigned y){
			string out = "x=" + to_str(x) + ",y=" + to_str(y) + ",rule=" + rule_to_rulestring(rule) + "\n";
			out += object_to_rle_object(obj, x, y);
			return out;
		}

		static string rule_to_rulestring(const std::pair <ruleType,ruleType> rule){
			if ((rule.first.size()>10) || (rule.second.size()>10)) // Rule has a digit above 9, impossible to fit within one character
				return "INVALID";

			string out="B";
			for (unsigned i=0; i<rule.first.size(); i++)
				if (rule.first[i]) out += (i+'0');

			out += "/S";
			for (unsigned i=0; i<rule.second.size(); i++)
				if (rule.second[i]) out += (i+'0');

			return out;
		}

		static bool is_digit(const char chr){
			return (chr >= '0') && (chr <= '9');
		}

		static std::pair <ruleType,ruleType> rulestring_to_rule(const string ruleString){
			ruleType outBirthRule(9);
			ruleType outSurviveRule(9);

			enum ParserState{inactive, birth, survive};
			ParserState state=inactive;

			for (unsigned i=0; i<ruleString.size(); i++){
				auto chr = ruleString[i];

				if ((chr=='B') || (chr=='b')){ // "B"
					state = birth;
					continue;
				} else if ((chr=='S') || (chr=='s')){ // "/S"
					if (i==0) continue; // Make sure I don't access ruleString[0-1] and crash the program
					if (ruleString[i-1] != '/') continue;
					state = survive;
					continue;
				}

				if (state == birth){
					if (is_digit(chr)) outBirthRule[chr-'0'] = true;
				} else if (state == survive){
					if (is_digit(chr)) outSurviveRule[chr-'0'] = true;
				}
			}

			return std::make_pair(outBirthRule,outSurviveRule);
		}
	};
}

#endif // CALIB_HPP
