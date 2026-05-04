#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <SFML/Graphics.hpp>  

#include "grid.hpp"
#include "threadManager.hpp"
// eventually add GLM for vector/matrix operations

float g = 9.806e0f;
float G = 1.f;
float offset = .01f;
float timeScale = 1.e0f;
float fps = 60.f;
float ms = 1.f / fps;
float fixedDt = 0.0005f;

int hwConc = std::thread::hardware_concurrency();
uint32_t numThreads = (hwConc == 0) ? 4 : hwConc;
bool debugTimeOutput = false;
float renderTick = 1.f;

float minRest = 0.9f;
float maxRest = 0.9f;

float windowScale = .75f;

unsigned int FULLWIDTHPX, FULLHEIGHTPX;
sf::Vector2u windowSize;
float mToPx, pxToM, FULLWIDTH, FULLHEIGHT;
float mouseInfluenceSqr, mouseInfluence;

Grid grid;
sf::RenderWindow window;

bool mousePressed = false;

// EVERYTHING IN METERS. FOR GRAPHICS CALCULATIONS, THE VALUES ARE CONVERTED!!!
// METRIC AND PIXEL SYSTEM START WITH 0 IN TOP LEFT OF --- MONITOR ---, NOT WINDOW

// Number of balls
int nBalls = 500;

// Radius params
float minR = 5.f;
float maxR = 5.f;

float gridMult = 1.f;

float maxVinit = 1.e0f;

float density = 1.e-1f;

bool gravityRadial = false;
bool sortDuplication = false;

sf::Vector2f mousePos;

std::vector<uint32_t> perfStorage;
std::vector<uint32_t> physStorage;

void elementInit();
sf::Vector2f findAccel(const sf::Vector2f& cPos);
inline void collisionCheck(Element& elt1, Element& elt2);


void physicsTask(uint32_t startIdx, uint32_t endIdx, sf::Vector2i windowPos);
void collisionTask(uint32_t col, const int (*offsets)[2]);

inline float norm(sf::Vector2f vec);
inline float dot(sf::Vector2f v1, sf::Vector2f v2);
inline float mean(std::vector<uint32_t>& arr);

void parseArguments(int argc, char* argv[]);

float vMinSqr = 2.f;
float vMaxSqr = 2.f;
float invVMaxSqr = 1.f / vMaxSqr;

uint32_t maxPhysThreads;

