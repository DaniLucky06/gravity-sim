#include <iostream>
#include <cmath>
#include <array>
#include <algorithm>
#include <SFML/Graphics.hpp>  

#include "grid.hpp"
// eventually add GLM for vector/matrix operations

float g = 9.806e0f;
float G = 1.f;
float offset = .01f;
float timeScale = 1.e0f;
float fps = 60.f;
float ms = 1.f / fps;
float fixedDt = 0.00015f;

int subSteps = 1;
int treeDepth = 6;
int maxEltPerNode = 3;
bool debugTimeOutput = false;

float minRest = 0.9f;
float maxRest = 0.9f;

float windowScale = .5f;
unsigned int FULLWIDTHPX = sf::VideoMode::getDesktopMode().size.x;
unsigned int FULLHEIGHTPX = sf::VideoMode::getDesktopMode().size.y;
sf::Vector2u windowSize(FULLWIDTHPX * windowScale, FULLHEIGHTPX * windowScale);
bool mousePressed = false;

// EVERYTHING IN METERS. FOR GRAPHICS CALCULATIONS, THE VALUES ARE CONVERTED!!!
// METRIC AND PIXEL SYSTEM START WITH 0 IN TOP LEFT OF --- MONITOR ---, NOT WINDOW

// meters to pixels conversion
float mToPx = FULLHEIGHTPX / 2;
float pxToM = 1 / mToPx;
float FULLWIDTH = FULLWIDTHPX * pxToM;
float FULLHEIGHT = FULLHEIGHTPX * pxToM;

// Number of balls
int nBalls = 2000;

// Radius params
float minR = 5.f;
float maxR = 5.f;

float maxV = 1.e0f;

float density = 1.e-1f;
bool gravityRadial = false;

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
            positions[i] = (sf::Vector2f((X*(windowSize.x - 2*radius) + radius), (Y*(windowSize.y - 2*radius) + radius)) + static_cast<sf::Vector2f>(window.getPosition())) * pxToM;
            velocities[i] = sf::Vector2f((rand()/fRAND_MAX*2.f - 1)*maxV, (rand()/fRAND_MAX*2.f - 1)*maxV);
            restitutions[i] = (rand() / fRAND_MAX) * (maxRest - minRest) + minRest;

            // update circles
            circles[i].setRadius(radius);
            circles[i].setFillColor(color);
            circles[i].setOrigin({radius, radius});
        }
    }
};

const int xNum = std::ceil(FULLWIDTH / (2 * maxR));
const int yNum = std::ceil(FULLHEIGHT / (2 * maxR));

Grid grid(FULLWIDTHPX, FULLHEIGHTPX, xNum, yNum);

void elementInit();
sf::Vector2f findAccel(const sf::Vector2f& cPos);

float norm(sf::Vector2f vec);
float dot(sf::Vector2f V1, sf::Vector2f V2);
bool comp(const sf::Vector2f& v1, const sf::Vector2f& v2);

void parseArguments(int argc, char* argv[]);

