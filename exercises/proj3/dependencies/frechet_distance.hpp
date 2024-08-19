#ifndef FRECHET_DISTANCE_HPP_INCLUDED
#define FRECHET_DISTANCE_HPP_INCLUDED

#include "geometry_basics.hpp"

// for get_frechet_distance
constexpr distance_t epsilon = 1e-5l;

/*
 * Checks whether the frechet distance is at most d, with the maximal accuracy distance_t allows.
 * O(a.size() * b.size())
 */
bool is_frechet_distance_at_most(const curve& a, const curve& b, distance_t d);

/*
 * Returns the frechet distance between a and b, accurate to +/- epsilon.
 * O(log(possible results) * a.size() * b.size())
 */
distance_t get_frechet_distance(const curve& a, const curve& b);

/*
 * Calculates an upper bound for the frechet distance of a and b by guessing a matching between a and b
 * O(a.size() + b.size())
 */
distance_t get_frechet_distance_upper_bound(const curve& a, const curve& b);

/*
 * Returns (a discrete approximation of) the first point c[j] on c, with j >= i, that is within distance d of point p.
 */
size_t nextclosepoint(const curve& c, size_t i, point p, distance_t d);

/*
 * Tries to show that the Frechet distance of a and b is more than d. Returns true if a proof is found.
 */
bool negfilter(const curve& c1, const curve& c2, distance_t d);



static inline curve translate(std::vector<std::tuple<double,double,double>> t){
    curve c;
    for (int i=0;i<t.size();i++){
        c.push_back({std::get<0>(t[i]),std::get<1>(t[i])});
    }
    return c;
}

 static inline double euclideanSqr(const point& a, const point& b) {
        return sqr(a.x-b.x)+sqr(a.y-b.y);
    }

static inline bool equalTime(const curve& t1, const curve& t2, double threshold_sqr) {
        int i, j;
        i = j = 0;
        double distance = euclideanSqr(t1[i], t2[j]);
        while (distance <= threshold_sqr && i + j < t1.size() + t2.size() - 2) {
            if (i == t1.size() - 1)
                distance = euclideanSqr(t1[i], t2[++j]);
            else if (j == t2.size() - 1)
                distance = euclideanSqr(t1[++i], t2[j]);
            else
                distance = euclideanSqr(t1[++i], t2[++j]);
        }
        return distance <= threshold_sqr;
    }

static inline bool similarity_test(std::vector<std::tuple<double,double,double>> t1,
                     std::vector<std::tuple<double,double,double>> t2,
                     double threshold){
    curve c1=translate(t1);
    curve c2=translate(t2);
    if (get_frechet_distance_upper_bound(c1, c2) <= threshold) {
        return true;
	}
	else if (negfilter(c1, c2, threshold)) {
	    return false;
	}
	else if (is_frechet_distance_at_most(c1, c2, threshold)) {
        return true;
	}
    
    return false;
}


static inline bool similarity_test_direct(const curve& c1,
                     const curve& c2,
                     double threshold){
    auto sqr_threshold = sqr(threshold);
    // check euclidean distance
    if (euclideanSqr(c1[0], c2[0]) > sqr_threshold || euclideanSqr(c1.back(), c2.back()) > sqr_threshold) 
        return false;
    
    // check equal time

    if (equalTime(c1, c2, sqr_threshold) || get_frechet_distance_upper_bound(c1, c2) <= threshold) {
        return true;
	}
	else if (negfilter(c1, c2, threshold)) {
	    return false;
	}

    // full check 
	else if (is_frechet_distance_at_most(c1, c2, threshold)) {
        return true;
	}
    
    return false;
}

#endif // FRECHET_DISTANCE_HPP_INCLUDED
