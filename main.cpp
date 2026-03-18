#include <iostream>
#include <cmath>
#include <array>
#include <algorithm>
#include <SFML/Graphics.hpp>
// eventually add GLM for vector/matrix operations

const float g = 9.806;
const float G = 1.f;
const float offset = .1f;
float timeScale = 1.e0f;
float fps = 60.f;
const int subSteps = 4;
const bool debugTimeOutput = false;

const float minRest = .9f;
const float maxRest = .9f;

const float windowScale = .5f;
const float FULLWIDTH = sf::VideoMode::getDesktopMode().width;
const float FULLHEIGHT = sf::VideoMode::getDesktopMode().height;
sf::Vector2u size(FULLWIDTH * windowScale, FULLHEIGHT * windowScale);
bool mousePressed = false;
// meters to pixels conversion
float mToPx = FULLHEIGHT / 2;
float pxToM = 1 / mToPx;

// Number of balls
int nBalls = 200;
const float minR = 20.f;
const float maxR = 20.f;
const float maxV = 1.f;
const float density = 10.f;

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


sf::Vector2f findAccel(const sf::Vector2f& cPos);
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
    sf::Clock renderClock;
    
    float renderDt = 1.f / fps;
    
    sf::Clock metricsClock;
    int physicsTicks = 0;
    int renderFrames = 0;

    // physics vars
    sf::Vector2f pxPos;
    float radius;

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
            if (event.type == sf::Event::MouseButtonPressed) {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                    mousePressed = !mousePressed;
                }
            }
        }
        float dt = clock.restart().asSeconds() * timeScale;
        float subDt = dt / subSteps;
        physicsTicks++;
        
        // physics
        for (int i = 0; i < nBalls; i++) {
            sf::Vector2f pos = balls.positions[i];
            sf::Vector2f vel = balls.velocities[i];
            
            sf::Vector2f acc = findAccel(pos);
            sf::Vector2f vHalf = vel + 0.5f * acc * dt;
            pos += vHalf * dt;
            acc = findAccel(pos);
            vel = vHalf + 0.5f * acc * dt;
            
            balls.positions[i] = pos;
            balls.velocities[i] = vel;
        }

        for (int step = 0; step < subSteps; step++) {

            // collisions
            for (int i = 0; i < nBalls; i++) {
                /* // color management [old]
                std::vector<sf::Vector2f>::iterator maxIt = std::max_element(balls.velocities.begin(), balls.velocities.end(), comp);
                float maxVel = norm(*maxIt);
                float c = 255 * norm(vel) / maxVel;
                balls.colors[i] = sf::Color(255, c, c, 255);
                balls.circles[i].setFillColor(balls.colors[i]);
                */

                // ball collisions
                float r1 = balls.radiuses[i] * pxToM;
                float invM1 = 1.0f / balls.masses[i];
                float rest1 = balls.restitutions[i];

                sf::Vector2f& p1 = balls.positions[i]; 
                sf::Vector2f& v1 = balls.velocities[i];

                for (int j = i + 1; j < nBalls; j++) {
                    sf::Vector2f& p2 = balls.positions[j];
                    sf::Vector2f& v2 = balls.velocities[j];

                    float dx = p2.x - p1.x;
                    float dy = p2.y - p1.y;
                    float distSq = dx * dx + dy * dy;
                    
                    float r2 = balls.radiuses[j] * pxToM;
                    float radDist = r1 + r2;

                    if (distSq < radDist * radDist && distSq > 0.00001f) {
                        float distance = std::sqrt(distSq);
                        float invDist = 1.0f / distance;
                        
                        float nx = dx * invDist;
                        float ny = dy * invDist;
                        
                        float invM2 = 1.0f / balls.masses[j];
                        float sumInvMass = invM1 + invM2;

                        // distancing
                        float overlap = radDist - distance;
                        float correction = overlap / sumInvMass;
                        float cx = nx * correction;
                        float cy = ny * correction;
                        
                        p1.x -= cx * invM1;
                        p1.y -= cy * invM1;
                        p2.x += cx * invM2;
                        p2.y += cy * invM2;
                        
                        // collision management
                        float dvx = v2.x - v1.x;
                        float dvy = v2.y - v1.y;
                        float dotProd = dvx * nx + dvy * ny;

                        if (dotProd < 0.f) { 
                            float e = rest1 * balls.restitutions[j];
                            float J = -(1.f + e) * dotProd / sumInvMass;
                            
                            float Jnx = J * nx;
                            float Jny = J * ny;
                            
                            v1.x -= Jnx * invM1;
                            v1.y -= Jny * invM1;
                            v2.x += Jnx * invM2;
                            v2.y += Jny * invM2;
                        }
                    }
                }
            }

            // border collisions
            for (int i = 0; i < nBalls; i++) {
                sf::Vector2f& p = balls.positions[i];
                sf::Vector2f& v = balls.velocities[i];
                float radius = balls.radiuses[i];
                float rest = balls.restitutions[i];
                
                float px = p.x * mToPx;
                float py = p.y * mToPx;
                
                if (px < radius + windowPos.x) {
                    v.x = -v.x * rest;
                    px = radius + windowPos.x;
                } else if (px > size.x - radius + windowPos.x) {
                    v.x = -v.x * rest;
                    px = size.x - radius + windowPos.x;
                }
                if (py < radius + windowPos.y)  {
                    v.y = -v.y * rest;
                    py = radius + windowPos.y;
                } else if (py > size.y - radius + windowPos.y) {
                    v.y = -v.y * rest;
                    py = size.y - radius + windowPos.y;
                }
                
                p.x = px * pxToM;
                p.y = py * pxToM;
            }
        }

        // graphics rendering
        if (renderClock.getElapsedTime().asSeconds() >= renderDt) {
            renderFrames++;

            windowPos = window.getPosition();

            window.clear();

            for (int i = 0; i < nBalls; i++) {
                pxPos = balls.positions[i] * mToPx;
                balls.circles[i].setPosition(pxPos.x - windowPos.x, pxPos.y - windowPos.y);
                window.draw(balls.circles[i]);
            }

            window.display();
            renderClock.restart();
        }

        if (metricsClock.getElapsedTime().asSeconds() >= 1.0f && debugTimeOutput) {
            std::cout << "FPS: " << renderFrames
                      << " | Physics Ticks (UPS): " << physicsTicks 
                      << " | Rapporto: " << (float)physicsTicks / renderFrames << " calcoli per frame\n";
            
            // Resetta i contatori per il secondo successivo
            physicsTicks = 0;
            renderFrames = 0;
            metricsClock.restart();
        }
    }
}

sf::Vector2f findAccel(const sf::Vector2f& cPos) {
    // return sf::Vector2f(0.f, g);
    
    if (mousePressed) {
        sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition()) * pxToM;
        float dx = cPos.x - mousePos.x;
        float dy = cPos.y - mousePos.y;
        float dSqr = dx * dx + dy * dy;
        float acc = G / (dSqr + offset);
        float d = std::sqrt(dSqr);
        return sf::Vector2f(-dx*acc/d, -dy*acc/d);
    }
    return sf::Vector2f(0.f, 0.f);
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