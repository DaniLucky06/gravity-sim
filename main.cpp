#include <iostream>
#include <cmath>
#include <array>
#include <SFML/Graphics.hpp>
// eventually add GLM for vector/matrix operations

const float g = 9.806;
float timeScale = 1.e0f;
float maxRest = 1.f;

const float windowScale = .75f;
float FULLWIDTH = sf::VideoMode::getDesktopMode().width;
float FULLHEIGHT = sf::VideoMode::getDesktopMode().height;
float WIDTH = FULLWIDTH * windowScale;
float HEIGHT = FULLHEIGHT * windowScale;
// meters to pixels conversion
float mToPx = HEIGHT / 2;
float pxToM = 1 / mToPx;

// Number of balls
int nBalls = 10;
float minR = 10.f;
float maxR = 50.f;
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

    void randomInit(int n, float minR, float maxR, float maxV, float density) {
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
            positions[i] = sf::Vector2f((X*(WIDTH - 2*radius) + radius)*pxToM, (Y*(HEIGHT - 2*radius) + radius)*pxToM);
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
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "SFML window");
    sf::View fixedView(sf::FloatRect(0.f, 0.f, WIDTH, HEIGHT));
    window.setView(fixedView);
    bool isFullscreen = false;
    sf::Vector2i windowPosPrev = window.getPosition();
    sf::Vector2i windowPosNow = windowPosPrev;
    sf::Vector2f windowVelNow(0.f, 0.f);
    sf::Vector2f windowVelPrev(0.f, 0.f);
    sf::Vector2f windowVelMet, windowAcc, fakeAcc;

    Balls balls{};
    balls.randomInit(nBalls, minR, maxR, maxV, density);

    // time
    sf::Clock clock;
    float dt;

    // physics vars
    sf::Vector2f pos, pxPos, vel, acc, vHalf;
    float rest;

    while (window.isOpen())
    {
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) // closing window logic
        {
            // Close window: exit
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::F11) {
                            isFullscreen = !isFullscreen;
                    
                            if (isFullscreen) {
                                // Switch to Fullscreen
                                window.create(sf::VideoMode(WIDTH, HEIGHT), "SFML Window", sf::Style::Fullscreen);
                                window.setView(fixedView);
                            } else {
                                // Switch back to Windowed
                                window.create(sf::VideoMode(WIDTH, HEIGHT), "SFML Window", sf::Style::Default);
                                window.setView(fixedView);
                            }
                }
            }
        }
        dt = clock.restart().asSeconds() * timeScale;
        
        // clear screen
        window.clear();
        
        // window dragging
        windowPosNow = window.getPosition();

        windowVelNow = sf::Vector2f(
            (windowPosNow.x - windowPosPrev.x) / dt,
            (windowPosNow.y - windowPosPrev.y) / dt
        );

        windowAcc = -(windowVelNow - windowVelPrev) / dt;
        fakeAcc = windowAcc * pxToM;
        fakeAcc.y = -fakeAcc.y;
        
        windowPosPrev = windowPosNow;
        windowVelPrev = windowVelNow;

        // physics
        for (int i = 0; i < nBalls; i++) {
            float radius = balls.radiuses[i];
            pos = balls.positions[i];
            vel = balls.velocities[i];
            acc = findAccel(pos) + fakeAcc;
            vHalf = vel + .5f * acc * dt;
            pos = pos + vHalf * dt;
            acc = findAccel(pos) + fakeAcc;
            vel = vHalf + .5f * acc * dt;
            pxPos = pos * mToPx;

            // border collisions
            windowVelMet = windowVelNow * pxToM;
            windowVelMet.x = -windowVelMet.x;
            rest = balls.restitutions[i];
            if (std::abs(vel.y) > 0.0001f) {
                if (pxPos.y < radius)  {
                    vel.y = -vel.y * rest;
                    pxPos.y = radius;
                } else if (pxPos.y > HEIGHT - radius) {
                    vel.y = -vel.y * rest;
                    pxPos.y = HEIGHT - radius;
                }
            }
            if (std::abs(vel.x) > 0.0001f) {
                if (pxPos.x < radius) {
                    vel.x = -vel.x * rest;
                    pxPos.x = radius;
                } else if (pxPos.x > WIDTH - radius) {
                    vel.x = -vel.x * rest;
                    pxPos.x = WIDTH - radius;
                }
            }
            pos = pxPos * pxToM;
            balls.positions[i] = pos;
            balls.velocities[i] = vel;

            // change render positions
            balls.circles[i].setPosition(pxPos.x, HEIGHT - pxPos.y);
            window.draw(balls.circles[i]);
        }

        // windowPos = window.getPosition();

        // Update the window
        window.display();
    }
}

sf::Vector2f findAccel(sf::Vector2f& cPos) {
    return sf::Vector2f(0.f, -g);
}