#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstdlib>
#include <array>
#include <vector>

#include "calib.hpp"

using std::array;
using std::vector;

const sf::Color bgColor{0,0,0};
const sf::Color cellColor{255,255,255};
const sf::Color objectColor{255,0,0};

const unsigned width=200, height=200;

typedef vector <vector <bool>> gridType;

void usage(){
	std::cerr << "Usage: gui [number of threads]\n";
}

void draw_pxl(const unsigned x, const unsigned y, sf::RenderWindow &window, const sf::Color color){
	sf::Texture pxlTxt;
	pxlTxt.loadFromFile("pixel.png");
	sf::Sprite pxlSpr;
	pxlSpr.setTexture(pxlTxt);
	pxlSpr.setPosition(x,y);
	pxlSpr.setColor(color);

	window.draw(pxlSpr);
}

// Very slow
void draw_grid_to_window(sf::RenderWindow &window, calib::Calib &ca){
	window.clear(bgColor); // This function only draws the alive cells
	const array <unsigned long, 2> gridSize = ca.get_size();
	for (unsigned y=0; y<gridSize[1]; y++){
		for (unsigned x=0; x<gridSize[0]; x++){
			if (ca.get_state(x,y))
				draw_pxl(x,y, window, cellColor);
		}
	}
}

// For not crashing when clicking outside of the window
bool mousePosInView(const sf::Vector2i mousePos, sf::View &view){
	return !((mousePos.x > view.getSize().x || mousePos.x < 0)
		|| (mousePos.y > view.getSize().y || mousePos.y < 0));
}

int main(int argc, char *argv[]){
	calib::Calib ca(width, height);
	const array <unsigned long, 2> gridSize = ca.get_size();

	// Command-line argument checking
	if (argc > 1){
		unsigned numThreads;
		try{
			numThreads = std::stoi(argv[1]);
		} catch (std::invalid_argument){
			usage();
			numThreads = 1;
		}
		if (gridSize[0] % numThreads != 0)
			std::cout << "WARNING: Width is not divisible by number of threads. Output will be wrong\n";
		ca.set_num_threads(numThreads);
	} else {
		usage();
		ca.set_num_threads(1);
	}

	const unsigned numThreads=ca.get_num_threads();
	std::cerr << "Using " << numThreads << " thread" << (numThreads>1?"s":"") << ".\n";

	// Set up window
	const double zoom=3; // Every cell will be of size zoom*zoom in the window
	sf::RenderWindow window(sf::VideoMode(gridSize[0]*zoom,gridSize[1]*zoom), "calib", sf::Style::Titlebar | sf::Style::Close);
	sf::View view;
	view.reset(sf::FloatRect(0, 0, window.getSize().x, window.getSize().y));
	view.setViewport(sf::FloatRect(0, 0, zoom, zoom));
	window.setActive(false);
	window.setView(view);
	window.setActive(true);

	unsigned i=0;
	while (window.isOpen()){
		sf::Event e;
		while (window.pollEvent(e)){
			if (e.type == sf::Event::Closed)
				window.close();
		}

		// Draw with the mouse
		if (sf::Mouse::isButtonPressed(sf::Mouse::Left)){
			auto mousePos = sf::Mouse::getPosition(window);
			auto worldPos = window.mapPixelToCoords(mousePos);

			if (mousePosInView(mousePos, view))
				ca.set_state(worldPos.x, worldPos.y, 1);
		} else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)){
			auto mousePos = sf::Mouse::getPosition(window);
			auto worldPos = window.mapPixelToCoords(mousePos);

			if (mousePosInView(mousePos, view))
				ca.set_state(worldPos.x, worldPos.y, 0);
		}

		// Click 'R' to fill the grid randomly
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::R))
			ca.fill_grid_randomly();


		// Press Space to iterate the grid (hold shift if you want to iterate using naivelife)
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)){
			++i;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
				ca.update_naively(); // This approach of updating won't work with threading
			else
				ca.update_using_threads(); // Alternatively, use ca.update();

			std::cout << i << '\n';
		}

		// Uncomment this line to only draw every 50 iterations. Change 50 to whatever you want
//		if (i % 50 == 0)
		{
			draw_grid_to_window(window, ca);

			// EXPERIMENTAL (Click 'O' to highlight objects)
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::O)){
				auto mousePos = sf::Mouse::getPosition(window);
				auto worldPos = window.mapPixelToCoords(mousePos);

				auto objectCells = ca.get_object_cells(worldPos.x, worldPos.y);
				for (auto cell : objectCells)
					draw_pxl(cell[0], cell[1], window, objectColor);
			}
			window.display();
		}
	}
}
