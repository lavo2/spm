#include "frechet_distance.hpp"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <vector>

#include <cassert>
#include <cstddef>

using std::vector;
using std::numeric_limits;
using std::max;
using std::min;

namespace {

distance_t eps = 1e-10;

// describes reachable intervals in the free space diagram as described in http://www.cs.uu.nl/people/marc/asci/ag-cfdbt-95.pdf
// You can think of reachable_a as the intervals in the horizontal direction and reachable_b as intervals in the vertical direction
// reachable_a[i][j] describes what section of the line segment from b[j] to b[j + 1] is close enough to the point a[i]
inline interval get_reachable_a(size_t i, size_t j, const curve& a, const curve& b, distance_t d)
{
    distance_t start, end;
    std::tie(start, end) = intersection_interval(a[i], d, b[j], b[j + 1]);
    return {start + j, end + j};
}


inline interval get_reachable_b(size_t i, size_t j, const curve& a, const curve& b, distance_t d)
{
    return get_reachable_a(j, i, b, a, d);
}

void merge (vector<interval>& v, interval i)
{
    if (is_empty_interval(i)) return;
    if (v.size() && i.first - eps <= v.back().second) v.back().second = i.second;
    else v.push_back(i);
}

void get_reachable_intervals(size_t i_min, size_t i_max, size_t j_min, size_t j_max, const curve& a, const curve& b, distance_t d, const vector<interval>& rb, const vector<interval>& ra, vector<interval>& rb_out, vector<interval>& ra_out)
{
	interval tb = empty_interval;
    auto it = std::upper_bound(rb.begin(), rb.end(), interval{j_max, numeric_limits<distance_t>::lowest()});
    if (it != rb.begin()) {
        --it;
        if (it->first <= j_max && it->second >= j_min) {
            tb = *it;
        }
    }

    interval ta = empty_interval;
    it = std::upper_bound(ra.begin(), ra.end(), interval{i_max, numeric_limits<distance_t>::lowest()});
    if (it != ra.begin()) {
        --it;
        if (it->first <= i_max && it->second >= i_min) {
            ta = *it;
        }
    }

    if (is_empty_interval(tb) && is_empty_interval(ta)) return;

    if (tb.first <= j_min + eps && tb.second >= j_max - eps && ta.first <= i_min + eps && ta.second >= i_max - eps) {
        size_t i_mid = (i_min + 1 + i_max)/2;
		size_t j_mid = (j_min + 1 + j_max)/2;
		if (dist(a[i_mid], b[j_mid]) + std::max(a.curve_length(i_min+1, i_mid),a.curve_length(i_mid, i_max)) + std::max(b.curve_length(j_min+1, j_mid),b.curve_length(j_mid, j_max)) <= d) {
            merge(rb_out, {j_min, j_max});
            merge(ra_out, {i_min, i_max});
            return;
        }
    }

    if (i_min == i_max - 1 && j_min == j_max - 1) {
        interval aa = get_reachable_a(i_max, j_min, a, b, d);
        interval bb = get_reachable_b(i_min, j_max, a, b, d);

        if (is_empty_interval(ta)) {
            aa.first = max(aa.first, tb.first);
        }
		else if (is_empty_interval(tb)) { bb.first = max(bb.first, ta.first); }

        merge(rb_out, aa);
        merge(ra_out, bb);

    } else {
        if (j_max - j_min > i_max - i_min) {
        	vector<interval> ra_middle;
        	size_t split_position = (j_max + j_min) / 2;
        	get_reachable_intervals(i_min, i_max, j_min, split_position, a, b, d, rb, ra, rb_out, ra_middle);
        	get_reachable_intervals(i_min, i_max, split_position, j_max, a, b, d, rb, ra_middle, rb_out, ra_out);
        } else {
        	vector<interval> rb_middle;
        	size_t split_position = (i_max + i_min) / 2;
        	get_reachable_intervals(i_min, split_position, j_min, j_max, a, b, d, rb, ra, rb_middle, ra_out);
        	get_reachable_intervals(split_position, i_max, j_min, j_max, a, b, d, rb_middle, ra, rb_out, ra_out);
		}
    }
}

distance_t get_last_reachable_point_from_start(const curve& a, const curve& b, const distance_t d)
{
    size_t j = 0;
    while (j < b.size() - 2 && dist_sqr(a.front(), b[j + 1]) <= sqr(d)) ++j;
    distance_t result;
    tie(std::ignore, result) = get_reachable_a(0, j, a, b, d);
    return result;
}

distance_t get_dist_to_point_sqr(const curve& a, point b)
{
    distance_t result = 0;
    for (point p: a) result = max(result, dist_sqr(p, b));
    return result;
}

} // namespace

bool is_frechet_distance_at_most(const curve& a, const curve& b, distance_t d)
{
    assert(a.size());
    assert(b.size());
    if (dist(a.front(), b.front()) > d || dist(a.back(), b.back()) > d) return false;
    if (a.size() == 1 && b.size() == 1) return true;
    else if (a.size() == 1) return get_dist_to_point_sqr(b, a[0]) <= sqr(d);
    else if (b.size() == 1) return get_dist_to_point_sqr(a, b[0]) <= sqr(d);

    vector<interval> ra, rb, ra_out, rb_out;
    ra.push_back({0, get_last_reachable_point_from_start(a, b, d)});
    rb.push_back({0, get_last_reachable_point_from_start(b, a, d)});

    get_reachable_intervals(0, a.size() - 1, 0, b.size() - 1, a, b, d, ra, rb, ra_out, rb_out);

    return ra_out.size() && (ra_out.back().second >= b.size() - static_cast<distance_t>(1.5));
}

