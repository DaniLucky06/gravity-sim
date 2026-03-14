#include <iostream>
#include <cmath>
#include <array>
#include <SFML/Graphics.hpp>
// eventually add GLM for vector/matrix operations

const float g = 9.806;
float timeScale = 1.e0f;
float minRest = .8f;
float maxRest = .99f;

const float windowScale = .5f;
float FULLWIDTH = sf::VideoMode::getDesktopMode().width;
float FULLHEIGHT = sf::VideoMode::getDesktopMode().height;
sf::Vector2u size(FULLWIDTH * windowScale, FULLHEIGHT * windowScale);
// meters to pixels conversion
float mToPx = FULLHEIGHT / 2;
float pxToM = 1 / mToPx;

// Number of balls
int nBalls = 10;
float minR = 30.f;
float maxR = 60.f;
float maxV = 3.f;
float density = 10.f;

struct Balls {
    std::vector<float> radiuses;
    std::vector<float> masses;
    std::vector<float> restitutions;
    std::vector<sf::Color> colors;
    std::vector<sf::Vector2f> positions;
    std::vector<sf::Vector2f> velocities;
    std::vector<sf::CircleShape> circles;

    void randomInit(sf::Window& window, int n, float minR, float maxR, float maxV, float density, float minRest, float maxRest) {
        radiuses.resize(n);
        masses.resize(n);
        colors.resize(n);
        positions.resize(n);
        velocities.resize(n);
        circles.resize(n);
        restitutions.resize(n);
        
        srand(time(0));
        float fRAND_MAX = static_cast<float>(RAND_MAX);
        for (int i = 0; i < n; i++) {
            float radius = (rand() / fRAND_MAX * (maxR - minR)) + minR;
            radiuses[i] = radius;
            masses[i] = radius * radius * M_PI * density;
            sf::Color color(rand()%256, rand()%256, rand()%256, rand()%256);
            colors[i] = color;
            float X = rand() / fRAND_MAX;
            float Y = rand() / fRAND_MAX;
            positions[i] = (sf::Vector2f((X*(size.x - 2*radius) + radius), (Y*(size.y - 2*radius) + radius)) + static_cast<sf::Vector2f>(window.getPosition())) * pxToM;
            velocities[i] = sf::Vector2f((rand()/fRAND_MAX*2.f - 1)*maxV, (rand()/fRAND_MAX*2.f - 1)*maxV);
            restitutions[i] = (rand() / fRAND_MAX) * (maxRest - minRest) + minRest;

            // update circles
            circles[i].setRadius(radius);
            circles[i].setFillColor(color);
            circles[i].setOrigin(radius, radius);
        }
    }
};

sf::Vector2f findAccel(sf::Vector2f& cPos);

int main(int, char**){
    sf::RenderWindow window(sf::VideoMode(size.x, size.y), "Bawls");
    sf::View fixedView(sf::FloatRect(0.f, 0.f, size.x, size.y));
    window.setView(fixedView);
    sf::Vector2i windowPos = window.getPosition();

    Balls balls{};
    balls.randomInit(window, nBalls, minR, maxR, maxV, density, minRest, maxRest);

    // time
    sf::Clock clock;
    float dt;

    // physics vars
    sf::Vector2f pos, pxPos, vel, acc, vHalf;
    float rest, radius;

    while (window.isOpen())
    {
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) // closing window logic
        {
            // Close window: exit
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::Resized){
                size = window.getSize();
                window.setView(sf::View(sf::FloatRect(0.f, 0.f, size.x, size.y)));
            }
            if (event.type == sf::Event::KeyPressed) {
            }
        }
        dt = clock.restart().asSeconds() * timeScale;
        
        // clear screen
        window.clear();
        
        // window dragging
        windowPos = window.getPosition();
        
        // physics
        for (int i = 0; i < nBalls; i++) {
            radius = balls.radiuses[i];
            pos = balls.positions[i];
            vel = balls.velocities[i];
            acc = findAccel(pos);
            vHalf = vel + .5f * acc * dt;
            pos = pos + vHalf * dt;
            acc = findAccel(pos);
            vel = vHalf + .5f * acc * dt;
            pxPos = pos * mToPx;

            // border collisions
            rest = balls.restitutions[i];

            if (pxPos.x < radius + windowPos.x) {
                vel.x = -vel.x * rest;
                pxPos.x = radius + windowPos.x;
            } else if (pxPos.x > size.x + windowPos.x) {
                vel.x = -vel.x * rest;
                pxPos.x = size.x - radius + windowPos.x;
            }
            if (pxPos.y < radius + windowPos.y)  {
                vel.y = -vel.y * rest;
                pxPos.y = radius + windowPos.y;
            } else if (pxPos.y > size.y - radius + windowPos.y) {
                vel.y = -vel.y * rest;
                pxPos.y = size.y - radius + windowPos.y;
            }
            pos = pxPos * pxToM;
            balls.positions[i] = pos;
            balls.velocities[i] = vel;

            // change render positions
            balls.circles[i].setPosition(pxPos.x - windowPos.x, pxPos.y - windowPos.y);
            window.draw(balls.circles[i]);
        }

        // windowPos = window.getPosition();

        // Update the window
        window.display();
    }
}

sf::Vector2f findAccel(sf::Vector2f& cPos) {
    return sf::Vector2f(0.f, g);
}