sf::RenderWindow window(sf::VideoMode({windowSize.x, windowSize.y}), "Bawls");
int main(int argc, char* argv[]) {
    parseArguments(argc, argv);

    sf::View fixedView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)}));
    window.setView(fixedView);
    
    // time
    sf::Clock clock;
    sf::Clock renderClock;
    
    sf::Clock metricsClock;
    int physicsTicks = 0;
    int renderFrames = 0;

    std::vector<uint64_t> collisionPairs;
    
    while (window.isOpen())
    {
        // Process events
        while (const std::optional<sf::Event> event = window.pollEvent()) // closing window logic
        {
            // Close window: exit
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } 
            else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                windowSize = resized->size;
                window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)})));
            } 
            else if (const auto* mousePressedEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressedEvent->button == sf::Mouse::Button::Left) mousePressed = !mousePressed;
            }
        }

        sf::Vector2i windowPos = window.getPosition();
        float timePassed = clock.restart().asSeconds() * timeScale;
        
        bool render = renderClock.getElapsedTime().asSeconds() >= ms;
        if (render) window.clear();

        // physics, graphics, border collisions
        for (int i = 0; i < nBalls; i++) {
            grid.remove(i);
            Element& element = grid.elements[i];

            sf::Vector2f pos = {element.cx, element.cy};
            sf::Vector2f vel = {element.vx, element.vy};
            
            sf::Vector2f acc = findAccel(pos);
            sf::Vector2f vHalf = vel + 0.5f * acc * fixedDt;
            pos += vHalf * fixedDt;
            acc = findAccel(pos);
            vel = vHalf + 0.5f * acc * fixedDt;

            // border collisions
            float radius = element.radius * mToPx;
            float rest = element.rest;
            
            float px = pos.x * mToPx;
            float py = pos.y * mToPx;
            
            if (px < radius + windowPos.x) {
                if (vel.x < 0) vel.x = -vel.x * rest;
                vel.x *= rest;

                px = radius + windowPos.x;
            } else if (px > windowSize.x - radius + windowPos.x) {
                if (vel.x > 0) vel.x = -vel.x * rest;
                vel.x *= rest;

                px = windowSize.x - radius + windowPos.x;
            }
            if (py < radius + windowPos.y)  {
                if (vel.y < 0) vel.y = -vel.y * rest;
                vel.y *= rest;

                py = radius + windowPos.y;
            } else if (py > windowSize.y - radius + windowPos.y) {
                if (vel.y > 0) vel.y = -vel.y * rest;
                vel.y *= rest;

                py = windowSize.y - radius + windowPos.y;
            }
            
            pos.x = px * pxToM;
            pos.y = py * pxToM;

            // rendering
            if (render) {
                sf::CircleShape circle(radius, 20);
                circle.setPosition(sf::Vector2f({px, py}) - static_cast<sf::Vector2f>(windowPos));
                circle.setFillColor(element.color);

                window.draw(circle);
            }
            
            grid.elements[i].cx = pos.x;
            grid.elements[i].cy = pos.y;
            grid.elements[i].vx = vel.x;
            grid.elements[i].vy = vel.y;
            grid.insert(i);
        }

        if (render) {
            window.display();
            renderClock.restart();
        }

        // ball collisions BROAD
        for (int row = 0; row < grid.yNum; row++) {
            for (int col = 0; col < grid.xNum; col++) {
                uint32_t eltRefId1 = grid.cells[row * xNum + col];

                // loop until we reach the last element in the cell
                while (eltRefId1 != -1) {
                    ElementRef& eltRef1 = grid.rowElements[row][eltRefId1];
                    uint32_t eltRefId2 = eltRef1.nextInCell;
                    
                    // if eltRefId
                    while (eltRefId2 != -1) {
                        ElementRef& eltRef2 = grid.rowElements[row][eltRefId2];

                        collisionPairs.push_back((eltRef1.ref << 32) | eltRef2.ref); // insert a hash of the two indexes

                        eltRefId2 = eltRef2.nextInCell;
                    }

                    eltRefId1 = eltRef1.nextInCell;
                }
            }
        }

        // ball collisions NARROW
        std::sort(collisionPairs.begin(), collisionPairs.end());
        // set previous collison pair to not be the first one
        uint64_t previousCollisionPair = ~collisionPairs[0];
        for (uint64_t collisionPair : collisionPairs) {
            // skip if we already checked the collision
            if (collisionPair == previousCollisionPair) continue;

            uint32_t ballId1 = static_cast<uint32_t>(collisionPair >> 32); // first ball index
            uint32_t ballId2 = static_cast<uint32_t>(collisionPair & 0xFFFFFFFF); // second ball index

            Element& ball1 = grid.elements[ballId1];
            Element& ball2 = grid.elements[ballId2];

            float dx = ball2.cx - ball1.cx;
            float dy = ball2.cy - ball1.cy;

            float r1 = ball1.radius;
            float r2 = ball2.radius;

            float distSq = dx * dx + dy * dy;
            float radDist = r1 + r2;

            // skip to next pair if they don't collide
            if (distSq >= radDist * radDist || distSq <= 0.00001f) {
                previousCollisionPair = collisionPair;
                continue;
            }

            grid.remove(ballId1);
            grid.remove(ballId2);

            float distance = std::sqrt(distSq);
            float invDist = 1.0f / distance;
            
            float nx = dx * invDist;
            float ny = dy * invDist;
            
            float invM1 = 1.0f / ball1.mass;
            float invM2 = 1.0f / ball2.mass;
            float sumInvMass = invM1 + invM2;

            // distancing
            float overlap = radDist - distance;
            float correction = overlap / sumInvMass;
            float cx = nx * correction;
            float cy = ny * correction;
            
            ball1.cx -= cx * invM1;
            ball1.cy -= cy * invM1;
            ball2.cx += cx * invM2;
            ball2.cy += cy * invM2;
            
            // speed management
            float dvx = ball2.vx - ball1.vx;
            float dvy = ball2.vy - ball1.vy;
            float dotProd = dvx * nx + dvy * ny;

            // if they are going apart, don't change their velocities
            if (dotProd >= 0.f) {
                previousCollisionPair = collisionPair;
                continue;
            }

            float e = ball1.rest * ball2.rest;
            float J = -(1.f + e) * dotProd / sumInvMass;
            
            float Jnx = J * nx;
            float Jny = J * ny;
            
            ball1.vx -= Jnx * invM1;
            ball1.vy -= Jny * invM1;
            ball2.vx += Jnx * invM2;
            ball2.vy += Jny * invM2;

            previousCollisionPair = collisionPair;
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

void elementInit() {
    srand(time(0));
    float fRAND_MAX = static_cast<float>(RAND_MAX);

    for (int i = 0; i < nBalls; i++) {
        float radius = (rand() / fRAND_MAX * (maxR - minR)) + minR;
        float mass = radius * radius * M_PI * density;

        // sf::Color color(rand()%256, rand()%256, rand()%256, rand()%256);
        sf::Color color(255, 255, 255, 255);

        float X = rand() / fRAND_MAX;
        float Y = rand() / fRAND_MAX;

        float cx = (X*(windowSize.x - 2*radius) + radius) + window.getPosition().x * pxToM;
        float cy = (Y*(windowSize.y - 2*radius) + radius) + window.getPosition().y * pxToM;
        float vx = (rand()/fRAND_MAX*2.f - 1)*maxV;
        float vy = (rand()/fRAND_MAX*2.f - 1)*maxV;
        float rest = (rand() / fRAND_MAX) * (maxRest - minRest) + minRest;
        int eltId = grid.addElement({radius, mass, rest, vx, vy, color, cx, cy});
        grid.insert(eltId);
    }
}


sf::Vector2f findAccel(const sf::Vector2f& cPos) {
    if (gravityRadial && mousePressed) {
        sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition()) * pxToM;
        float dx = cPos.x - mousePos.x;
        float dy = cPos.y - mousePos.y;
        float dSqr = dx * dx + dy * dy;
        float acc = G / (dSqr + offset);
        float d = std::sqrt(dSqr + offset);
        return sf::Vector2f(-dx*acc/d, -dy*acc/d);
    } else if (gravityRadial) {
        return sf::Vector2f(0.f, 0.f);
    }

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

void parseArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: gravity-sim [options]\n"
                      << "Options:\n"
                      << "  --nBalls <int>       Number of balls (default: 2000)\n"
                      << "  --g <float>          Gravity (default: 9.806)\n"
                      << "  --G <float>          Mouse gravity constant (default: 1.0)\n"
                      << "  --rMin <float>       Minimum radius in pixels (default: 5.0)\n"
                      << "  --rMax <float>       Maximum radius in pixels (default: 5.0)\n"
                      << "  --rFix <float>       Uniform radius in pixels\n"
                      << "  --maxV <float>       Maximum initial velocity (default: 0.0)\n"
                      << "  --density <float>    Ball density (default: 10.0)\n"
                      << "  --subSteps <int>     Physics sub-steps (default: 1)\n"
                      << "  --timeScale <float>  Time scale multiplier (default: 1.0)\n"
                      << "  --physicsDt <float>  Fixed physics delta-time (default: 1.0)\n"
                      << "  --restMin <float>    Minimum restitution (default: 0.7)\n"
                      << "  --restMax <float>    Maximum restitution (default: 0.7)\n"
                      << "  --restMax <float>    Uniform restitution\n"
                      << "  --scale <float>      Window scale (default: 0.5)\n"
                      << "  --g-radial           Gravity mode radial\n"
                      << "  --debug              Enable console debug output\n";
            exit(0);
        }
        else if (arg == "--nBalls" && i + 1 < argc) nBalls = std::stoi(argv[++i]);
        else if (arg == "--g" && i + 1 < argc) g = std::stof(argv[++i]);
        else if (arg == "--G" && i + 1 < argc) G = std::stof(argv[++i]);
        else if (arg == "--rMin" && i + 1 < argc) minR = std::stof(argv[++i]);
        else if (arg == "--rMax" && i + 1 < argc) maxR = std::stof(argv[++i]);
        else if (arg == "--rFix" && i + 1 < argc) {maxR = std::stof(argv[++i]); minR = maxR;}
        else if (arg == "--maxV" && i + 1 < argc) maxV = std::stof(argv[++i]);
        else if (arg == "--density" && i + 1 < argc) density = std::stof(argv[++i]);
        else if (arg == "--subSteps" && i + 1 < argc) subSteps = std::stoi(argv[++i]);
        else if (arg == "--timeScale" && i + 1 < argc) timeScale = std::stof(argv[++i]);
        else if (arg == "--physicsDt" && i + 1 < argc) fixedDt = std::stof(argv[++i]);
        else if (arg == "--restMin" && i + 1 < argc) minRest = std::stof(argv[++i]);
        else if (arg == "--restMax" && i + 1 < argc) maxRest = std::stof(argv[++i]);
        else if (arg == "--restFix" && i + 1 < argc) {maxRest = std::stof(argv[++i]); minRest = maxRest;}
        else if (arg == "--scale" && i + 1 < argc) windowScale = std::stof(argv[++i]);
        else if (arg == "--g-radial") gravityRadial = true;
        else if (arg == "--debug") debugTimeOutput = true;
    }
}