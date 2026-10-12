// A multithreaded path tracer.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#include "bvh.h"
#include "camera.h"
#include "hittable.h"
#include "material.h"
#include "png.h"
#include "ray.h"
#include "scenes.h"
#include "vec3.h"

namespace {

const double kInfinity = std::numeric_limits<double>::infinity();

// Traces one ray path and returns the light it gathers.
color ray_color(ray r, const hittable& world, const sky& background, int max_depth) {
    color radiance(0, 0, 0);
    color throughput(1, 1, 1);

    for (int depth = 0; depth < max_depth; depth++) {
        hit_record rec;
        // Small offset to avoid re-hitting the surface just left.
        if (!world.hit(r, 0.001, kInfinity, rec)) {
            radiance += throughput * background.value(r.direction());
            break;
        }

        radiance += throughput * rec.mat->emitted(rec.u, rec.v, rec.p);

        ray scattered;
        color attenuation;
        if (!rec.mat->scatter(r, rec, attenuation, scattered)) break;

        throughput = throughput * attenuation;
        r = scattered;

        // Russian roulette: randomly end dim paths.
        if (depth >= 3) {
            double p = std::max({throughput.x(), throughput.y(), throughput.z()});
            p = std::clamp(p, 0.05, 1.0);
            if (random_double() >= p) break;
            throughput /= p;
        }
    }
    return radiance;
}

inline double linear_to_gamma(double linear) {
    return linear > 0 ? std::sqrt(linear) : 0;
}

inline uint8_t to_byte(double c) {
    if (!(c == c)) c = 0;  // NaN guard
    double g = linear_to_gamma(c);
    return uint8_t(256 * std::clamp(g, 0.0, 0.999));
}

struct options {
    std::string scene_name = "spheres";
    std::string out_path = "render.png";
    int width = 800;
    int samples = 100;
    int max_depth = 50;
    int threads = 0;  // 0 = hardware concurrency
    uint32_t seed = 1;
};

void print_usage() {
    std::cout <<
        "raytracer - a multithreaded path tracer\n\n"
        "Usage: raytracer [options]\n\n"
        "  --scene <name>    spheres | simple | cornell   (default: spheres)\n"
        "  --width <px>      image width                  (default: 800)\n"
        "  --samples <n>     samples per pixel            (default: 100)\n"
        "  --depth <n>       max ray bounces              (default: 50)\n"
        "  --threads <n>     worker threads, 0 = auto     (default: auto)\n"
        "  --seed <n>        RNG seed for the scene       (default: 1)\n"
        "  --out <file>      output PNG path              (default: render.png)\n"
        "  --help            show this message\n";
}

bool parse_args(int argc, char** argv, options& o) {
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto need = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::cerr << "error: " << name << " needs a value\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--help" || a == "-h") {
            print_usage();
            std::exit(0);
        } else if (a == "--scene") {
            o.scene_name = need("--scene");
        } else if (a == "--width") {
            o.width = std::atoi(need("--width"));
        } else if (a == "--samples") {
            o.samples = std::atoi(need("--samples"));
        } else if (a == "--depth") {
            o.max_depth = std::atoi(need("--depth"));
        } else if (a == "--threads") {
            o.threads = std::atoi(need("--threads"));
        } else if (a == "--seed") {
            o.seed = uint32_t(std::atoi(need("--seed")));
        } else if (a == "--out") {
            o.out_path = need("--out");
        } else {
            std::cerr << "error: unknown argument " << a << "\n\n";
            print_usage();
            return false;
        }
    }
    if (o.width < 1 || o.samples < 1 || o.max_depth < 1) {
        std::cerr << "error: width, samples and depth must all be >= 1\n";
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    options opt;
    if (!parse_args(argc, argv, opt)) return 2;

    seed_rng(opt.seed);

    scene sc;
    if (!build_scene(opt.scene_name, sc)) {
        std::cerr << "error: unknown scene " << opt.scene_name
                  << " (try spheres, simple or cornell)\n";
        return 2;
    }

    const int image_width = opt.width;
    const int image_height = std::max(1, int(image_width / sc.aspect_ratio));
    const double aspect = double(image_width) / double(image_height);

    camera cam(sc.lookfrom, sc.lookat, sc.vup, sc.vfov, aspect, sc.defocus_angle, sc.focus_dist);

    // Build the BVH.
    size_t object_count = sc.world.objects.size();
    auto build_start = std::chrono::steady_clock::now();
    bvh_node world(sc.world);
    auto build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - build_start).count();

    // Round samples up to a square grid.
    const int sqrt_spp = std::max(1, int(std::ceil(std::sqrt(double(opt.samples)))));
    const double inv_sqrt_spp = 1.0 / sqrt_spp;
    const int samples_per_pixel = sqrt_spp * sqrt_spp;

    unsigned hw = std::thread::hardware_concurrency();
    int num_threads = opt.threads > 0 ? opt.threads : int(hw ? hw : 4);
    num_threads = std::max(1, std::min(num_threads, image_height));

    std::cerr << "scene    : " << opt.scene_name << " (" << object_count
              << " objects, BVH built in " << build_ms << " ms)\n"
              << "image    : " << image_width << "x" << image_height << ", " << samples_per_pixel
              << " spp (" << sqrt_spp << "x" << sqrt_spp << " stratified), max depth "
              << opt.max_depth << "\n"
              << "threads  : " << num_threads << "\n";

    std::vector<uint8_t> pixels(size_t(image_width) * size_t(image_height) * 3);
    std::atomic<int> next_row{0};
    std::atomic<int> rows_done{0};

    auto render_start = std::chrono::steady_clock::now();

    // Workers take scanlines from a shared atomic counter.
    auto worker = [&]() {
        for (;;) {
            int j = next_row.fetch_add(1);
            if (j >= image_height) break;

            // Per-row seed, so output is the same for any thread count.
            seed_rng(opt.seed * 2654435761u + uint32_t(j) * 2246822519u + 1u);

            for (int i = 0; i < image_width; i++) {
                color pixel(0, 0, 0);
                // Stratified sampling over the pixel.
                for (int sj = 0; sj < sqrt_spp; sj++) {
                    for (int si = 0; si < sqrt_spp; si++) {
                        double px = (si + random_double()) * inv_sqrt_spp;
                        double py = (sj + random_double()) * inv_sqrt_spp;
                        double u = (i + px) / image_width;
                        double v = (j + py) / image_height;
                        pixel += ray_color(cam.get_ray(u, v), world, sc.background, opt.max_depth);
                    }
                }
                pixel /= double(samples_per_pixel);

                size_t idx = (size_t(j) * size_t(image_width) + size_t(i)) * 3;
                pixels[idx + 0] = to_byte(pixel.x());
                pixels[idx + 1] = to_byte(pixel.y());
                pixels[idx + 2] = to_byte(pixel.z());
            }

            int done = rows_done.fetch_add(1) + 1;
            if (done % 8 == 0 || done == image_height) {
                std::cerr << "\rrendering: " << done << "/" << image_height << " rows ("
                          << (100 * done / image_height) << "%)   " << std::flush;
            }
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(size_t(num_threads));
    for (int t = 0; t < num_threads; t++) pool.emplace_back(worker);
    for (auto& th : pool) th.join();

    auto render_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - render_start).count();
    std::cerr << "\rrendering: done in " << (render_ms / 1000.0) << " s"
              << std::string(20, ' ') << "\n";

    if (!png::write_rgb(opt.out_path, image_width, image_height, pixels)) {
        std::cerr << "error: could not write " << opt.out_path << "\n";
        return 1;
    }
    std::cerr << "wrote    : " << opt.out_path << "\n";
    return 0;
}
