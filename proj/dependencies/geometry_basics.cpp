#include "geometry_basics.hpp"

#include <cassert>

#include <iostream>
using namespace std;

// If no bounding box is given, we assume (hope) that coordinates are in the range [-10e18, 10e18].
distance_t min_x = - 1e18l, min_y = - 1e18l, max_x = 1e18l, max_y = 1e18l;

interval intersection_interval(point circle_center, distance_t radius, point line_start, point line_end)
{
    // move the circle center to (0, 0) to simplify the calculation
    line_start -= circle_center;
    line_end -= circle_center;
    circle_center = {0, 0};

    // The line can be represented as line_start + lambda * v
    const vec v = line_end - line_start;

    // Find points p = line_start + lambda * v with
    //     dist(p, circle_center) = radius
    // <=> sqrt(p.x^2 + p.y^2) = radius
    // <=> p.x^2 + p.y^2 = radius^2
    // <=> (line_start.x + lambda * v.x)^2 + (line_start.y + lambda * v.y)^2 = radius^2
    // <=> (line_start.x^2 + 2 * line_start.x * lambda * v.x + lambda^2 * v.x^2) + (line_start.y^2 + 2 * line_start.y * lambda * v.y + lambda^2 * v.y^2) = radius^2
    // <=> lambda^2 * (v.x^2 + v.y^2) + lambda * (2 * line_start.x * v.x + 2 * line_start.y * v.y) + line_start.x^2 + line_start.y^2) - radius^2 = 0
    // let a := v.x^2 + v.y^2, b := 2 * line_start.x * v.x + 2 * line_start.y * v.y, c := line_start.x^2 + line_start.y^2 - radius^2
    // <=> lambda^2 * a + lambda * b + c) = 0
    // <=> lambda^2 + (b / a) * lambda + c / a) = 0
    // <=> lambda1/2 = - (b / 2a) +/- sqrt((b / 2a)^2 - c / a)

    const distance_t a = sqr(v.x) + sqr(v.y);
    const distance_t b = (line_start.x * v.x + line_start.y * v.y);
    const distance_t c = sqr(line_start.x) + sqr(line_start.y) - sqr(radius);

    const distance_t discriminant = sqr(b / a) - c / a;

    if (discriminant < 0) {
        return empty_interval; // no intersection;
    }

    const distance_t lambda1 = - b / a - sqrt(discriminant);
    const distance_t lambda2 = - b / a + sqrt(discriminant);

    if (lambda2 < 0 || lambda1 > 1) return empty_interval;
    else return {max<distance_t>(lambda1, 0), min<distance_t>(lambda2, 1)};
}
