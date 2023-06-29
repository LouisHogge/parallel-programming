#ifndef _TINYRAYTRACER_HH
#define _TINYRAYTRACER_HH

#include <SFML/Graphics.hpp>

#include "geometry.hh"

struct Light {
  Vec3f position;
  float intensity;
  Light(const Vec3f &p, const float i) :
    position(p), intensity(i) { };
};

struct Material {
  float refractive_index;
  Vec4f albedo;
  Vec3f diffuse_color;
  float specular_exponent;
  Material(const float r, const Vec4f &a, const Vec3f &color, const float spec) :
    refractive_index(r), albedo(a), diffuse_color(color), specular_exponent(spec) {};
  Material() :
    refractive_index(1), albedo(1,0,0,0), diffuse_color(), specular_exponent() {};
};
  
struct Sphere {
  Vec3f center;
  float radius;
  Material material;

  Sphere(const Vec3f &c, const float r, const Material &m) : center(c), radius(r), material(m) {}

  bool ray_intersect(const Vec3f &orig, const Vec3f &dir, float &t0) const {
    Vec3f L = center - orig;
    float tca = L*dir;
    float d2 = L*L - tca*tca;
    if (d2 > radius*radius) return false;
    float thc = sqrtf(radius*radius - d2);
    t0       = tca - thc;
    float t1 = tca + thc;
    if (t0 < 0) t0 = t1;
    if (t0 < 0) return false;
    return true;
  }
};

class Tinyraytracer {
  unsigned width, height;
  int envmap_width, envmap_height;
  std::vector<Vec3f> envmap;
  int logo_width, logo_height;
  std::vector<Vec4f> logo;
  float logo_fwidth, logo_fheight;
  Vec3f logo_N;
  Vec3f logo_H;
  Vec3f logo_V;
  Vec3f logo_pos;
  Material logo_material;  
  std::vector<Sphere> spheres;
  std::vector<Light> lights;

public:
  Tinyraytracer(unsigned w, unsigned h, sf::Image env_img, sf::Image logo_img, Vec3f apos);
  void add_sphere(Sphere s) { spheres.push_back(s); };
  void add_light(Light l) { lights.push_back(l); };
  sf::Image render(float anglev, float angleh, float anglel);
private:
  bool scene_intersect(const Vec3f &orig, const Vec3f &dir, Vec3f &hit, Vec3f &N, float anglel,
		       Material &material);
  Vec3f cast_ray(const Vec3f &orig, const Vec3f &dir, float anglel, size_t depth);
};

#endif
