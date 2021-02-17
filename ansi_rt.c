/*
 * ansi_rt.c: A terminal-based raytracer
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <math.h>

//////////////////////////////
// PREPROCESSOR
//
#define DRAW_LIGHTS      /* Whether or not to render lights as spheres */
#define MAX_OBJS 16      /* Maximum number of objects which can be added to a scene */
#define MAX_DEPTH 100.0f /* Maximum depth to check for object-ray intersections.  Larger values mean slower render */
#define STEP_SIZE 1.0f   /* Size of steps along ray when calculating intersections.  Smaller values mean slower render, but higher quality */

#define WIDTH (ws.ws_col/2)
#define HEIGHT (ws.ws_row)

#define WRAP_PIXEL(p) if(p.r>255){p.r=255;}if(p.g>255){p.g=255;}if(p.b>255){p.b=255;}if(p.r<0){p.r=0;}if(p.g<0){p.g=0;}if(p.b<0){p.b=0;}

//////////////////////////////
// TYPEDEFS AND ENUMS
//
typedef struct {
  int r;
  int g;
  int b;
} pixel;

typedef struct {
  double x;
  double y;
  double z;
} vec3;

typedef struct {
  vec3 origin; /* Starting point of the ray */
  vec3 pass;   /* Point which establishes the direction of the ray (in the context of the screen ray, the screen position) */
  vec3 step;   /* Current point along the ray */
} ray;

typedef struct {
  int type;
  int reflective;
  vec3 *scale;
  vec3 *pos;
  vec3 *rot;
  pixel *material;
} object;

enum {
  VIEWPOINT = 'v',

  CUBE = 'c',
  SPHERE = 's',
  PLANE = 'p',
  LIGHT = 'l'
};

//////////////////////////////
// GEOMETRY
//
int sphere(double r, vec3 center, vec3 rot, vec3 test){
  return (pow(center.x-test.x, 2) +
          pow(center.y-test.y, 2) +
          pow(center.z-test.z, 2) <=
          pow(r, 2));
}

int cube(double side, vec3 center, vec3 rot, vec3 test){
  return (fabs(test.x-center.x) <= side &&
          fabs(test.y-center.y) <= side &&
          fabs(test.z-center.z) <= side);
/*
  return (fabs(
            center.x+
            (center.x*cos(rot.y))-
            (center.z*sin(rot.y))+
            (center.x*cos(rot.z))-
            (center.y*sin(rot.z))-
            test.x) <= side &&
          fabs(
            center.y+
            (center.y*cos(rot.x))-
            (center.z*sin(rot.x))+
            (center.x*sin(rot.z))+
            (center.y*cos(rot.z))-
            test.y) <= side &&
          fabs(
            center.z+
            (center.y*sin(rot.x))+
            (center.z*cos(rot.x))+
            (center.x*sin(rot.y))+
            (center.z*cos(rot.y))-
            test.z) <= side);
*/
}

int plane(double side, vec3 center, vec3 rot, vec3 test){
  return (fabs(test.x-center.x) <= side &&
          fabs(test.y-center.y) <= 1    &&
          fabs(test.z-center.z) <= side);
}

/* TODO: Function pointers */
int intersect(object *o, vec3 point){
  switch(o->type){
    case CUBE:
      return cube(o->scale->x, *(o->pos), *(o->rot), point);
    case SPHERE:
      return sphere(o->scale->x, *(o->pos), *(o->rot), point);
    case PLANE:
      return plane(o->scale->x, *(o->pos), *(o->rot), point);
    case LIGHT:
#ifdef DRAW_LIGHTS
      return sphere(o->scale->x, *(o->pos), *(o->rot), point);
#endif
      break;
    default:
      printf("Warning: Unrecognized object type with ID %c.\n", o->type);
      break;
  }
  return 0;
}

//////////////////////////////
// SCENE PARSING
//
int scene_parse(char *file, object **o, ray *camera){
  int num = 0, reflective;
  char type[32];
  FILE *fp = fopen(file, "r");
  vec3 pos, rot, scale;
  pixel pix;

  if(fp == NULL){
    return -1;
  }

  while(fscanf(fp, "%s %lf %lf %lf %lf %lf %lf %lf %lf %lf %i %i %i %i\n", type, &(pos.x), &(pos.y), &(pos.z), &(rot.x), &(rot.y), &(rot.z), &(scale.x), &(scale.y), &(scale.z), &reflective, &(pix.r), &(pix.g), &(pix.b)) != EOF){
    if(type[0] == VIEWPOINT){
      memcpy(&camera->origin, &pos, sizeof(vec3));
    } else {
      o[num] = malloc(sizeof(object));
      o[num]->type = type[0];
      o[num]->reflective = reflective;
      o[num]->pos = malloc(sizeof(vec3));
      o[num]->rot = malloc(sizeof(vec3));
      o[num]->scale = malloc(sizeof(vec3));
      o[num]->material = malloc(sizeof(pixel));

      memcpy(o[num]->pos, &pos, sizeof(vec3));
      memcpy(o[num]->rot, &rot, sizeof(vec3));
      memcpy(o[num]->scale, &scale, sizeof(vec3));
      memcpy(o[num]->material, &pix, sizeof(pixel));

      num++;
    }
  }

  fclose(fp);

  return num;
}

void scene_free(int num, object **o){
  int i;
  for(i=0;i<num;i++){
    free(o[i]->pos);
    free(o[i]->rot);
    free(o[i]->scale);
    free(o[i]->material);
    free(o[i]);
  }
  free(o);
}

