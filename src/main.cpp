#include <iostream>
#include <cmath>
#include <array>
#include <algorithm>
#include <SFML/Graphics.hpp>  

#include "quadTree.hpp"
// eventually add GLM for vector/matrix operations

float g = 9.806e0f;
float G = 1.f;
float offset = .01f;
float timeScale = 1.e0f;
float fps = 60.f;
float fixedDt = 1.f / 10.f;

int subSteps = 4;
int treeDepth = 6;
int maxEltPerNode = 3;
bool debugTimeOutput = false;

float minRest = 0.9f;
float maxRest = 0.9f;

float windowScale = .5f;
float FULLWIDTH = sf::VideoMode::getDesktopMode().size.x;
float FULLHEIGHT = sf::VideoMode::getDesktopMode().size.y;
sf::Vector2u size(FULLWIDTH * windowScale, FULLHEIGHT * windowScale);
bool mousePressed = false;

// EVERYTHING IN METERS. FOR GRAPHICS CALCULATIONS, THE VALUES ARE CONVERTED!!!
// METRIC AND PIXEL SYSTEM START WITH 0 IN TOP LEFT OF --- MONITOR ---, NOT WINDOW

// meters to pixels conversion
float mToPx = FULLHEIGHT / 2;
float pxToM = 1 / mToPx;

// Number of balls
int nBalls = 2000;

// Radius params
float minR = 5.f;
float maxR = 5.f;

float maxV = 1.e0f;

float density = 1.e-1f;
bool gravityRadial = false;

struct Balls {
    std::vector<float> masses;
    std::vector<float> restitutions;
    std::vector<sf::Color> colors;
    std::vector<sf::Vector2f> positions;
    std::vector<sf::Vector2f> velocities;
    std::vector<sf::CircleShape> circles;
    std::vector<QuadElt> treeElts;

    void randomInit(sf::Window& window, int n, float minR, float maxR, float maxV, float density, float minRest, float maxRest) {
        masses.resize(n);
        colors.resize(n);
        positions.resize(n);
        velocities.resize(n);
        circles.resize(n);
        restitutions.resize(n);
        treeElts.resize(n);
        
        srand(time(0));
        float fRAND_MAX = static_cast<float>(RAND_MAX);
        for (int i = 0; i < n; i++) {
            float pRadius = (rand() / fRAND_MAX * (maxR - minR)) + minR;
            float radius = pxToM * pRadius;

            masses[i] = radius * radius * M_PI * density;

            // sf::Color color(rand()%256, rand()%256, rand()%256, rand()%256);
            
            sf::Color color(255, 255, 255, 255);
            colors[i] = color;

            float X = rand() / fRAND_MAX;
            float Y = rand() / fRAND_MAX;
            X = (X * (static_cast<float>(size.x) - 2 * pRadius) + window.getPosition().x + pRadius) * pxToM;
            Y = (Y * (static_cast<float>(size.y) - 2 * pRadius) + window.getPosition().y + pRadius) * pxToM;
            positions[i] = {X, Y};
            treeElts[i].cx = X;
            treeElts[i].cy = Y;
            treeElts[i] = QuadElt(radius, X, Y);
            treeElts[i].objId = i;

            velocities[i] = sf::Vector2f((rand()/fRAND_MAX*2.f - 1)*maxV, (rand()/fRAND_MAX*2.f - 1)*maxV);
            restitutions[i] = (rand() / fRAND_MAX) * (maxRest - minRest) + minRest;

            // update circles
            circles[i].setRadius(pRadius);
            circles[i].setFillColor(color);
            circles[i].setOrigin({pRadius, pRadius});
        }
    }
};

sf::Vector2f findAccel(const sf::Vector2f& cPos);
float norm(sf::Vector2f vec);
float dot(sf::Vector2f V1, sf::Vector2f V2);
bool comp(const sf::Vector2f& v1, const sf::Vector2f& v2);

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
                      << "  --subSteps <int>     Physics sub-steps (default: 4)\n"
                      << "  --timeScale <float>  Time scale multiplier (default: 1.0)\n"
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
        else if (arg == "--restMin" && i + 1 < argc) minRest = std::stof(argv[++i]);
        else if (arg == "--restMax" && i + 1 < argc) maxRest = std::stof(argv[++i]);
        else if (arg == "--restFix" && i + 1 < argc) {maxRest = std::stof(argv[++i]); minRest = maxRest;}
        else if (arg == "--scale" && i + 1 < argc) windowScale = std::stof(argv[++i]);
        else if (arg == "--g-radial") gravityRadial = true;
        else if (arg == "--debug") debugTimeOutput = true;
    }
}