bool render;
int main(int argc, char* argv[]) {
    // 1. SFML desktop query — safe now, before any threads exist
    FULLWIDTHPX  = sf::VideoMode::getDesktopMode().size.x;
    FULLHEIGHTPX = sf::VideoMode::getDesktopMode().size.y;
    windowSize   = sf::Vector2u(FULLWIDTHPX * windowScale, FULLHEIGHTPX * windowScale);

    mToPx    = FULLHEIGHTPX / 2.f;
    pxToM    = 1.f / mToPx;
    FULLWIDTH  = FULLWIDTHPX  * pxToM;
    FULLHEIGHT = FULLHEIGHTPX * pxToM;
    mouseInfluence    = 100.f * pxToM;
    mouseInfluenceSqr = mouseInfluence * mouseInfluence;

    parseArguments(argc, argv);
    // 2. Recompute grid dims now that maxR is known from args
    const int xNum = std::ceil(FULLWIDTHPX / (gridMult * 2 * maxR));
    const int yNum = std::ceil(FULLHEIGHTPX / (gridMult * 2 * maxR));

    // 3. Construct SFML window — XInitThreads() called here, safely
    window = sf::RenderWindow(sf::VideoMode({windowSize.x, windowSize.y}), "Bawls");

    // 4. Construct grid
    grid = Grid(FULLWIDTH, FULLHEIGHT, xNum, yNum, nBalls);

    maxPhysThreads = std::max(1u, std::min(numThreads, static_cast<uint32_t>(nBalls)));;

    // 5. NOW it's safe to spawn threads
    ThreadPool threadpool(numThreads);
    elementInit();


    sf::View fixedView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)}));
    window.setView(fixedView);

    sf::CircleShape mouseCircle(mouseInfluence * mToPx, 30);
    mouseCircle.setOutlineColor(sf::Color::Red);
    mouseCircle.setOutlineThickness(-3);
    mouseCircle.setFillColor(sf::Color::Transparent);
    mouseCircle.setOrigin({mouseInfluence * mToPx, mouseInfluence * mToPx});
    
    // time
    sf::Clock clock;
    sf::Clock renderClock;
    
    
    sf::Clock metricsClock;
    int physicsTicks = 0;
    int renderFrames = 0;
    
    std::vector<uint64_t> collisionPairs;
    
    // Build ONCE, outside all loops
    const int SEGS = 24;
    sf::VertexArray va(sf::PrimitiveType::Triangles, nBalls * SEGS * 3);
    
    // Pre-compute angles once (avoids recomputing sin/cos every frame)
    std::vector<float> cosA(SEGS), sinA(SEGS);
    for (int s = 0; s < SEGS; s++) {
        float a = s * 2.f * M_PI / SEGS;
        cosA[s] = std::cos(a);
        sinA[s] = std::sin(a);
    }
    // 4 sweep configs, rotated each tick
    constexpr int sweepOffsets[4][4][2] = {
        {{ 1, 0}, { 1,-1}, { 1, 1}, { 0, 1}},  // right + below
        {{ 1, 0}, { 1,-1}, { 1, 1}, { 0,-1}},  // right + above
        {{-1, 0}, {-1,-1}, {-1, 1}, { 0, 1}},  // left  + below
        {{-1, 0}, {-1,-1}, {-1, 1}, { 0,-1}},  // left  + above
    };
    // configs 0,1 are right-sweeps: even cols first
    // configs 2,3 are left-sweeps:  odd cols first
    constexpr uint32_t sweepPhase0Start[4] = {0, 0, 1, 1};

    static uint32_t sweepCounter = 0;
    
    while (window.isOpen())
    {
        // Process events
        while (const std::optional<sf::Event> event = window.pollEvent()) // closing window logic
        {
            // Close window: exit
            if (event->is<sf::Event::Closed>()) {
                float meanPhysTicks = mean(physStorage);
                if (debugTimeOutput) printf("\n ----------------------------------------- \n Mean Physics Ticks: %.0f/s | Time scale: %.4f\n", meanPhysTicks, (meanPhysTicks * fixedDt));
                window.close();
                return 0;
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
        
        render = renderClock.getElapsedTime().asSeconds() >= ms;
        if (render) {
            window.clear();
            if (mousePressed) mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition()) * pxToM;
        }
        
        
        // physics
        uint32_t ballsPerThread = (nBalls + maxPhysThreads - 1) / maxPhysThreads;
        for (uint32_t t = 0; t < maxPhysThreads; t++) {
            uint32_t startIdx = t * ballsPerThread;
            uint32_t endIdx = std::min(static_cast<uint32_t>(nBalls), startIdx + ballsPerThread);
            threadpool.enqueue(physicsTask, startIdx, endIdx, windowPos);
        }
        threadpool.waitFinished();
        
        // fill the grid
        grid.build();
                
        vMaxSqr = 0.f;
        for (size_t i = 0; i < nBalls; i++) {
            Element& el = grid.elements[i];
            float vSqr = norm({el.vx, el.vy});
            if (vSqr > vMaxSqr) vMaxSqr = vSqr;

            if (render) {
                float r = el.radius * mToPx / std::cos(M_PI / SEGS);
                float px = el.cx * mToPx - windowPos.x;
                float py = el.cy * mToPx - windowPos.y;

                // Sets the color based on velocity: faster balls are redder, slower balls are bluer
                float vNorm = vSqr * invVMaxSqr;
                float vNormMin = std::min(vNorm, 1.f);

                if (vSqr <= 0.12f) vNormMin = 0.f; // deadzone for very slow balls, to avoid distracting color flickering.
                
                uint8_t red  = static_cast<uint8_t>(vNormMin * 0xFF);
                uint8_t blue = static_cast<uint8_t>((1.f - vNormMin) * 0xFF);
                sf::Color col(red, 0, blue);

                for (int s = 0; s < SEGS; s++) {
                    int base = i * SEGS * 3 + s * 3;
                    va[base + 0] = {{px, py}, col};
                    va[base + 1] = {{px + r * cosA[s],            py + r * sinA[s]},            col};
                    va[base + 2] = {{px + r * cosA[(s+1) % SEGS], py + r * sinA[(s+1) % SEGS]}, col};
                }
            }
        }

        if (render) {
            window.draw(va);
        }
        
        physicsTicks++;
        
        if (render) {
            invVMaxSqr = (vMaxSqr > 0.00001f) ? 1 / vMaxSqr : 1.f;
            renderFrames++;
            if (mousePressed) {mouseCircle.setPosition(mousePos * mToPx - static_cast<sf::Vector2f>(windowPos)); window.draw(mouseCircle);}
            
            window.display();
            renderClock.restart();
        }
        
        // ball collisions — two row phases
        const int (*offsets)[2] = sweepOffsets[sweepCounter % 4];
        uint32_t p0 = sweepPhase0Start[sweepCounter % 4];
        sweepCounter++;

        for (uint8_t j = 0; j < 2; j++) {
            uint32_t startCol = (j == 0) ? p0 : 1 - p0;
            std::atomic<uint32_t> nextCol{startCol};
            for (uint32_t t = 0; t < numThreads; t++) {
                threadpool.enqueue([&nextCol, offsets]() {
                    uint32_t col;
                    while ((col = nextCol.fetch_add(2)) < grid.xNum)
                        collisionTask(col, offsets);
                });
            }
            threadpool.waitFinished();
        }
        
        // debug output
        if (metricsClock.getElapsedTime().asSeconds() >= renderTick && debugTimeOutput) {
            std::cout << "FPS: " << renderFrames
            << " | Physics Ticks (UPS): " << physicsTicks / renderTick
                      << "/s | Rapporto: " << (float)physicsTicks / renderFrames
                      << " calcoli per frame | Time scale: " << physicsTicks * fixedDt / renderTick << "\n";

                      physStorage.push_back(physicsTicks / renderTick);
                      perfStorage.push_back((float)physicsTicks / renderFrames);

                      
                      // Resetta i contatori per il secondo successivo
                      physicsTicks = 0;
            renderFrames = 0;
            metricsClock.restart();
        }
    }
}

