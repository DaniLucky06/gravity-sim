#version 460

layout(local_size_x = 256) in;
struct Element {
    float radius, mass, rest;
    float vx, vy;
    uint color;
    float cx, cy;
    float _pad[6]; // 64-byte alignment
};

layout(std430, binding = 0) buffer Elements {
    Element elts[]; 
};

uniform float fixedDt;
uniform float g;
uniform vec2 windowSizePx;
uniform vec2 windowPosPx;
uniform float mToPx;
uniform float pxToM;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= elts.length()) return;

    Element e = elts[i];

    // Velocity Verlet
    vec2 pos = vec2(e.cx, e.cy);
    vec2 vel = vec2(e.vx, e.vy);
    vec2 acc = vec2(0.0, g);

    vec2 vHalf = vel + 0.5 * acc * fixedDt;
    pos += vHalf * fixedDt;
    vel = vHalf + 0.5 * acc * fixedDt;

    // Border collisions
    float radiusPx = e.radius * mToPx;
    vec2 posPx = pos * mToPx;

    if (posPx.x < radiusPx + windowPosPx.x) {
        if (vel.x < 0.0) vel.x = -vel.x;
        vel.x *= e.rest;
        posPx.x = radiusPx + windowPosPx.x;
    } else if (posPx.x > windowSizePx.x - radiusPx + windowPosPx.x) {
        if (vel.x > 0.0) vel.x = -vel.x;
        vel.x *= e.rest;
        posPx.x = windowSizePx.x - radiusPx + windowPosPx.x;
    }
    if (posPx.y < radiusPx + windowPosPx.y) {
        if (vel.y < 0.0) vel.y = -vel.y;
        vel.y *= e.rest;
        posPx.y = radiusPx + windowPosPx.y;
    } else if (posPx.y > windowSizePx.y - radiusPx + windowPosPx.y) {
        if (vel.y > 0.0) vel.y = -vel.y;
        vel.y *= e.rest;
        posPx.y = windowSizePx.y - radiusPx + windowPosPx.y;
    }

    elts[i].cx = posPx.x * pxToM;
    elts[i].cy = posPx.y * pxToM;
    elts[i].vx = vel.x;
    elts[i].vy = vel.y;
}