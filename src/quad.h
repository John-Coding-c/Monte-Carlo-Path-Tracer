#pragma once

#include <cmath>
#include <memory>

#include "hittable.h"
#include "material.h"
#include "vec3.h"

// A parallelogram anchored at Q spanned by the vectors u and v.
class quad : public hittable {
  public:
    quad(const point3& Q, const vec3& u, const vec3& v, std::shared_ptr<material> mat)
        : Q(Q), u(u), v(v), mat(mat) {
        vec3 n = cross(u, v);
        normal = unit_vector(n);
        D = dot(normal, Q);
        w = n / dot(n, n);

        // Pad flat boxes so they have volume.
        aabb box_diagonal(Q, Q + u + v);
        aabb box_alternate(Q + u, Q + v);
        bbox = aabb(box_diagonal, box_alternate);
        for (int i = 0; i < 3; i++) {
            if (bbox.max_p[i] - bbox.min_p[i] < 1e-4) {
                bbox.min_p[i] -= 1e-4;
                bbox.max_p[i] += 1e-4;
            }
        }
    }

    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        double denom = dot(normal, r.direction());
        if (std::fabs(denom) < 1e-8) return false;  // ray parallel to the plane

        double t = (D - dot(normal, r.origin())) / denom;
        if (t <= t_min || t >= t_max) return false;

        // Check the hit lies inside the parallelogram.
        point3 intersection = r.at(t);
        vec3 planar_hitpt = intersection - Q;
        double alpha = dot(w, cross(planar_hitpt, v));
        double beta = dot(w, cross(u, planar_hitpt));
        if (alpha < 0 || alpha > 1 || beta < 0 || beta > 1) return false;

        rec.t = t;
        rec.p = intersection;
        rec.u = alpha;
        rec.v = beta;
        rec.mat = mat;
        rec.set_face_normal(r, normal);
        return true;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    point3 Q;
    vec3 u, v, w;
    vec3 normal;
    double D;
    std::shared_ptr<material> mat;
    aabb bbox;
};

// Axis-aligned box from two opposite corners, as six quads.
inline std::shared_ptr<hittable_list> make_box(const point3& a, const point3& b,
                                               std::shared_ptr<material> mat) {
    auto sides = std::make_shared<hittable_list>();

    point3 min_p(std::fmin(a.x(), b.x()), std::fmin(a.y(), b.y()), std::fmin(a.z(), b.z()));
    point3 max_p(std::fmax(a.x(), b.x()), std::fmax(a.y(), b.y()), std::fmax(a.z(), b.z()));

    vec3 dx(max_p.x() - min_p.x(), 0, 0);
    vec3 dy(0, max_p.y() - min_p.y(), 0);
    vec3 dz(0, 0, max_p.z() - min_p.z());

    sides->add(std::make_shared<quad>(point3(min_p.x(), min_p.y(), max_p.z()),  dx,  dy, mat));
    sides->add(std::make_shared<quad>(point3(max_p.x(), min_p.y(), max_p.z()), -dz,  dy, mat));
    sides->add(std::make_shared<quad>(point3(max_p.x(), min_p.y(), min_p.z()), -dx,  dy, mat));
    sides->add(std::make_shared<quad>(point3(min_p.x(), min_p.y(), min_p.z()),  dz,  dy, mat));
    sides->add(std::make_shared<quad>(point3(min_p.x(), max_p.y(), max_p.z()),  dx, -dz, mat));
    sides->add(std::make_shared<quad>(point3(min_p.x(), min_p.y(), min_p.z()),  dx,  dz, mat));
    return sides;
}