void physicsTask(uint32_t startIdx, uint32_t endIdx, sf::Vector2i windowPos) 
{
    for (size_t i = startIdx; i < endIdx; i++) {
        // grid.remove(i); REMOVED TO TRY GRID CLEAR
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
            if (vel.x < 0) vel.x = -vel.x;
            vel.x *= rest;

            px = radius + windowPos.x;
        } else if (px > windowSize.x - radius + windowPos.x) {
            if (vel.x > 0) vel.x = -vel.x;
            vel.x *= rest;

            px = windowSize.x - radius + windowPos.x;
        }
        if (py < radius + windowPos.y)  {
            if (vel.y < 0) vel.y = -vel.y;
            vel.y *= rest;

            py = radius + windowPos.y;
        } else if (py > windowSize.y - radius + windowPos.y) {
            if (vel.y > 0) vel.y = -vel.y;
            vel.y *= rest;

            py = windowSize.y - radius + windowPos.y;
        }
        
        pos.x = px * pxToM;
        pos.y = py * pxToM;
        
        grid.elements[i].cx = pos.x;
        grid.elements[i].cy = pos.y;
        grid.elements[i].vx = vel.x;
        grid.elements[i].vy = vel.y;
    }
}

void collisionTask(uint32_t col, const int (*offsets)[2])
{
    for (uint32_t row = 0; row < grid.yNum; row++) {
        uint32_t cellId = row * grid.xNum + col;
        uint32_t startIdx = grid.cellStart[cellId];
        if (startIdx >= nBalls) continue;

        for (uint32_t idx1 = startIdx; idx1 < nBalls && grid.indices[idx1].cellId == cellId; idx1++) {
            Element& elt1 = grid.elements[grid.indices[idx1].ballId];

            for (uint32_t idx2 = idx1 + 1; idx2 < nBalls && grid.indices[idx2].cellId == cellId; idx2++)
                collisionCheck(elt1, grid.elements[grid.indices[idx2].ballId]);

            for (int k = 0; k < 4; k++) {
                int nCol = static_cast<int>(col) + offsets[k][0];
                int nRow = static_cast<int>(row) + offsets[k][1];
                if (nCol < 0 || nCol >= static_cast<int>(grid.xNum) || nRow < 0 || nRow >= static_cast<int>(grid.yNum))
                    continue;
                uint32_t nCellId = nRow * grid.xNum + nCol;
                uint32_t nStartIdx = grid.cellStart[nCellId];
                if (nStartIdx >= nBalls) continue;
                for (uint32_t nIdx = nStartIdx; nIdx < nBalls && grid.indices[nIdx].cellId == nCellId; nIdx++)
                    collisionCheck(elt1, grid.elements[grid.indices[nIdx].ballId]);
            }
        }
    }
}


