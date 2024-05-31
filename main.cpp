#include "lib/console.h"
#include "lib/frame.h"
#include "lib/input.h"
#include "lib/render.h"
#include <string>
#include <vector>

using namespace std;

class Grid
{
public:
    Grid(int _width, int _height)
    {
        width = _width;
        height = _height;
        tiles = vector(height, string(width, empty));
    }

    const vector<string> GetTiles() const { return tiles; }

    void SetTile(int x, int y, char c = empty)
    {
        tiles[y][x] = c;
    }

    bool IsOutOfBounds(int x, int y)
    {
        return x < 0 || x >= width || y < 0 || y >= height;
    }

    bool IsCollision(int x, int y, char c)
    {
        return tiles[y][x] == c;
    }

private:
    static constexpr char empty = ' ';
    int width{};
    int height{};
    vector<string> tiles;
};

class Bullet
{
public:
    Bullet() {}

    void Fire(int _x, int _y, int _speed)
    {
        if (active) return;
        active = true;
        x = _x;
        y = _y;
        speed = _speed;
    }

    void Update(Grid& grid)
    {            
        if (!active) return;
        grid.SetTile(x, y);
        y += speed;
        if (grid.IsOutOfBounds(x, y))
        {
            active = false;
            return;
        }
        grid.SetTile(x, y, ascii);
    }

    static constexpr char ascii = '|';

private:
    int x{};
    int y{};
    int speed{};
    bool active{};
};

class Invader
{
public:
    Invader(int _x, int _y)
    {
        x = _x;
        y = _y;
        alive = true;
    }

    void Collide(Grid& grid)
    {
        if (!alive) return;
        if (grid.IsCollision(x, y, Bullet::ascii))
        {
            alive = false;
            grid.SetTile(x, y); 
            return;
        }
    }

    void Update(const int speed, Grid& grid)
    {
        if (!alive) return;
        grid.SetTile(x, y); 
        x += speed;
        grid.SetTile(x, y, ascii);
    }

    int GetX() const { return x; }
    int GetY() const { return y; }
    bool IsAlive() const { return alive; }

    static constexpr char ascii = '*';

private:
    int x{};
    int y{};
    bool alive{};
};

class Fleet
{
public:
    Fleet(int _xMax, int _yMax)
    {
        xMax = _xMax;
        yMax = _yMax;
        int y = _yMax*0.1;
        for (int x = _xMax*0.15; x < _xMax*0.85; x+=2)
            invaders.push_back(Invader{x, y});
        actionPoints = actionThreshold;
    }

    void Update(Grid& grid)
    {
        int indexToDelete = -1;
        for (int i = 0; i < invaders.size(); i++)
        {
            invaders[i].Collide(grid);
            if (!invaders[i].IsAlive())
            {
                indexToDelete = i;
                break;
            }
        }
        if (indexToDelete >= 0)
            invaders.erase(invaders.begin()+indexToDelete);            

        actionPoints += 1;
        if (actionPoints < actionThreshold) return;
        actionPoints = 0;

        for (auto& invader : invaders)
            invader.Update(speed, grid);

        if (speed == 1 && invaders.back().GetX() == xMax)
            speed = -1;
        else if (speed == -1 && invaders.front().GetX() == 0)
            speed = 1;
    }

    const vector<Invader> GetInvaders() const { return invaders; }

private:
    int xMax{};
    int yMax{};
    int speed = 1;
    int actionPoints{};
    int actionThreshold{20};
    vector<Invader> invaders{};
};

class Player 
{
public:
    Player(int _xMax, int _yMax) : xMax(_xMax), yMax(_yMax)
    {
        x = xMax/2;
        y = yMax*0.9;
    }

    void Update(UserInput userInput, Grid& grid)
    {
        grid.SetTile(x, y);
        if (userInput == UserInput::Left)
            x += -1;
        else if (userInput == UserInput::Right)
            x += 1;
        grid.SetTile(x, y, ascii);

        if (userInput == UserInput::Up)
            bullet.Fire(x, y, -1);
        bullet.Update(grid);
    }

    int GetX() const { return x; }
    int GetY() const { return y; }

    static constexpr char ascii = '@';

private:
    int x{};
    int y{};
    int xMax{};
    int yMax{};
    int speed{};
    Bullet bullet;
};

class Game
{
public:
    Game(int _width, int _height) : 
        player(_width-1, _height-1),
        fleet(_width-1, _height-1),
        grid(_width, _height)
    {}        

    void Update(UserInput userInput)
    {
        player.Update(userInput, grid);
        fleet.Update(grid);
    }

    const Grid GetGrid() { return grid; }

private:
    Grid grid;
    Player player;
    Fleet fleet;
    
};

int main()
{
    Console console{};
    Frame frame{60};
    Input input{};
    Render render{console};
    Game game{console.width, console.height};

    while(1)
    {
        frame.limit();

        UserInput userInput = input.Read();

        if (userInput == UserInput::Quit) break;

        game.Update(userInput);

        render.Draw(game.GetGrid().GetTiles() );
    }

    frame.limit();

    return 0;
}