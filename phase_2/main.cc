#include <cstring>

#include "tinyraytracer.hh"

#include <thread>
#include <mutex>
#include <queue>

/*
#define WIDTH 1024
#define HEIGHT 768
*/
#define WIDTH 512
#define HEIGHT 384

float fps = 10.; //approximation of fps at first iteration
int counter = 0;


// structures
typedef struct angle_t{
  float h, v, logo;
  int priorityAngle;
}Angle;

typedef struct image_t{
  sf::Image imageQueue;
  int priority;

  image_t(sf::Image imageQueue, int priority)
    : imageQueue(imageQueue), priority(priority)
  {
  }
}ImageInQueue;

struct ComparePriorityImage{
  bool operator()(ImageInQueue const& image1, ImageInQueue const& image2){
    return image1.priority > image2.priority;
  }
};


// global variables
std::mutex mutex;
std:: queue<Angle> angle_queue;
std:: priority_queue<ImageInQueue, std::vector<ImageInQueue>, ComparePriorityImage> image_queue; 
bool exit_threadDisplayAndUserInteraction = false;
bool exit_computation_threads = false;


// prototypes
void displayAndUserInteraction(Tinyraytracer tinyraytracer, bool animate);
void computationImage(Tinyraytracer tinyraytracer);


int
main(int argc, char *argv[]) 
{
  bool gui = false, animate = false;

  if (argc > 1)
    for (int i = 1; i < argc; i++) {
      bool full = (!strcmp(argv[i], "-full"));
      gui |= full | (!strcmp(argv[i], "-gui"));
      animate |= full | (!strcmp(argv[i], "-animate"));
    } 
  
  sf::Image background;
  if (!background.loadFromFile("envmap.jpg")) {
    std::cerr << "Error: can not load the environment map" << std::endl;
    return -1;
  }

  sf::Image logo;
  if (!logo.loadFromFile("logo.png"))  {
    std::cerr << "Error: can not load logo" << std::endl;
    return -1;
  }

  Tinyraytracer tinyraytracer(WIDTH, HEIGHT, background, logo, Vec3f(-4,2,-10));

  Material      ivory(1.0, Vec4f(0.6,  0.3, 0.1, 0.0), Vec3f(0.4, 0.4, 0.3),   50.);
  //  Material      glass(1.5, Vec4f(0.0,  0.5, 0.1, 0.8), Vec3f(0.6, 0.7, 0.8),  125.);
  Material red_rubber(1.0, Vec4f(0.9,  0.1, 0.0, 0.0), Vec3f(0.3, 0.1, 0.1),   10.);
  Material     mirror(1.0, Vec4f(0.0, 10.0, 0.8, 0.0), Vec3f(1.0, 1.0, 1.0), 1425.);
  
  tinyraytracer.add_sphere(Sphere(Vec3f(-3,    0,   -16), 2,      ivory));
  //  tinyraytracer.add_sphere(Sphere(Vec3f(-1.0, -1.5, -12), 2,      glass));
  tinyraytracer.add_sphere(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
  tinyraytracer.add_sphere(Sphere(Vec3f( 7,    5,   -18), 4,     mirror));

  tinyraytracer.add_light(Light(Vec3f(-20, 20,  20), 1.5));
  tinyraytracer.add_light(Light(Vec3f( 30, 50, -25), 1.8));
  tinyraytracer.add_light(Light(Vec3f( 30, 20,  30), 1.7));

  if (gui)
  {
    std::thread computation_threads[3];
    for (size_t i = 0; i<3; i++)
    {
      computation_threads[i] = std::thread(computationImage, tinyraytracer);
      computation_threads[i].detach();
    }
    
    std::thread threadDisplayAndUserInteraction(displayAndUserInteraction, tinyraytracer, animate);
    threadDisplayAndUserInteraction.join();
  }
  else
  {
    sf::Image result = tinyraytracer.render(0, 0, 15);
    result.saveToFile("out.jpg");
  }

  return 0;
}

void displayAndUserInteraction(Tinyraytracer tinyraytracer, bool animate)
{
  sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "TinyRT");
  sf::Image result;
  sf::Texture texture;
  sf::Sprite sprite;
  float angle_h = 0., angle_v = 0.;
  float angle_logo = 15.;
  sf::Clock clock;
  clock.restart();
  
  window.setFramerateLimit(150);
  window.clear();
  window.display();

  sf::Image starting_image = tinyraytracer.render(angle_v, angle_h, angle_logo);
  texture.loadFromImage(starting_image);
  sprite.setTexture(texture);
  window.draw(sprite);
  window.display();
  
  while (window.isOpen() && !exit_threadDisplayAndUserInteraction)
  {
    sf::Event event;
    bool update = false;
    if (animate)
    {
      bool angle_queue_too_big = true;
      mutex.lock();
      if (angle_queue.size() < 10)
        angle_queue_too_big = false;
      mutex.unlock();

      if (!angle_queue_too_big)
      {
        mutex.lock();
        angle_logo += angle_logo >= 359.? -359. : 6./fps;
        update = true;
        mutex.unlock();
      }
    }
    if (window.pollEvent(event))
    {
      if (event.type == sf::Event::KeyPressed)
      {
        if (event.key.code == sf::Keyboard::Left)
        {
          mutex.lock();
          angle_h += angle_h >= 359. ? -359. : 1.;
          update = true;
          mutex.unlock();
          std::cerr << "Key pressed: " << "Left" << std::endl;
        }
        else if (event.key.code == sf::Keyboard::Right)
        {
          mutex.lock();
          angle_h -= angle_h < 1. ? -359 : 1.;
          update = true;
          mutex.unlock();
          std::cerr << "Key pressed: " << "Right" << std::endl;
        }
        else if (event.key.code == sf::Keyboard::Up)
        {
          mutex.lock();
          angle_v +=  angle_v >= 359. ? -359. : 1.;
          update = true;
          mutex.unlock();
          std::cerr << "Key pressed: " << "Up" << std::endl;
        }
        else if (event.key.code == sf::Keyboard::Down)
        {
          mutex.lock();
          angle_v -= angle_v < 1. ? -359 : 1.;
          update = true;
          mutex.unlock();
          std::cerr << "Key pressed: " << "Down" << std::endl;
        }
        else if (event.key.code == sf::Keyboard::Space)
          std::cerr << "Key pressed: " << "Space" << std::endl;
        else if (event.key.code == sf::Keyboard::Q)
        {
          exit_threadDisplayAndUserInteraction = true;
          exit_computation_threads = true;
          window.close();
        }
        else
          std::cerr << "Key pressed: " << "Unknown" << std::endl;
      }
      if (event.type == sf::Event::Closed)
      {
        exit_threadDisplayAndUserInteraction = true;
        exit_computation_threads = true;
        window.close();
      }
    }

    if (update)
    {
      Angle angle;
      angle.logo = angle_logo;
      angle.h = angle_h;
      angle.v = angle_v;
      angle.priorityAngle = counter;

      // send angles in variable "angle" to global queue
      mutex.lock();
      angle_queue.push(angle);
      counter++;
      mutex.unlock();
    }

    static unsigned framecount = 0;
    bool image_queue_empty = true;

    mutex.lock();
    // check if image queue is empty
    if(!(image_queue.empty()))
      image_queue_empty = false;
    mutex.unlock();

    // if image queue not empty we get the image with highest priority and delete it from the queue
    if(!image_queue_empty)
    {
      mutex.lock();
      ImageInQueue current_image = image_queue.top();
      image_queue.pop();
      mutex.unlock();

      // we display the current image
      texture.loadFromImage(current_image.imageQueue); // avant parametre = img
      window.clear();
      window.draw(sprite);
      window.display();
      framecount++;
      sf::Time currentTime = clock.getElapsedTime();
      if (currentTime.asSeconds() > 1.0)
      {
        mutex.lock();
        fps = framecount / currentTime.asSeconds();
        mutex.unlock();
        std::cout << "fps: " << fps << std::endl;
        clock.restart();
        framecount = 0;
      }
    }
  }
}

void computationImage(Tinyraytracer tinyraytracer)
{
  // counter for priority of images

  while(!exit_computation_threads)
  {
    // check if angle queue is empty
    bool angle_queue_empty = true;
    mutex.lock();
    if(!(angle_queue.empty()) && image_queue.size() < 10)
      angle_queue_empty = false;
    mutex.unlock();

    // if angle queue not empty
    if(!angle_queue_empty)
    { 
      // we get the angle and delete it from the queue
      mutex.lock();
      Angle current_angles = angle_queue.front();
      angle_queue.pop();
      mutex.unlock();

      // computation of images
      int priority = current_angles.priorityAngle;
      sf::Image image_render = tinyraytracer.render(current_angles.v, current_angles.h, current_angles.logo);

      // store image with is priority and increment counter for future priorities
      mutex.lock();
      image_queue.push(image_t(image_render, priority));  
      mutex.unlock();
    }
  }   
} 