void elementInit() {
    srand(time(0));
    float fRAND_MAX = static_cast<float>(RAND_MAX);

    for (int i = 0; i < nBalls; i++) {
        float radiusPx = (rand() / fRAND_MAX * (maxR - minR)) + minR; 
        float radius = radiusPx * pxToM;
        float mass = radius * radius * M_PI * density;
        uint32_t color = 0xFFFFFFFF;

        float X = rand() / fRAND_MAX;
        float Y = rand() / fRAND_MAX;

        float cx = ((X*(windowSize.x - 2*radiusPx) + radiusPx) + window.getPosition().x) * pxToM;
        float cy = ((Y*(windowSize.y - 2*radiusPx) + radiusPx) + window.getPosition().y) * pxToM;
        
        float vx = (rand()/fRAND_MAX*2.f - 1)*maxVinit;
        float vy = (rand()/fRAND_MAX*2.f - 1)*maxVinit;
        float rest = (rand() / fRAND_MAX) * (maxRest - minRest) + minRest;
        
        // Directly overwrite the pre-allocated struct
        grid.elements[i] = {radius, mass, rest, vx, vy, color, cx, cy};
    }
}

sf::Vector2f findAccel(const sf::Vector2f& cPos) {
    if (gravityRadial && mousePressed) {
        float dx = cPos.x - mousePos.x;
        float dy = cPos.y - mousePos.y;
        float dSqr = dx * dx + dy * dy;

        if (dSqr > mouseInfluenceSqr) return sf::Vector2f(0.f, g);
        
        float acc = G / (dSqr + offset);

        sf::Vector2f n = {dx, dy};
        n /= std::sqrt(dSqr);

        return (-n) * acc;
    } else if (gravityRadial) {
        return sf::Vector2f(0.f, g);
    }

    return sf::Vector2f(0.f, g);
}

inline void collisionCheck(Element& elt1, Element& elt2) {
    // Check the collision!

    float dx = elt2.cx - elt1.cx;
    float dy = elt2.cy - elt1.cy;

    float r1 = elt1.radius;
    float r2 = elt2.radius;

    float distSq = dx * dx + dy * dy;
    float radDist = r1 + r2;

    // skip to next pair if they don't collide
    if (distSq >= radDist * radDist || distSq <= 0.00001f) {
        return;
    }

    /* // REMOVED TO TRY GRID CLEAR
    grid.remove(ballId1);
    grid.remove(ballId2);
    */

    float distance = std::sqrt(distSq);
    float invDist = 1.0f / distance;
    
    float nx = dx * invDist;
    float ny = dy * invDist;
    
    float invM1 = 1.0f / elt1.mass;
    float invM2 = 1.0f / elt2.mass;
    float sumInvMass = invM1 + invM2;

    // distancing
    float overlap = radDist - distance;
    float correction = overlap / sumInvMass;
    float cx = nx * correction;
    float cy = ny * correction;
    
    elt1.cx -= cx * invM1;
    elt1.cy -= cy * invM1;
    elt2.cx += cx * invM2;
    elt2.cy += cy * invM2;

    /* // REMOVED TO TRY GRID CLEAR
    grid.insert(ballId1);
    grid.insert(ballId2);
    */
    
    // speed management
    float dvx = elt2.vx - elt1.vx;
    float dvy = elt2.vy - elt1.vy;
    float dotProd = dvx * nx + dvy * ny;

    // if they are going apart, don't change their velocities
    if (dotProd > 0.f) {
        return;
    }

    float e = elt1.rest * elt2.rest;
    float J = -(1.f + e) * dotProd / sumInvMass;
    
    float Jnx = J * nx;
    float Jny = J * ny;
    
    elt1.vx -= Jnx * invM1;
    elt1.vy -= Jny * invM1;
    elt2.vx += Jnx * invM2;
    elt2.vy += Jny * invM2;
}

