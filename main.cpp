#include <iostream>
#include <cmath>
#include <array>
#include <algorithm>
#include <SFML/Graphics.hpp>
// eventually add GLM for vector/matrix operations

const float g = 19.806;
float timeScale = 1.e-1f;
float minRest = 1.f;
float maxRest = .98f;

const float windowScale = .5f;
float FULLWIDTH = sf::VideoMode::getDesktopMode().width;
float FULLHEIGHT = sf::VideoMode::getDesktopMode().height;
sf::Vector2u size(FULLWIDTH * windowScale, FULLHEIGHT * windowScale);
// meters to pixels conversion
float mToPx = FULLHEIGHT / 2;
float pxToM = 1 / mToPx;

// Number of balls
int nBalls = 500;
float minR = 20.f;
float maxR = 20.f;
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
            // sf::Color color(rand()%256, rand()%256, rand()%256, rand()%256);
            sf::Color color(255, 255, 255, 255);
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
float norm(sf::Vector2f vec);
float dot(sf::Vector2f V1, sf::Vector2f V2);
bool comp(const sf::Vector2f& v1, const sf::Vector2f& v2);

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
            pos = balls.positions[i];
            vel = balls.velocities[i];
            /*
            std::vector<sf::Vector2f>::iterator maxIt = std::max_element(balls.velocities.begin(), balls.velocities.end(), comp);
            float maxVel = norm(*maxIt);
            float c = 255 * norm(vel) / maxVel;
            balls.colors[i] = sf::Color(255, c, c, 255);
            balls.circles[i].setFillColor(balls.colors[i]);
            */
            radius = balls.radiuses[i];
            acc = findAccel(pos);
            vHalf = vel + .5f * acc * dt;
            pos = pos + vHalf * dt;
            acc = findAccel(pos);
            vel = vHalf + .5f * acc * dt;
            balls.positions[i] = pos;
            balls.velocities[i] = vel;

            // ball
            rest = balls.restitutions[i];
            for (int j = i + 1; j < nBalls; j++) {
                sf::Vector2f direction = balls.positions[j] - balls.positions[i]; // m
                float distance = norm(direction); // m
                float r1 = balls.radiuses[i] * pxToM; // m
                float r2 = balls.radiuses[j] * pxToM; // m
                
                float radDist = (r2 + r1); // m
                if (distance < radDist) {
                    // distancing
                    sf::Vector2f n = direction / distance; // m
                    balls.positions[i] += -1.f * r1 * n * (1 - distance/radDist);
                    balls.positions[j] += 1.f * r2 * n * (1 - distance/radDist); // m/m*px * m/px = m
                    
                    // collision management
                    sf::Vector2f v1 = balls.velocities[i]; // m/s
                    sf::Vector2f v2 = balls.velocities[j]; // m/s
                    float m1 = balls.masses[i];
                    float m2 = balls.masses[j];
                    float e = balls.restitutions[i] * balls.restitutions[j];

                    float J = -(1 + e) * dot(v2 - v1, n) / (1 / m1 + 1 / m2);
                    balls.velocities[i] = v1 - J/m1 * n;
                    balls.velocities[j] = v2 + J/m2 * n;
                }
            }

            // border collisions
            vel = balls.velocities[i];
            pxPos = balls.positions[i] * mToPx;
            if (pxPos.x < radius + windowPos.x) {
                vel.x = -vel.x * rest;
                pxPos.x = radius + windowPos.x;
            } else if (pxPos.x > size.x - radius + windowPos.x) {
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
            balls.positions[i] = pxPos * pxToM;
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

float norm(sf::Vector2f vec) {
    return sqrt(vec.x * vec.x + vec.y * vec.y);
}

bool comp(const sf::Vector2f& v1, const sf::Vector2f& v2) {
    return norm(v1) < norm(v2);
}

float dot(sf::Vector2f V1, sf::Vector2f V2) {
    return V1.x * V2.x + V1.y * V2.y;
}