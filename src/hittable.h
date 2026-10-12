#pragma once

#include <memory>
#include <vector>

#include "aabb.h"
#include "ray.h"
#include "vec3.h"

class material;

struct hit_record {
    point3 p;                          // point of intersection
    vec3 normal;                       // always faces against the incoming ray
    std::shared_ptr<material> mat;
    double t = 0;
    double u = 0, v = 0;               // surface coordinates, for textures
    bool front_face = false;

    // `outward_normal` is assumed to be unit length.
    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
  public:
    virtual ~hittable() = default;
    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const = 0;
    virtual aabb bounding_box() const = 0;
};

class hittable_list : public hittable {
  public:
    std::vector<std::shared_ptr<hittable>> objects;

    hittable_list() {}

    void clear() { objects.clear(); }

    void add(std::shared_ptr<hittable> object) {
        objects.push_back(object);
        bbox = aabb(bbox, object->bounding_box());
    }

    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        hit_record temp;
        bool hit_anything = false;
        double closest = t_max;

        for (const auto& object : objects) {
            if (object->hit(r, t_min, closest, temp)) {
                hit_anything = true;
                closest = temp.t;
                rec = temp;
            }
        }
        return hit_anything;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    aabb bbox;
};
