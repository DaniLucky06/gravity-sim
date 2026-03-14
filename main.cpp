#include <iostream>
#include <cmath>
#include <array>
#include <SFML/Graphics.hpp>
// eventually add GLM for vector/matrix operations

const float g = 9.806;
float timeScale = 1.e0f;
float maxRest = 1.f;

const float windowScale = .5f;
float FULLWIDTH = sf::VideoMode::getDesktopMode().width;
float FULLHEIGHT = sf::VideoMode::getDesktopMode().height;
sf::Vector2u size(FULLWIDTH * windowScale, FULLHEIGHT * windowScale);
// meters to pixels conversion
float mToPx = FULLHEIGHT / 2;
float pxToM = 1 / mToPx;

// Number of balls
int nBalls = 10;
float minR = 10.f;
float maxR = 200.f;
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

    void randomInit(sf::Window& window, int n, float minR, float maxR, float maxV, float density) {
        radiuses.resize(n);
        masses.resize(n);
        colors.resize(n);
        positions.resize(n);
        velocities.resize(n);
        circles.resize(n);
        restitutions.resize(n);
        
        srand(time(0));
        float iRAND_MAX = 1.f / RAND_MAX;
        for (int i = 0; i < n; i++) {
            float radius = (rand() * iRAND_MAX * (maxR - minR)) + minR;
            radiuses[i] = radius;
            masses[i] = radius * radius * M_PI * density;
            sf::Color color(rand()%256, rand()%256, rand()%256, rand()%256);
            colors[i] = color;
            float X = rand() * iRAND_MAX;
            float Y = rand() * iRAND_MAX;
            positions[i] = (sf::Vector2f((X*(size.x - 2*radius) + radius), (Y*(size.y - 2*radius) + radius)) + static_cast<sf::Vector2f>(window.getPosition())) * pxToM;
            velocities[i] = sf::Vector2f((rand()*iRAND_MAX*2.f - 1)*maxV, (rand()*iRAND_MAX*2.f - 1)*maxV);
            restitutions[i] = 1.f;

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
    sf::Vector2i windowPosPrev = window.getPosition();
    sf::Vector2i windowPosNow = windowPosPrev;
    sf::Vector2f windowVel(0.f, 0.f);
    sf::Vector2f windowVelMet;

    Balls balls{};
    balls.randomInit(window, nBalls, minR, maxR, maxV, density);

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
        windowPosNow = window.getPosition();

        windowVel = sf::Vector2f(
            (windowPosNow.x - windowPosPrev.x) / dt,
            (windowPosNow.y - windowPosPrev.y) / dt
        );
        windowPosPrev = windowPosNow;
        windowVelMet = windowVel * pxToM;
        
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

            if (pxPos.x < radius + windowPosNow.x) {
                vel.x = -vel.x * rest;
                pxPos.x = radius + windowPosNow.x;
            } else if (pxPos.x > size.x + windowPosNow.x) {
                vel.x = -vel.x * rest;
                pxPos.x = size.x - radius + windowPosNow.x;
            }
            if (pxPos.y < radius + windowPosNow.y)  {
                vel.y = -vel.y * rest;
                pxPos.y = radius + windowPosNow.y;
            } else if (pxPos.y > size.y - radius + windowPosNow.y) {
                vel.y = -vel.y * rest;
                pxPos.y = size.y - radius + windowPosNow.y;
            }
            pos = pxPos * pxToM;
            balls.positions[i] = pos;
            balls.velocities[i] = vel;

            // change render positions
            balls.circles[i].setPosition(pxPos.x - windowPosNow.x, pxPos.y - windowPosNow.y);
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