inline float norm(sf::Vector2f vec) {
    return vec.length();
}

inline float dot(sf::Vector2f V1, sf::Vector2f V2) {
    return V1.dot(V2);
}

inline float mean(std::vector<uint32_t>& arr) {
    if (arr.size() == 0) return 0.f;
    float sum = 0.f;
    for (uint32_t el : arr) sum += static_cast<float>(el);
    return sum / arr.size();
}

void parseArguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: gravity-sim [options]\n"
                      << "Options:\n"
                      << "  --nBalls <int>        Number of balls (default: 2000)\n"
                      << "  --g <float>           Gravity (default: 9.806)\n"
                      << "  --G <float>           Mouse gravity constant (default: 1.0)\n"
                      << "  --rMin <float>        Minimum radius in pixels (default: 5.0)\n"
                      << "  --rMax <float>        Maximum radius in pixels (default: 5.0)\n"
                      << "  --rFix <float>        Uniform radius in pixels\n"
                      << "  --maxV <float>        Maximum initial velocity (default: 0.0)\n"
                      << "  --density <float>     Ball density (default: 10.0)\n"
                      << "  --timeScale <float>   Time scale multiplier (default: 1.0)\n"
                      << "  --physicsDt <float>   Fixed physics delta-time (default: 0.0005)\n"
                      << "  --num-threads <float> Number of threads in the thread pool (default: system managed)\n"
                      << "  --fps <float>         Fps (default: 60.0)\n"
                      << "  --restMin <float>     Minimum restitution (default: 0.7)\n"
                      << "  --restMax <float>     Maximum restitution (default: 0.7)\n"
                      << "  --restFix <float>     Uniform restitution\n"
                      << "  --scale <float>       Window scale (default: 0.5)\n"
                      << "  --mouseSize <float>   Maximum mouse pull-in radius in pixels (default: 100)\n"
                      << "  --g-radial            Gravity mode radial\n"
                      << "  --sort                Sort the collisionPairs array for de-duplication\n"
                      << "  --debug               Enable console debug output\n";
            exit(0);
        }
        else if (arg == "--nBalls" && i + 1 < argc) nBalls = std::stoi(argv[++i]);
        else if (arg == "--g" && i + 1 < argc) g = std::stof(argv[++i]);
        else if (arg == "--G" && i + 1 < argc) G = std::stof(argv[++i]);
        else if (arg == "--rMin" && i + 1 < argc) minR = std::stof(argv[++i]);
        else if (arg == "--rMax" && i + 1 < argc) maxR = std::stof(argv[++i]);
        else if (arg == "--rFix" && i + 1 < argc) {maxR = std::stof(argv[++i]); minR = maxR;}
        else if (arg == "--maxV" && i + 1 < argc) maxVinit = std::stof(argv[++i]);
        else if (arg == "--density" && i + 1 < argc) density = std::stof(argv[++i]);
        else if (arg == "--timeScale" && i + 1 < argc) timeScale = std::stof(argv[++i]);
        else if (arg == "--physicsDt" && i + 1 < argc) fixedDt = std::stof(argv[++i]);
        else if (arg == "--num-threads" && i + 1 < argc) numThreads = std::stof(argv[++i]);
        else if (arg == "--grid-mult" && i + 1 < argc) gridMult = std::stof(argv[++i]);
        else if (arg == "--fps" && i + 1 < argc) {fps = std::stof(argv[++i]); ms = 1.f / fps;}
        else if (arg == "--restMin" && i + 1 < argc) minRest = std::stof(argv[++i]);
        else if (arg == "--restMax" && i + 1 < argc) maxRest = std::stof(argv[++i]);
        else if (arg == "--restFix" && i + 1 < argc) {maxRest = std::stof(argv[++i]); minRest = maxRest;}
        else if (arg == "--scale" && i + 1 < argc) windowScale = std::stof(argv[++i]);
        else if (arg == "--mouseSize" && i + 1 < argc) {mouseInfluence = (std::stof(argv[++i]) * pxToM);
                                                        mouseInfluenceSqr = mouseInfluence * mouseInfluence;}
        else if (arg == "--g-radial") gravityRadial = true;
        else if (arg == "--sort") sortDuplication = true;
        else if (arg == "--debug") debugTimeOutput = true;
    }
}
