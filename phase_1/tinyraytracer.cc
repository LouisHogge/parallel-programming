#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include "geometry.hh"
#include "tinyraytracer.hh"

/* #define RENDER_BOARD */
/* #define RENDER_DUCK */
#ifdef RENDER_DUCK
#include "model.hh"
#endif

#define LOGO_DPI 100

#ifdef RENDER_DUCK
Model duck("duck.obj");
Material duck_material(1.0, Vec4f(0.9,  0.1, 0.0, 0.0), Vec3f(0.3, 0.1, 0.1), 10.);
#endif

static Vec3f reflect(const Vec3f &I, const Vec3f &N) {
  return I - N*2.f*(I*N);
}

static Vec3f refract(const Vec3f &I, const Vec3f &N, const float eta_t, const float eta_i=1.f) { // Snell's law
  float cosi = - std::max(-1.f, std::min(1.f, I*N));
  if (cosi<0) return refract(I, -N, eta_i, eta_t); // if the ray comes from the inside the object, swap the air and the media
  float eta = eta_i / eta_t;
  float k = 1 - eta*eta*(1 - cosi*cosi);
  return k<0 ? Vec3f(1,0,0) : I*eta + N*(eta*cosi - sqrtf(k)); // k<0 = total reflection, no ray to refract. I refract it anyways, this has no physical meaning
}

Tinyraytracer::Tinyraytracer(unsigned w, unsigned h, sf::Image env_img, sf::Image logo_img, Vec3f apos)
{
  width = w;
  height= h;
  const unsigned char * pixmap = env_img.getPixelsPtr();
  envmap_width = env_img.getSize().x;
  envmap_height = env_img.getSize().y;
  envmap = std::vector<Vec3f>(envmap_width * envmap_height);
  for (int i = 0; i < envmap_height * envmap_width; i++)
    envmap[i] = Vec3f(pixmap[4 * i + 0], pixmap[4 * i + 1], pixmap[4 * i + 2])*(1/255.);
  pixmap = logo_img.getPixelsPtr();
  logo_width = logo_img.getSize().x;
  logo_height = logo_img.getSize().y;
  logo_fwidth = logo_width * 1. / LOGO_DPI;
  logo_fheight = logo_height * 1. / LOGO_DPI;
  logo = std::vector<Vec4f>(logo_width * logo_height);
  for (int i = 0; i < logo_height * logo_width; i++)
    logo[i] = Vec4f(pixmap[4 * i + 0], pixmap[4 * i + 1],
		    pixmap[4 * i + 2], pixmap[4 * i + 3])*(1./255);
  logo_material = Material(1.0, Vec4f(1,  0, 0, 0), Vec3f(0.1, 0.1, 0.3),   10.);
  logo_pos = apos;
}

bool
Tinyraytracer::scene_intersect(const Vec3f &orig, const Vec3f &dir, Vec3f &hit, Vec3f &N, float anglel,
			       Material &material)
{
  float dist = std::numeric_limits<float>::max();
  for (const auto &s : spheres) {
    float dist_i;
    if (s.ray_intersect(orig, dir, dist_i) && dist_i < dist) {
      dist = dist_i;
      hit = orig + dir*dist_i;
      N = (hit - s.center).normalize();
      material = s.material;
    }
  }

#ifdef RENDER_BOARD
  float checkerboard_dist = std::numeric_limits<float>::max();
  if (fabs(dir.y)>1e-3)  {
    float d = -(orig.y+4)/dir.y; // the checkerboard plane has equation y = -4
    Vec3f pt = orig + dir*d;
    if (d>0 && fabs(pt.x)<10 && pt.z<-10 && pt.z>-30 && d<dist) {
      checkerboard_dist = d;
      hit = pt;
      N = Vec3f(0,1,0);
      material.diffuse_color = (int(.5*hit.x+1000) + int(.5*hit.z)) & 1 ? Vec3f(.3, .3, .3) : Vec3f(.3, .2, .1);
    }
  }
  if (dist > checkerboard_dist) dist = checkerboard_dist;
#endif

  logo_N = Vec3f(cos(anglel*M_PI/180),0.,sin(anglel*M_PI/180));
  logo_H = Vec3f(cos((anglel-90)*M_PI/180),0.,sin((anglel-90)*M_PI/180));
  logo_V = cross(logo_H, logo_N);  
  Vec3f p = logo_pos - orig;
  // compute point on the logo plane
  float logo_dist = (p * logo_N) / (dir * logo_N);
  p = dir * logo_dist - p;
  if (fabs(p * logo_V) * 2 < logo_fheight &&
      fabs(p * logo_H) * 2 < logo_fwidth ) // hit
    if (logo_dist > 0 && logo_dist < dist)
      {
	unsigned x = (p * logo_H + logo_fwidth / 2) / logo_fwidth * logo_width;
	unsigned y = (p * logo_V + logo_fheight / 2 ) / logo_fheight * logo_height;
	unsigned i = (x + y * logo_width);
	if (logo[i].w > 0)
	  {
	    dist = logo_dist;
	    hit = p + logo_pos;
	    N = logo_N;
	    if (N * dir > 0) N = -N;
	    material = logo_material;
	    material.diffuse_color = Vec3f(logo[i].x, logo[i].y, logo[i].z);
	    material.albedo.x = logo[i].w;
	    material.albedo.w = 1. - logo[i].w;
	  }
      }
  
#ifdef RENDER_DUCK
  for (int i = 0; i < duck.nfaces(); i++) {
    float dist_i;
    Vec3f N2;
    if (duck.ray_triangle_intersect(i, orig, dir, dist_i, N2) && dist_i < dist) {
      dist = dist_i;
      hit = orig + dir*dist_i;
      N = N2.normalize();
      material = duck_material;
    }
  }
#endif
  return dist < 1000;
}

