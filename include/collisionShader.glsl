#version 460
layout(local_size_x = 64) in;

struct Element {
    float radius, mass, rest;
    float vx, vy;
    uint color;
    float cx, cy;
    float _pad[6]; // 64-byte alignment
};

layout(std430, binding = 0) buffer Elements  { Element elts[]; };
layout(std430, binding = 1) buffer CellStart { uint cellStart[]; };
layout(std430, binding = 2) buffer Indices   { uvec2 indices[]; }; // (cellId, ballId)

uniform uint xNum;
uniform uint yNum;
uniform uint nBalls;
uniform int dCol;   //  1 = right, -1 = left
uniform int dRow;   //  1 = below, -1 = above

uniform uint phaseStart; // 0 or 1, set per dispatch
uniform int offsets[4][2];

void resolveCollision(uint i, uint j) {
    // Check the collision!
    float dx = elts[j].cx - elts[i].cx;
    float dy = elts[j].cy - elts[i].cy;

    float r1 = elts[i].radius;
    float r2 = elts[j].radius;

    float distSq = dx * dx + dy * dy;
    float radDist = r1 + r2;

    // skip to next pair if they don't collide
    if (distSq >= radDist * radDist || distSq <= 0.00001f) {
        return;
    }

    float distance = sqrt(distSq);
    float invDist = 1.0f / distance;
    
    float nx = dx * invDist;
    float ny = dy * invDist;
    
    float invM1 = 1.0f / elts[i].mass;
    float invM2 = 1.0f / elts[j].mass;
    float sumInvMass = invM1 + invM2;

    // distancing
    float overlap = radDist - distance;
    float correction = overlap / sumInvMass;
    float cx = nx * correction;
    float cy = ny * correction;
    
    elts[i].cx -= cx * invM1;
    elts[i].cy -= cy * invM1;
    elts[j].cx += cx * invM2;
    elts[j].cy += cy * invM2;

    /* // REMOVED TO TRY CLEAR
    insert(ballId1);
    insert(ballId2);
    */
    
    // speed management
    float dvx = elts[j].vx - elts[i].vx;
    float dvy = elts[j].vy - elts[i].vy;
    float dotProd = dvx * nx + dvy * ny;

    // if they are going apart, don't change their velocities
    if (dotProd > 0.f) {
        return;
    }

    float e = elts[i].rest * elts[j].rest;
    float J = -(1.f + e) * dotProd / sumInvMass;
    
    float Jnx = J * nx;
    float Jny = J * ny;
    
    elts[i].vx -= Jnx * invM1;
    elts[i].vy -= Jny * invM1;
    elts[j].vx += Jnx * invM2;
    elts[j].vy += Jny * invM2;
}

void main() {
    // Each thread gets a column spaced 2 apart — safe for concurrent execution
    uint col = gl_GlobalInvocationID.x * 2 + phaseStart;
    if (col >= xNum) return;

    for (uint row = 0; row < yNum; row++) {
        uint cellId = row * xNum + col;
        uint start = cellStart[cellId];
        if (start >= nBalls) continue;

        for (uint i = start; i < nBalls && indices[i].x == cellId; i++) {

            for (uint j = i + 1; j < nBalls && indices[j].x == cellId; j++)
                resolveCollision(indices[i].y, indices[j].y);

            for (int k = 0; k < 4; k++) {
                int nCol = int(col) + offsets[k][0];
                int nRow = int(row) + offsets[k][1];
                if (nCol < 0 || nCol >= int(xNum) || nRow < 0 || nRow >= int(yNum))
                    continue;
                uint nCellId = uint(nRow) * xNum + uint(nCol);
                uint nStart = cellStart[nCellId];
                if (nStart >= nBalls) continue;
                for (uint n = nStart; n < nBalls && indices[n].x == nCellId; n++)
                    resolveCollision(indices[i].y, indices[n].y);
            }
        }
    }
}