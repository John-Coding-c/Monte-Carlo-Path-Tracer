#pragma once

#include <algorithm>
#include <limits>

#include "ray.h"
#include "vec3.h"

// Axis-aligned bounding box, used by the BVH.
class aabb {
  public:
    point3 min_p{ infinity_v(),  infinity_v(),  infinity_v()};
    point3 max_p{-infinity_v(), -infinity_v(), -infinity_v()};

    aabb() {}
    aabb(const point3& a, const point3& b) {
        for (int i = 0; i < 3; i++) {
            min_p[i] = std::fmin(a[i], b[i]);
            max_p[i] = std::fmax(a[i], b[i]);
        }
    }

    aabb(const aabb& a, const aabb& b) {
        for (int i = 0; i < 3; i++) {
            min_p[i] = std::fmin(a.min_p[i], b.min_p[i]);
            max_p[i] = std::fmax(a.max_p[i], b.max_p[i]);
        }
    }

    static double infinity_v() { return std::numeric_limits<double>::infinity(); }

    point3 centroid() const { return 0.5 * (min_p + max_p); }

    // Index of the longest axis.
    int longest_axis() const {
        vec3 d = max_p - min_p;
        if (d.x() > d.y()) return d.x() > d.z() ? 0 : 2;
        return d.y() > d.z() ? 1 : 2;
    }

    // Ray-box intersection test.
    bool hit(const ray& r, double t_min, double t_max) const {
        for (int a = 0; a < 3; a++) {
            double inv_d = 1.0 / r.direction()[a];
            double t0 = (min_p[a] - r.origin()[a]) * inv_d;
            double t1 = (max_p[a] - r.origin()[a]) * inv_d;
            if (inv_d < 0.0) std::swap(t0, t1);
            t_min = t0 > t_min ? t0 : t_min;
            t_max = t1 < t_max ? t1 : t_max;
            if (t_max <= t_min) return false;
        }
        return true;
    }
};
