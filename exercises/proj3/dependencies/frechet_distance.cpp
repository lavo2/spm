#include "frechet_distance.hpp"

#include <algorithm>
#include <limits>
#include <vector>

#include <cassert>
#include <cstddef>

using std::vector;
using std::numeric_limits;
using std::max;
using std::min;

distance_t get_frechet_distance(const curve& a, const curve& b)
{
    distance_t min_coordinate = a[0].x, max_coordinate = a[0].x;
    for (const curve& c: {a, b}) {
        for (const point& p: c) {
            min_coordinate = min(min_coordinate, min(p.x, p.y));
            max_coordinate = max(max_coordinate, max(p.x, p.y));
        }
    }

    distance_t min_d = 0, max_d = (max_coordinate - min_coordinate) * 3 + 1;
    int cnt = 0;
    while (min_d + epsilon < max_d) {
        ++cnt;
        distance_t m = (min_d + max_d) / 2;
        if (is_frechet_distance_at_most(a, b, m)) max_d = m;
        else min_d = m;
    }

    return min_d;
}

distance_t get_frechet_distance_upper_bound(const curve& a, const curve& b)
{
    distance_t distance = dist(a.back(), b.back());
    size_t pos_a = 0, pos_b = 0;

    while (pos_a + pos_b < a.size() + b.size() - 2) {
        distance = max(distance, dist(a[pos_a], b[pos_b]));
        if (pos_a == a.size() - 1) ++pos_b;
        else if (pos_b == b.size() - 1) ++pos_a;
        else {
            distance_t dist_a = dist_sqr(a[pos_a + 1], b[pos_b]);
            distance_t dist_b = dist_sqr(a[pos_a], b[pos_b + 1]);
            distance_t dist_both = dist_sqr(a[pos_a + 1], b[pos_b + 1]);
            if (dist_a < dist_b && dist_a < dist_both) {
                ++pos_a;
            } else if (dist_b < dist_both) {
                ++pos_b;
            } else {
                ++pos_a;
                ++pos_b;
            }
        }
    }

    return distance;
}

size_t nextclosepoint(const curve& c, size_t i, point p, distance_t d)
{
	size_t delta = 1;
	size_t k = i;
	while (true) {
		if (k == c.size()-1) {
			if (dist(c[k],p) <= d) { return k; }
			else { return c.size(); }
		}
		else {
			delta = std::min(delta, c.size() - 1 - k);
			if (dist(p, c[k]) - c.curve_length(k,k+delta) > d) {
				k += delta;
				delta *= 2;
			}
			else if (delta > 1) {delta /= 2; }
			else { return k; }
		}
	}
}

bool negfilter(const curve& c1, const curve& c2, distance_t d)
{
	for (size_t delta = std::max(c1.size(),c2.size())-1; delta >= 1; delta /= 2) {
		size_t i = 0;
		for (size_t j = 0; j < c2.size(); j += delta) {
			i = nextclosepoint(c1, i, c2[j], d);
			if (i >= c1.size()) {
				return true;
			}
		}
		size_t j = 0;
		for (size_t i = 0; i < c1.size(); i += delta) {
			j = nextclosepoint(c2, j, c1[i], d);
			if (j >= c2.size()) {
				return true;
			}
		}
	}
	return false;
}









