#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "aabb.h"
#include "hittable.h"

// Bounding volume hierarchy, built once up front.
class bvh_node : public hittable {
  public:
    bvh_node(hittable_list list) : bvh_node(list.objects, 0, list.objects.size()) {}

    bvh_node(std::vector<std::shared_ptr<hittable>>& objects, size_t start, size_t end) {
        bbox = aabb();
        for (size_t i = start; i < end; i++) bbox = aabb(bbox, objects[i]->bounding_box());

        int axis = bbox.longest_axis();
        size_t span = end - start;

        if (span == 1) {
            left = right = objects[start];
        } else if (span == 2) {
            left = objects[start];
            right = objects[start + 1];
        } else {
            std::sort(objects.begin() + start, objects.begin() + end,
                      [axis](const std::shared_ptr<hittable>& a,
                             const std::shared_ptr<hittable>& b) {
                          return a->bounding_box().min_p[axis] < b->bounding_box().min_p[axis];
                      });
            size_t mid = start + span / 2;
            left = std::make_shared<bvh_node>(objects, start, mid);
            right = std::make_shared<bvh_node>(objects, mid, end);
        }
    }

    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override {
        if (!bbox.hit(r, t_min, t_max)) return false;

        bool hit_left = left->hit(r, t_min, t_max, rec);
        bool hit_right = right->hit(r, t_min, hit_left ? rec.t : t_max, rec);
        return hit_left || hit_right;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    std::shared_ptr<hittable> left;
    std::shared_ptr<hittable> right;
    aabb bbox;
};
