#pragma once

#include <cmath>

#include "ray.h"
#include "vec3.h"

// Pinhole camera with optional depth of field.
class camera {
  public:
    camera(point3 lookfrom, point3 lookat, vec3 vup, double vfov, double aspect_ratio,
           double defocus_angle, double focus_dist)
        : center(lookfrom), defocus_angle(defocus_angle) {
        double theta = vfov * 3.1415926535897932385 / 180.0;
        double h = std::tan(theta / 2);
        double viewport_height = 2.0 * h * focus_dist;
        double viewport_width = viewport_height * aspect_ratio;

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        vec3 viewport_u = viewport_width * u;
        vec3 viewport_v = viewport_height * -v;

        viewport_u_ = viewport_u;
        viewport_v_ = viewport_v;

        viewport_upper_left = center - (focus_dist * w) - viewport_u / 2 - viewport_v / 2;

        double defocus_radius = focus_dist * std::tan(defocus_angle * 3.1415926535897932385 / 360.0);
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    // (s,t) are normalized screen coordinates in [0,1].
    ray get_ray(double s, double t) const {
        point3 pixel_sample = viewport_upper_left + s * viewport_u_ + t * viewport_v_;
        point3 origin = defocus_angle <= 0 ? center : defocus_disk_sample();
        return ray(origin, pixel_sample - origin);
    }

  private:
    point3 center;
    double defocus_angle;
    vec3 u, v, w;
    vec3 viewport_u_, viewport_v_;
    point3 viewport_upper_left;
    vec3 defocus_disk_u, defocus_disk_v;

    point3 defocus_disk_sample() const {
        vec3 p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }
};
