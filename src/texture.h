#pragma once

#include <cmath>
#include <memory>

#include "vec3.h"

class texture {
  public:
    virtual ~texture() = default;
    virtual color value(double u, double v, const point3& p) const = 0;
};

class solid_color : public texture {
  public:
    solid_color(const color& albedo) : albedo(albedo) {}
    solid_color(double r, double g, double b) : albedo(r, g, b) {}

    color value(double, double, const point3&) const override { return albedo; }

  private:
    color albedo;
};

// 3D checkerboard pattern.
class checker_texture : public texture {
  public:
    checker_texture(double scale, std::shared_ptr<texture> even, std::shared_ptr<texture> odd)
        : inv_scale(1.0 / scale), even(even), odd(odd) {}

    checker_texture(double scale, const color& c1, const color& c2)
        : checker_texture(scale, std::make_shared<solid_color>(c1),
                          std::make_shared<solid_color>(c2)) {}

    color value(double u, double v, const point3& p) const override {
        int x = int(std::floor(inv_scale * p.x()));
        int y = int(std::floor(inv_scale * p.y()));
        int z = int(std::floor(inv_scale * p.z()));
        return ((x + y + z) % 2 == 0) ? even->value(u, v, p) : odd->value(u, v, p);
    }

  private:
    double inv_scale;
    std::shared_ptr<texture> even;
    std::shared_ptr<texture> odd;
};
