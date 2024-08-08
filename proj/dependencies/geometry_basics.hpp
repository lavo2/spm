#ifndef GEOMETRY_BASICS_HPP_INCLUDED
#define GEOMETRY_BASICS_HPP_INCLUDED

/*
 * Defines the classes point, curve and a function intersection_interval to calculate intersections between points and circles, as well as some helper function and operators.
 */

#include <iostream>
#include <limits>
#include <tuple>
#include <array>
#include <vector>
#include <cmath>
#define TRAJ_MAX_SIZE 70
/*
 * The datatype to use for all calculations that require floating point numbers.
 * double seems to be enough for all testet inputs.
 * For higher precision, long double or boost::multiprecision could be used.
 */

using namespace std;
typedef double distance_t;

// bounding box
extern distance_t min_x, min_y, max_x, max_y;

struct point {
    distance_t x, y;
    point(distance_t x, distance_t y) : x(x), y(y) {}
    point() {}
    template<class Archive>
    void serialize(Archive & archive) {
        archive(x, y);
    }
};

typedef point vec;

inline point& operator-=(point& a, const point& b)
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
}

inline point operator-(const point& a, const point& b)
{
    auto tmp = a;
    tmp -= b;
    return tmp;
}

inline point& operator+=(point& a, const point& b)
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

inline point operator+(const point& a, const point& b)
{
    auto tmp = a;
    tmp += b;
    return tmp;
}

inline point& operator/=(point& a, distance_t d)
{
    a.x /= d;
    a.y /= d;
    return a;
}

inline std::ostream& operator<<(std::ostream& out, const point& p)
{
    out << "(" << p.x << ", " << p.y << ")";
    return out;
}

template<typename T>
inline T sqr(T d)
{
    return d * d;
}

inline distance_t dist(const point& a, const point& b)
{
    return std::sqrt(sqr(a.x - b.x) + sqr(a.y - b.y));
}

inline distance_t dist_sqr(const point& a, const point& b)
{
    return sqr(a.x - b.x) + sqr(a.y - b.y);
}

typedef std::pair<distance_t, distance_t> interval; // .first is the startpoint, .second the endpoint (start inclusive, end exclusive)

constexpr interval empty_interval{std::numeric_limits<distance_t>::max(), std::numeric_limits<distance_t>::lowest()};

inline std::ostream& operator<<(std::ostream& out, const interval& i)
{
    out << "[" << i.first << ", " << i.second << "]";
    return out;
}

inline bool is_empty_interval(const interval& i)
{
    return i.first >= i.second;
}

/*
 * Represents a trajectory. Additionally to the points given in the input file, we also store the length of any prefix of the trajectory.
 */
class curve {
public:
    
    /*inline curve(const mm::vector<point>& v): points(v), actualSize(v.size()), prefix_length(v.size())
    {
        prefix_length[0] = 0;
#ifdef DISABLE_FF_DISTRIBUTED        
        for (int i = 1; i < v.size(); ++i) prefix_length[i] = prefix_length[i - 1] + dist(v[i - 1], v[i]);
#endif
    }*/

    inline curve() = default;

    inline size_t size() const { return actualSize; }

    inline point operator[](size_t i) const
    {
        return points[i];
    }

    inline distance_t curve_length(size_t i, size_t j) const
    {
        return prefix_length[j] - prefix_length[i];
    }

    inline point front() const { return points.front(); }

    inline point back() const { return points[actualSize-1]; }
 
    inline void pop_back()
    {
        if (actualSize) --actualSize;
    }

    inline void push_back(point p)
    {
        if (actualSize == TRAJ_MAX_SIZE) return;
        if (actualSize) prefix_length[actualSize] = prefix_length[actualSize-1] + dist(points[actualSize-1], p);
        //if (prefix_length.size()) prefix_length.push_back(prefix_length.back() + dist(points.back(), p));
        points[actualSize++] = p;
    }

    template<class Archive>
    void save(Archive & archive) const  {
        archive(points, actualSize); 
    }

    template<class Archive>
    void load(Archive & archive){
        archive(points, actualSize);
        prefix_length[0] = 0;
        for (int i = 1; i < actualSize; ++i) 
            prefix_length[i] = prefix_length[i - 1] + dist(points[i - 1], points[i]);
    }

private:
    std::array<point, TRAJ_MAX_SIZE> points{};
    std::array<distance_t, TRAJ_MAX_SIZE> prefix_length{};
    size_t actualSize = 0;



    friend std::array<point, TRAJ_MAX_SIZE>::const_iterator begin(const curve& c);
    friend std::array<point, TRAJ_MAX_SIZE>::const_iterator end(const curve& c);
};

inline std::array<point, TRAJ_MAX_SIZE>::const_iterator begin(const curve& c)
{
    return c.points.begin();
}

inline std::array<point, TRAJ_MAX_SIZE>::const_iterator end(const curve& c)
{
    return c.points.data()+c.actualSize;
}

inline std::ostream& operator<<(std::ostream& out, const curve& c)
{
    out << "[";
    for (auto p: c) out << p << ", ";
    out << "]";
    return out;
}



/*
 * Returns which section of the line segment from line_start to line_end is inside the circle given by circle_center and radius.
 * If the whole line segment lies inside the circle, the result would be [0, 1].
 * If the circle and line segment do not intersect, the result is the empty interval.
 */
interval intersection_interval(point circle_center, distance_t radius, point line_start, point line_end);

#endif // GEOMETRY_BASICS_HPP_INCLUDED
