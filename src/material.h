#pragma once

#include <memory>

#include "hittable.h"
#include "ray.h"
#include "texture.h"
#include "vec3.h"

class material {
  public:
    virtual ~material() = default;

    // Returns false when the ray is absorbed.
    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation,
                         ray& scattered) const {
        (void)r_in; (void)rec; (void)attenuation; (void)scattered;
        return false;
    }

    // Emitted light; black for non-emitters.
    virtual color emitted(double u, double v, const point3& p) const {
        (void)u; (void)v; (void)p;
        return color(0, 0, 0);
    }
};

// Diffuse surface.
class lambertian : public material {
  public:
    lambertian(const color& albedo) : tex(std::make_shared<solid_color>(albedo)) {}
    lambertian(std::shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation,
                 ray& scattered) const override {
        (void)r_in;
        vec3 scatter_direction = rec.normal + random_unit_vector();
        if (scatter_direction.near_zero()) scatter_direction = rec.normal;

        scattered = ray(rec.p, scatter_direction);
        attenuation = tex->value(rec.u, rec.v, rec.p);
        return true;
    }

  private:
    std::shared_ptr<texture> tex;
};

// Reflective surface with optional fuzz.
class metal : public material {
  public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation,
                 ray& scattered) const override {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        reflected = reflected + fuzz * random_unit_vector();
        scattered = ray(rec.p, reflected);
        attenuation = albedo;
        return dot(scattered.direction(), rec.normal) > 0;
    }

  private:
    color albedo;
    double fuzz;
};

// Glass: refracts or reflects.
class dielectric : public material {
  public:
    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation,
                 ray& scattered) const override {
        attenuation = color(1.0, 1.0, 1.0);
        double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;
        if (cannot_refract || reflectance(cos_theta, ri) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);

        scattered = ray(rec.p, direction);
        return true;
    }

  private:
    double refraction_index;

    // Schlick approximation of reflectance.
    static double reflectance(double cosine, double ri) {
        double r0 = (1 - ri) / (1 + ri);
        r0 = r0 * r0;
        return r0 + (1 - r0) * std::pow((1 - cosine), 5);
    }
};

class diffuse_light : public material {
  public:
    diffuse_light(const color& emit) : tex(std::make_shared<solid_color>(emit)) {}
    diffuse_light(std::shared_ptr<texture> tex) : tex(tex) {}

    color emitted(double u, double v, const point3& p) const override {
        return tex->value(u, v, p);
    }

  private:
    std::shared_ptr<texture> tex;
};