int main(int argc, char* argv[]){
    parseArguments(argc, argv);

    sf::RenderWindow window(sf::VideoMode({size.x, size.y}), "Bawls");
    sf::View fixedView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(size.x), static_cast<float>(size.y)}));
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

    // Initialize Quadtree
    Quadtree ballTree(FULLWIDTH * pxToM, FULLHEIGHT * pxToM, treeDepth, maxEltPerNode);

    for (int i = 0; i < nBalls; i++) {
        ballTree.insert(balls.treeElts[i]);
    }
    

    float accumulator = 0.f;
    while (window.isOpen())
    {
        // Process events
        while (const std::optional<sf::Event> event = window.pollEvent()) // window event logic
        {
            // Close window: exit
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } 
            else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                size = resized->size;
                window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(size.x), static_cast<float>(size.y)})));
            } 
            else if (const auto* mousePressedEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressedEvent->button == sf::Mouse::Button::Left) {
                    mousePressed = !mousePressed;
                }
            }
        }

        sf::Vector2i windowPos = window.getPosition();
        float frameTime = clock.restart().asSeconds() * timeScale;

        accumulator += frameTime;
        
        while (accumulator >= fixedDt) {
            physicsTicks++;
            // Physics and tree update
            for (int i = 0; i < nBalls; i++) {
                // remove from tree to update (reinsert at the end of the iteration)
                ballTree.remove(balls.treeElts[i]);

                // Update position and velocity
                sf::Vector2f pos = balls.positions[i];
                sf::Vector2f vel = balls.velocities[i];
                
                sf::Vector2f acc = findAccel(pos);
                sf::Vector2f vHalf = vel + 0.5f * acc * fixedDt;
                pos += vHalf * fixedDt;
                acc = findAccel(pos);
                vel = vHalf + 0.5f * acc * fixedDt;

                // border velocity reflection logic
                float radius = balls.treeElts[i].radius;
                float pRadius = radius * mToPx;
                sf::Vector2f pxPos = pos * mToPx;
                float rest = balls.restitutions[i];

                if ((pxPos.x < pRadius + windowPos.x && vel.x < 0) || (pxPos.x > size.x - pRadius + windowPos.x && vel.x > 0)) {
                    vel.x = -vel.x * rest;
                }
                if ((pxPos.y < pRadius + windowPos.y && vel.y < 0) || (pxPos.y > size.y - pRadius + windowPos.y && vel.y > 0)) {
                    vel.y = -vel.y * rest;
                }

                // updating physics coordinates
                balls.positions[i] = pos;
                balls.velocities[i] = vel;

                // bounding box and ball element update
                balls.treeElts[i].cx = pos.x;
                balls.treeElts[i].cy = pos.y;
                balls.treeElts[i].minX = pos.x - radius;
                balls.treeElts[i].maxX = pos.x + radius;
                balls.treeElts[i].minY = pos.y - radius;
                balls.treeElts[i].maxY = pos.y + radius;

                // re-insertion in tree
                ballTree.insert(balls.treeElts[i]);
            }

            // broad collisions from tree
            std::vector<Quadtree::CollisionData> pairs = ballTree.getCollisions();

            // fine collisions with substepping
            for (int step = 0; step < subSteps; step++) {
                // ball collisions
                for (const auto& pair: pairs) {
                    // radiuses
                    const float r1 = balls.treeElts[pair.id1].radius;
                    const float r2 = balls.treeElts[pair.id2].radius;
                    const float radDist = r1 + r2;

                    // masses
                    const float invM1 = 1.0f / balls.masses[pair.id1];
                    const float invM2 = 1.0f / balls.masses[pair.id2];

                    // positions
                    sf::Vector2f& pos1 = balls.positions[pair.id1];
                    sf::Vector2f& pos2 = balls.positions[pair.id2];
                    sf::Vector2f& vel1 = balls.velocities[pair.id1];
                    sf::Vector2f& vel2 = balls.velocities[pair.id2];

                    sf::Vector2f dPos = pos2 - pos1;
                    float distSq = dPos.lengthSquared();

                    if (distSq < radDist * radDist && distSq > 0.00001f) {
                        float distance = std::sqrt(distSq);
                        float invDist = 1.0f / distance;
                        
                        sf::Vector2f n = dPos * invDist;
                        
                        float invSumInvMass = 1 / (invM1 + invM2);

                        // distancing
                        float overlap = radDist - distance;
                        float correction = overlap * invSumInvMass;
                        sf::Vector2f correctionVect = n * correction;
                        
                        pos1 -= correctionVect * invM1;
                        pos2 += correctionVect * invM2;
                        
                        // collision management
                        sf::Vector2f dVel = vel2 - vel1;
                        float dotProd = dot(dVel, n);

                        if (dotProd < 0.f) { 
                            float e = balls.restitutions[pair.id1] * balls.restitutions[pair.id2];
                            float J = -(1.f + e) * dotProd * invSumInvMass;
                            
                            sf::Vector2f Jn = J * n;
                            
                            vel1 -= Jn * invM1;
                            vel2 += Jn * invM2;
                        }
                    }
                }

                // border collisions
                for (int i = 0; i < nBalls; i++) {
                    sf::Vector2f& pos = balls.positions[i];
                    sf::Vector2f& vel = balls.velocities[i];
                    float radius = balls.treeElts[i].radius;
                    float pRadius = radius * mToPx;
                    float rest = balls.restitutions[i];
                    
                    // getting the positions (IN METERS converted to PIXELS)
                    sf::Vector2f pxPos = pos * mToPx;
                    
                    bool posChanged = false;
                    if (pxPos.x < pRadius + windowPos.x) {
                        pxPos.x = pRadius + windowPos.x;
                        posChanged = true;
                    } else if (pxPos.x > size.x - pRadius + windowPos.x) {
                        pxPos.x = size.x - pRadius + windowPos.x;
                        posChanged = true;
                    }
                    if (pxPos.y < pRadius + windowPos.y)  {
                        pxPos.y = pRadius + windowPos.y;
                        posChanged = true;
                    } else if (pxPos.y > size.y - pRadius + windowPos.y) {
                        pxPos.y = size.y - pRadius + windowPos.y;
                        posChanged = true;
                    }
                    
                    // setting the positions IN METERS
                    if (posChanged) {
                        balls.positions[i] = pxPos * pxToM;
                    }
                }
            }

            // CLEAN THE EMTPY LEAFS AND BRANCHES!!!
            ballTree.cleanup();

            accumulator -= fixedDt;
        }


        // graphics rendering
        if (renderClock.getElapsedTime().asSeconds() >= renderDt) {
            renderFrames++;

            window.clear();

            for (int i = 0; i < nBalls; i++) {
                sf::Vector2f pxPos = balls.positions[i] * mToPx; 

                balls.circles[i].setPosition(pxPos - static_cast<sf::Vector2f>(windowPos));
                
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
    return 0;
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