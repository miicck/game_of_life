#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

/// The number of cpu cores to run on
const auto CPU_COUNT = std::thread::hardware_concurrency();

/// This class represents a game of life board.
class board
{
private:
    
    bool** grid; ///<The current state of the board
    bool** new_grid; ///<The next state of the board
    int** point_indicies; ///<The indicies of the squares in the vertex array
    int x_size; ///<The x size of the board
    int y_size; ///<The y size of the board
    int square_size; ///<The size of a square in the board
    sf::VertexArray* vertex_array; ///<Vertex array used to draw the squares

    /// Returns the number of alive neighbours of the square at \p x \p y.
    /// Assumes 0 < x < x_size-1 and 0 < y < y_size-1.
    int neighbours(int x, int y)
    {
        int ret = -grid[x][y];
        for (int dx=-1; dx<2; ++dx)
            for (int dy=-1; dy<2; ++dy)
                if (grid[x+dx][y+dy])
                    ret += 1;
        return ret;
    }

    /// Update board::new_grid for x in [xmin, xmax), by applying
    /// the game of life rules to board::grid.
    void update_new_grid(int xmin, int xmax)
    {
        if (xmin < 1) xmin = 1;
        if (xmax > x_size - 1) xmax = x_size - 1;
        for (int x=xmin; x<xmax; ++x)
            for (int y=1; y<y_size-1; ++y)
            {
                int ns = neighbours(x, y);
                if (ns < 2) new_grid[x][y] = false;
                else if (ns == 2) new_grid[x][y] = grid[x][y];
                else if (ns == 3) new_grid[x][y] = true;
                else new_grid[x][y] = false;

                int i = point_indicies[x][y];
                for (int j=i; j<i+3; ++j)
                {
                    if (!new_grid[x][y])
                        vertex_array->operator[](j).color = sf::Color::Black;
                    else if (ns == 3)
                        vertex_array->operator[](j).color = sf::Color::Blue;
                    else 
                    vertex_array->operator[](j).color = sf::Color::Red;
                }
            }
    }

    /// Copy board::new_grid to board::grid for x in [xmin, xmax).
    void copy_grid(int xmin, int xmax)
    {
        // Update grid with new grid
        for (int x=xmin; x<xmax; ++x)
            for (int y=0; y<y_size; ++y)
                grid[x][y] = new_grid[x][y];
    }

public:
    
    /// Creates a game of life board of size \p x_size by \p y_size
    board(int x_size, int y_size, int square_size=1)
    {
        this->x_size = x_size;
        this->y_size = y_size;
        this->square_size = square_size;

        // Initialize arrays
        grid = new bool*[x_size];
        new_grid = new bool*[x_size];
        point_indicies = new int*[x_size];
        for (int x=0; x<x_size; ++x)
        {
            grid[x] = new bool[y_size];
            new_grid[x] = new bool[y_size];
            point_indicies[x] = new int[y_size];
        }

        // Initalize the squares 
        for (int x=0; x<x_size; ++x)
            for (int y=0; y<y_size; ++y)
            {
                grid[x][y] = rand() % 2 == 0;

                // Set edge to false
                if (x==0 || y==0 || x==x_size-1 || y==y_size-1)
                    grid[x][y] = false;

                // Start with new_grid = grid
                new_grid[x][y] = grid[x][y];
            }

        // Set vertex array positions
        vertex_array = new sf::VertexArray(sf::PrimitiveType::Triangles, 3*x_size*y_size);
        int i = 0;
        for (int x=0; x<x_size; ++x)
            for (int y=0; y<y_size; ++y)
            {
                point_indicies[x][y] = i;
                vertex_array->operator[](i+0).position = sf::Vector2f(x*square_size, y*square_size);
                vertex_array->operator[](i+1).position = sf::Vector2f((x+1)*square_size, y*square_size);
                vertex_array->operator[](i+2).position = sf::Vector2f((x+1)*square_size, (y+1)*square_size);
                vertex_array->operator[](i+0).color = sf::Color::Black;
                vertex_array->operator[](i+1).color = sf::Color::Black;
                vertex_array->operator[](i+2).color = sf::Color::Black;
                i += 3;
            }
    }

    ~board()
    {
        // Free memory
        for (int x=0; x<x_size; ++x)
        {
            delete[] grid[x];
            delete[] new_grid[x];
            delete[] point_indicies[x];
        }
        delete[] grid;
        delete[] new_grid;
        delete[] point_indicies;
        delete vertex_array;
    }

    /// Run one iteration of the game of life.
    void iterate()
    {
        // Update the grid (distribute across threads)
        std::thread threads[CPU_COUNT];
        for (unsigned i=0; i<CPU_COUNT; ++i)
        {
            int xmin = i * (x_size / CPU_COUNT);
            int xmax = (i+1) * (x_size / CPU_COUNT);
            if (i == CPU_COUNT - 1) xmax = x_size;
            threads[i] = std::thread([this, xmin, xmax] { 
                this->update_new_grid(xmin, xmax); });
        }

        for(unsigned i=0; i<CPU_COUNT; ++i)
            threads[i].join();
            
        // Copy new grid to active grid
        for (unsigned i=0; i<CPU_COUNT; ++i)
        {
            int xmin = i * (x_size / CPU_COUNT);
            int xmax = (i+1) * (x_size / CPU_COUNT);
            if (i == CPU_COUNT - 1) xmax = x_size;
            threads[i] = std::thread([this, xmin, xmax] { 
                this->copy_grid(xmin, xmax); });
        }

        for(unsigned i=0; i<CPU_COUNT; ++i)
            threads[i].join();
    }

    /// Draws the game of life board to the given render window.
    void draw(sf::RenderWindow &window)
    {
        window.draw(*vertex_array);
    }

};

/// Program entrypoint.
int main(int argc, char** argv)
{
    std::cout << "Running on " << CPU_COUNT << " cores.\n";
    int square_size = 1;
    for (int i=1; i<argc; ++i)
    {
        try
        {
            square_size = std::stoi(argv[i]);
        }
        catch (const std::exception& e)
        {
            std::cout << "Could not parse square size from \"" << argv[i] << "\"!\n";
        }
    }


    // Seed the random number generator
    srand(clock());

    // Create a fullscreen window
    sf::RenderWindow window(
        sf::VideoMode::getFullscreenModes()[0], 
        "Game of life", 
        sf::Style::Fullscreen);

    window.setVerticalSyncEnabled(false);

    // Create a board to fill the window
    int xsize = window.getSize().x/square_size;
    int ysize = window.getSize().y/square_size;
    board b(xsize, ysize, square_size);

    // Main loop
    while (window.isOpen())
    {
        // Handle events
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Update the board if enough time has passed
        b.iterate();
        window.clear();
        b.draw(window);
        window.display();
    }

    return 0;
}
