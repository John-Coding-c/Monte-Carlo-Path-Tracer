#pragma once

#include <cmath>
#include <memory>

#include "hittable.h"
#include "material.h"
#include "vec3.h"

class sphere : public hittable {
  public:
    sphere(const point3& center, double radius, std::shared_ptr<material> mat)
        : center(center), radius(std::fmax(0.0, radius)), mat(mat) {
        vec3 rvec(radius, radius, radius);
        bbox = aabb(center - rvec, center + rvec);
    }

    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        vec3 oc = center - r.origin();
        double a = r.direction().length_squared();
        double h = dot(r.direction(), oc);
        double c = oc.length_squared() - radius * radius;

        double discriminant = h * h - a * c;
        if (discriminant < 0) return false;
        double sqrtd = std::sqrt(discriminant);

        // Nearest valid root.
        double root = (h - sqrtd) / a;
        if (root <= t_min || root >= t_max) {
            root = (h + sqrtd) / a;
            if (root <= t_min || root >= t_max) return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v);
        rec.mat = mat;
        return true;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    point3 center;
    double radius;
    std::shared_ptr<material> mat;
    aabb bbox;

    // Point to (u,v) texture coordinates.
    static void get_sphere_uv(const point3& p, double& u, double& v) {
        double theta = std::acos(-p.y());
        double phi = std::atan2(-p.z(), p.x()) + 3.1415926535897932385;
        u = phi / (2 * 3.1415926535897932385);
        v = theta / 3.1415926535897932385;
    }
};
