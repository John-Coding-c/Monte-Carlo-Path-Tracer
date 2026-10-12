#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>

// ---------------------------------------------------------------------------
// Random numbers (per-thread generator).
// ---------------------------------------------------------------------------

inline uint32_t& rng_state() {
    static thread_local uint32_t state = 0x9E3779B9u;
    return state;
}

inline void seed_rng(uint32_t seed) {
    rng_state() = seed ? seed : 0x9E3779B9u;
}

// xorshift32 RNG.
inline uint32_t next_u32() {
    uint32_t x = rng_state();
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state() = x;
    return x;
}

// Uniform in [0,1).
inline double random_double() {
    return (next_u32() >> 8) * (1.0 / 16777216.0);
}

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

inline int random_int(int min, int max) {  // inclusive
    return min + int(random_double() * (max - min + 1));
}

// ---------------------------------------------------------------------------
// vec3
// ---------------------------------------------------------------------------

struct vec3 {
    double e[3];

    vec3() : e{0, 0, 0} {}
    vec3(double x, double y, double z) : e{x, y, z} {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    double operator[](int i) const { return e[i]; }
    double& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator*=(double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(double t) { return *this *= 1 / t; }

    double length() const { return std::sqrt(length_squared()); }
    double length_squared() const { return e[0] * e[0] + e[1] * e[1] + e[2] * e[2]; }

    // True if all components are near zero.
    bool near_zero() const {
        const double s = 1e-8;
        return std::fabs(e[0]) < s && std::fabs(e[1]) < s && std::fabs(e[2]) < s;
    }

    static vec3 random() { return vec3(random_double(), random_double(), random_double()); }

    static vec3 random(double min, double max) {
        return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
    }
};

using point3 = vec3;
using color = vec3;

inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

inline vec3 operator+(const vec3& u, const vec3& v) {
    return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline vec3 operator-(const vec3& u, const vec3& v) {
    return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

inline vec3 operator*(const vec3& u, const vec3& v) {
    return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(double t, const vec3& v) { return vec3(t * v.e[0], t * v.e[1], t * v.e[2]); }
inline vec3 operator*(const vec3& v, double t) { return t * v; }
inline vec3 operator/(const vec3& v, double t) { return (1 / t) * v; }

inline double dot(const vec3& u, const vec3& v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

inline vec3 cross(const vec3& u, const vec3& v) {
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline vec3 unit_vector(const vec3& v) { return v / v.length(); }

// Random point on the unit sphere.
inline vec3 random_unit_vector() {
    while (true) {
        vec3 p = vec3::random(-1, 1);
        double lensq = p.length_squared();
        if (lensq > 1e-160 && lensq <= 1.0) return p / std::sqrt(lensq);
    }
}

inline vec3 random_in_unit_disk() {
    while (true) {
        vec3 p(random_double(-1, 1), random_double(-1, 1), 0);
        if (p.length_squared() < 1.0) return p;
    }
}

inline vec3 reflect(const vec3& v, const vec3& n) { return v - 2 * dot(v, n) * n; }

// Refracts uv through n (Snell's law).
inline vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
    double cos_theta = std::fmin(dot(-uv, n), 1.0);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}
