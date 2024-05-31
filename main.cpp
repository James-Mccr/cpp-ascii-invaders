#include "lib/console.h"
#include "lib/frame.h"
#include "lib/input.h"
#include "lib/render.h"
#include <chrono>
#include <cstdlib>
#include <string>
#include <random>
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

enum class InvaderState { Alive, Dead, Inactive };

class Invader
{
public:
    Invader(int _x, int _y)
    {
        x = _x;
        y = _y;
        state = InvaderState::Alive;
    }

    void Collide(Grid& grid)
    {
        if (state != InvaderState::Alive) return;
        if (grid.IsCollision(x, y, Bullet::ascii))
        {
            state = InvaderState::Dead;
            grid.SetTile(x, y); 
            return;
        }
    }

    void Update(const int speed, Grid& grid)
    {
        if (state == InvaderState::Alive && rand() <= BULLET_THRESHOLD)
            bullet.Fire(x, y, 1);
        bullet.Update(grid);
        if (state != InvaderState::Alive) return;
        grid.SetTile(x, y); 
        x += speed;
        grid.SetTile(x, y, ascii);
    }

    int GetX() const { return x; }
    int GetY() const { return y; }
    bool IsAlive() const { return state == InvaderState::Alive; }
    bool IsDead() const { return state == InvaderState::Dead; }
    void SetInactive() { state = InvaderState::Inactive; }

    static constexpr char ascii = '*';

private:
    int x{};
    int y{};
    InvaderState state{};
    Bullet bullet{};
    static constexpr int BULLET_THRESHOLD = 214748365;
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
        for (int i = 0; i < invaders.size(); i++)
        {
            invaders[i].Collide(grid);
            if (invaders[i].IsDead())
            {
                if (actionThreshold > 5)
                    actionThreshold--;
                invaders[i].SetInactive();
                deadInvaders++;
                break;
            }
        }    

        actionPoints += 1;
        if (actionPoints < actionThreshold) return;
        actionPoints = 0;

        int rightInvader = -1;
        int leftInvader = -1;
        for (int i = 0; i < invaders.size(); i++)
        {
            if (leftInvader < 0 && invaders[i].IsAlive())
            {
                leftInvader = i;
                rightInvader = i;
            }
            if (i > rightInvader && invaders[i].IsAlive())
                rightInvader = i;
            invaders[i].Update(speed, grid);
        }

        if (speed == 1 && invaders[rightInvader].GetX() == xMax)
            speed = -1;
        else if (speed == -1 && invaders[leftInvader].GetX() == 0)
            speed = 1;
    }

    const vector<Invader> GetInvaders() const { return invaders; }
    bool IsDestroyed() const { return invaders.size() == deadInvaders; }

private:
    int xMax{};
    int yMax{};
    int speed = 1;
    int actionPoints{};
    int actionThreshold{20};
    int deadInvaders{};
    vector<Invader> invaders{};
    vector<Bullet> bullets{};
};

class Player 
{
public:
    Player(int _xMax, int _yMax) : xMax(_xMax), yMax(_yMax)
    {
        x = xMax/2;
        y = yMax*0.9;
        alive = true;
    }

    void Collide(Grid& grid)
    {
        if (grid.IsCollision(x, y, Bullet::ascii))
        {
            alive = false;
            grid.SetTile(x, y); 
            return;
        }
    }

    void Update(UserInput userInput, Grid& grid)
    {
        Collide(grid);
        if (!alive) return;
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
    bool IsAlive() const { return alive; }

    static constexpr char ascii = '@';

private:
    int x{};
    int y{};
    int xMax{};
    int yMax{};
    int speed{};
    bool alive{};
    Bullet bullet;
};

enum class GameState { Running, Victory, Defeat };

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
        if (!player.IsAlive()) state = GameState::Defeat;
        fleet.Update(grid);
        if (fleet.IsDestroyed()) state = GameState::Victory;
    }

    const Grid& GetGrid() { return grid; }
    const bool IsRunning() { return state == GameState::Running; }
    const bool IsVictory() { return state == GameState::Victory; }

private:
    Grid grid;
    Player player;
    Fleet fleet;
    GameState state{};
};

int main()
{
    Console console{};
    Frame frame{60};
    Input input{};
    Render render{console};
    Game game{console.width, console.height};
    srand(time(nullptr));

    while(1)
    {
        frame.limit();

        UserInput userInput = input.Read();

        if (userInput == UserInput::Quit) return 0;

        game.Update(userInput);
            
        render.Draw(game.GetGrid().GetTiles() );

        if (!game.IsRunning())
        {
            console.moveCursor(console.height/2, console.width/4);
            if (game.IsVictory())
                console.print("You defeated the evil invaders! Hip-hip-hooray!");
            else
                console.print("The evil invaders have won. Goodbye world!");
            break;
        }
    }

    frame = {1};
    frame.limit();
    frame.limit();
    frame.limit();
    frame.limit();
    frame.limit();

    return 0;
}