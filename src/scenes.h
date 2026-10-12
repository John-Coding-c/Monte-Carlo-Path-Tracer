#pragma once

#include <memory>
#include <string>

#include "bvh.h"
#include "camera.h"
#include "hittable.h"
#include "material.h"
#include "quad.h"
#include "sphere.h"
#include "texture.h"

// What a ray sees when it escapes the scene.
struct sky {
    color top = color(0.70, 0.80, 1.00);
    bool gradient = true;  // false: flat `top`

    color value(const vec3& direction) const {
        if (!gradient) return top;
        double a = 0.5 * (unit_vector(direction).y() + 1.0);
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a * top;
    }
};

struct scene {
    hittable_list world;
    sky background;  // what a ray hits when it escapes
    point3 lookfrom = point3(13, 2, 3);
    point3 lookat = point3(0, 0, 0);
    vec3 vup = vec3(0, 1, 0);
    double vfov = 20;
    double defocus_angle = 0.0;
    double focus_dist = 10.0;
    double aspect_ratio = 16.0 / 9.0;
};

// Random spheres scene.
inline scene scene_spheres() {
    scene s;

    auto checker = std::make_shared<checker_texture>(0.32, color(0.2, 0.3, 0.1),
                                                     color(0.9, 0.9, 0.9));
    s.world.add(std::make_shared<sphere>(point3(0, -1000, 0), 1000,
                                         std::make_shared<lambertian>(checker)));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            double choose_mat = random_double();
            point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());
            if ((center - point3(4, 0.2, 0)).length() <= 0.9) continue;

            std::shared_ptr<material> mat;
            if (choose_mat < 0.8) {
                mat = std::make_shared<lambertian>(color::random() * color::random());
            } else if (choose_mat < 0.95) {
                mat = std::make_shared<metal>(color::random(0.5, 1), random_double(0, 0.5));
            } else {
                mat = std::make_shared<dielectric>(1.5);
            }
            s.world.add(std::make_shared<sphere>(center, 0.2, mat));
        }
    }

    s.world.add(std::make_shared<sphere>(point3(0, 1, 0), 1.0,
                                         std::make_shared<dielectric>(1.5)));
    s.world.add(std::make_shared<sphere>(point3(-4, 1, 0), 1.0,
                                         std::make_shared<lambertian>(color(0.4, 0.2, 0.1))));
    s.world.add(std::make_shared<sphere>(point3(4, 1, 0), 1.0,
                                         std::make_shared<metal>(color(0.7, 0.6, 0.5), 0.0)));

    s.lookfrom = point3(13, 2, 3);
    s.lookat = point3(0, 0, 0);
    s.vfov = 20;
    s.defocus_angle = 0.6;
    s.focus_dist = 10.0;
    return s;
}

// Three balls on a plane.
inline scene scene_simple() {
    scene s;
    s.world.add(std::make_shared<sphere>(point3(0, -100.5, -1), 100,
                                         std::make_shared<lambertian>(color(0.8, 0.8, 0.0))));
    s.world.add(std::make_shared<sphere>(point3(0, 0, -1.2), 0.5,
                                         std::make_shared<lambertian>(color(0.1, 0.2, 0.5))));
    s.world.add(std::make_shared<sphere>(point3(-1, 0, -1), 0.5,
                                         std::make_shared<dielectric>(1.5)));
    s.world.add(std::make_shared<sphere>(point3(-1, 0, -1), 0.4,
                                         std::make_shared<dielectric>(1.0 / 1.5)));  // bubble
    s.world.add(std::make_shared<sphere>(point3(1, 0, -1), 0.5,
                                         std::make_shared<metal>(color(0.8, 0.6, 0.2), 0.1)));

    s.lookfrom = point3(-2, 2, 1);
    s.lookat = point3(0, 0, -1);
    s.vfov = 35;
    s.defocus_angle = 0.0;
    return s;
}

// Cornell box.
inline scene scene_cornell() {
    scene s;
    auto red = std::make_shared<lambertian>(color(0.65, 0.05, 0.05));
    auto white = std::make_shared<lambertian>(color(0.73, 0.73, 0.73));
    auto green = std::make_shared<lambertian>(color(0.12, 0.45, 0.15));
    auto light = std::make_shared<diffuse_light>(color(15, 15, 15));

    s.world.add(std::make_shared<quad>(point3(555, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), green));
    s.world.add(std::make_shared<quad>(point3(0, 0, 0), vec3(0, 555, 0), vec3(0, 0, 555), red));
    s.world.add(std::make_shared<quad>(point3(343, 554, 332), vec3(-130, 0, 0), vec3(0, 0, -105), light));
    s.world.add(std::make_shared<quad>(point3(0, 0, 0), vec3(555, 0, 0), vec3(0, 0, 555), white));
    s.world.add(std::make_shared<quad>(point3(555, 555, 555), vec3(-555, 0, 0), vec3(0, 0, -555), white));
    s.world.add(std::make_shared<quad>(point3(0, 0, 555), vec3(555, 0, 0), vec3(0, 555, 0), white));

    // Two boxes and two spheres.
    auto box1 = make_box(point3(130, 0, 65), point3(295, 165, 230), white);
    for (auto& o : box1->objects) s.world.add(o);
    auto box2 = make_box(point3(265, 0, 295), point3(430, 330, 460), white);
    for (auto& o : box2->objects) s.world.add(o);

    s.world.add(std::make_shared<sphere>(point3(190, 250, 145), 80,
                                         std::make_shared<dielectric>(1.5)));
    s.world.add(std::make_shared<sphere>(point3(348, 410, 377), 80,
                                         std::make_shared<metal>(color(0.8, 0.85, 0.88), 0.0)));

    s.background.top = color(0, 0, 0);
    s.background.gradient = false;
    s.lookfrom = point3(278, 278, -800);
    s.lookat = point3(278, 278, 0);
    s.vfov = 40;
    s.aspect_ratio = 1.0;
    s.defocus_angle = 0.0;
    return s;
}

inline bool build_scene(const std::string& name, scene& out) {
    if (name == "spheres") { out = scene_spheres(); return true; }
    if (name == "simple")  { out = scene_simple();  return true; }
    if (name == "cornell") { out = scene_cornell(); return true; }
    return false;
}