Vec3f
Tinyraytracer::cast_ray(const Vec3f &orig, const Vec3f &dir, float anglel, size_t depth = 0)
{
  Vec3f point, N;
  Material material;
  
  if (depth>4 || !scene_intersect(orig, dir, point, N, anglel, material)) {
    int a = std::max(0, std::min(envmap_width -1, static_cast<int>((atan2(dir.z, dir.x)/(2*M_PI) + .5)*envmap_width)));
    int b = std::max(0, std::min(envmap_height-1, static_cast<int>(acos(dir.y)/M_PI*envmap_height)));
    return envmap[a+b*envmap_width]; // background color
    //        return Vec3f(0.2, 0.7, 0.8); // background color
  }

  Vec3f reflect_dir = reflect(dir, N).normalize();
  Vec3f refract_dir = refract(dir, N, material.refractive_index).normalize();
  Vec3f reflect_orig = reflect_dir*N < 0 ? point - N*1e-3 : point + N*1e-3; // offset the original point to avoid occlusion by the object itself
  Vec3f refract_orig = refract_dir*N < 0 ? point - N*1e-3 : point + N*1e-3;
  Vec3f reflect_color = cast_ray(reflect_orig, reflect_dir, anglel, depth + 1);
  Vec3f refract_color = cast_ray(refract_orig, refract_dir, anglel, depth + 1);
  
  float diffuse_light_intensity = 0, specular_light_intensity = 0;
  for (size_t i = 0; i < lights.size(); i++) {
    Vec3f light_dir      = (lights[i].position - point).normalize();
    float light_distance = (lights[i].position - point).norm();
    
    Vec3f shadow_orig = light_dir*N < 0 ? point - N*1e-3 : point + N*1e-3; // checking if the point lies in the shadow of the lights[i]
    Vec3f shadow_pt, shadow_N;
    Material tmpmaterial;
    if (scene_intersect(shadow_orig, light_dir, shadow_pt, shadow_N, anglel, tmpmaterial) &&
	(shadow_pt-shadow_orig).norm() < light_distance)
      continue;
    
    diffuse_light_intensity  += lights[i].intensity * std::max(0.f, light_dir*N);
    specular_light_intensity += powf(std::max(0.f, -reflect(-light_dir, N)*dir), material.specular_exponent)*lights[i].intensity;
  }
  return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + Vec3f(1., 1., 1.)*specular_light_intensity * material.albedo[1] + reflect_color*material.albedo[2] + refract_color*material.albedo[3];
}

sf::Image
Tinyraytracer::render(float anglev, float angleh, float anglel)
{
  const float fov      = M_PI/3.;
  std::vector<unsigned char> pixmap(4 * width * height);
  Vec3f ex(cos(angleh * M_PI / 180),
	   0,
	   - sin(angleh * M_PI / 180));
  Vec3f ey(sin(anglev * M_PI / 180) * sin(angleh * M_PI / 180),
	   cos(anglev * M_PI / 180),
	   sin(anglev * M_PI / 180) * cos(angleh * M_PI / 180));
  Vec3f ez(cos(anglev * M_PI / 180) * sin(angleh * M_PI / 180),
	   - sin(anglev * M_PI / 180),
	   cos(anglev * M_PI / 180) * cos(angleh * M_PI / 180));

  #pragma omp parallel for
  for (size_t j = 0; j < height; j++) // actual rendering loop
  { 
    for (size_t i = 0; i < width; i++) 
    {
      Vec3f v_0 = ex * ((i + 0.5) -  width/2.) + ey * (-(j + 0.5) + height/2.) + ez * (height / (-2. * tan(fov/2.) ));
      Vec3f f = cast_ray(Vec3f(0,0,0), v_0.normalize(), anglel);

      float max = std::max(f[0], std::max(f[1], f[2]));

      if (max > 1) 
        f = f * (1. / max);

      for (size_t k = 0; k < 3; k++)
        pixmap[(j * width + i) * 4 + k] = (unsigned char)(255 * std::max(0.f, std::min(1.f, f[k])));
      pixmap[(j * width + i) * 4 + 3] = 255;
    }
  }

  sf::Image result;
  result.create(width, height, pixmap.data());
  return result;
}