//////////////////////////////
// VECTORS
//
void sub(vec3 a, vec3 b, vec3 *out){
  out->x = a.x - b.x;
  out->y = a.y - b.y;
  out->z = a.z - b.z;
}

void mult(double factor, vec3 a, vec3 *out){
  out->x = a.x * factor;
  out->y = a.y * factor;
  out->z = a.z * factor;
}

double dot(vec3 a, vec3 b){
  return (
    (a.x * b.x) +
    (a.y * b.y) +
    (a.z * b.z)
  );
}

void norm(vec3 a, vec3 *out){
  double max;

  if(a.x > a.y && a.x > a.z) { max = a.x; }
  if(a.y > a.x && a.y > a.z) { max = a.y; }
  if(a.z > a.x && a.z > a.y) { max = a.z; }

  out->x /= max;
  out->y /= max;
  out->z /= max;
}

//////////////////////////////
// RAYTRACING
//
int ray_walk(ray *r){
  r->step.z += STEP_SIZE;

  r->step.x =
    ((r->pass.x-r->origin.x)/(r->pass.z-r->origin.z)) /* dx/dz */
      * r->step.z;

  r->step.y =
    ((r->pass.y-r->origin.y)/(r->pass.z-r->origin.z)) /* dy/dz */
      * r->step.z;

  return (r->step.z <= MAX_DEPTH);
}

object *ray_trace(ray *r, object **scene, int n_objs){
  int i;

  r->step.z = 0;

  while(ray_walk(r)){
    for(i=0;i<n_objs;i++){
      if(intersect(scene[i], r->step)){
        return scene[i];
      }
    }
  }

  return NULL;
}

void ray_reflect(ray cast, vec3 norm, ray *out){
  out->origin.x = norm.x;
  out->origin.y = norm.y;
  out->origin.z = norm.z;

  out->pass.x = (cast.pass.x-2*dot(cast.pass, norm)*norm.x)*0.005f;
  out->pass.y = (cast.pass.y-2*dot(cast.pass, norm)*norm.y)*0.005f;
  out->pass.z = -cast.pass.z;
}

//////////////////////////////
// MAIN
//
int main(int argc, char **argv){
  int n_objs, i, j, k, lights_missed, lights_possible;
  pixel p;
  object **s = calloc(MAX_OBJS, sizeof(object*)),
         *hit,
         *testhit;
  ray r,
      testray;
  vec3 temp;
  struct winsize ws;

  if(argc < 2){
    printf("Usage: ansi_rt [scene file]\n");
    return 0;
  }

  if((n_objs=scene_parse(argv[1], s, &r)) == -1){
    printf("Failed to parse scene file.\n");
    return 1;
  }

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

  printf("\x1b[0;0H\x1b[?27l");
  for(r.pass.y=-HEIGHT/2;r.pass.y<HEIGHT/2;r.pass.y++){
    for(r.pass.x=-WIDTH/2;r.pass.x<WIDTH/2;r.pass.x++){
      if((hit=ray_trace(&r, s, n_objs)) == NULL){
        p.r = 75;
        p.g = 146;
        p.b = 176;
        goto finish_depthtest;
      }

      p.r = hit->material->r;
      p.g = hit->material->g;
      p.b = hit->material->b;

      lights_missed = 0;
      lights_possible = 1;

      testray.origin.x = r.step.x;
      testray.origin.y = r.step.y;
      testray.origin.z = r.step.z;
      for(j=0;j<n_objs;j++){
        if(s[j]->type == LIGHT){
          testray.step.z = 0;
          testray.pass.x = s[j]->pos->x;
          testray.pass.y = s[j]->pos->y;
          testray.pass.z = s[j]->pos->z;

          lights_possible++;

          if((testhit=ray_trace(&testray, s, n_objs)) != NULL &&
              testhit!=s[j] &&
              testhit!=hit){
            lights_missed++;
          }
        }
      }

      if(lights_missed != lights_possible){
        p.r += floor(50+(pow(150.0f-r.step.z, 3)/1000000.0f)*255.0f);
        p.g += floor(50+(pow(150.0f-r.step.z, 3)/1000000.0f)*255.0f);
        p.b += floor(50+(pow(150.0f-r.step.z, 3)/1000000.0f)*255.0f);
      } else {
        p.r = 0;
        p.g = 0;
        p.b = 0;
      }

      WRAP_PIXEL(p);

      if(hit->reflective){
        ray_reflect(r, r.step, &testray);
        testhit = ray_trace(&testray, s, n_objs);
        if(testhit != NULL){
/*          ray_reflect(testray, testray.step, &testray);
          testhit = ray_trace(&testray, s, n_objs);
          if(testhit != NULL){*/
          p.r += floor((pow(150.0f-testray.step.z, 3)/2000000.0f)*255.0f);
          p.g += floor((pow(150.0f-testray.step.z, 3)/2000000.0f)*255.0f);
          p.b += floor((pow(150.0f-testray.step.z, 3)/2000000.0f)*255.0f);

          p.r += testhit->material->r;
          p.g += testhit->material->g;
          p.b += testhit->material->b;

          WRAP_PIXEL(p);
          /*} else {
            p.r = 0;
            p.g = 200;
            p.b = 20;
          }*/
        }
      }

finish_depthtest:;
      printf("\x1b[48;2;%i;%i;%im  ", p.r, p.g, p.b);
    }
    printf("\r\n");
  }

  printf("\x1b[?27h\n");

  scene_free(n_objs, s);

  return 0;
}
