// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "instance_intersector1.h"
#include "../../common/scene.h"

namespace embree
{
  namespace isa
  {
    void InstanceBoundsFunction(void* userPtr, const Instance* instance, size_t item, BBox3fa* bounds_o) 
    {
      if (instance->numTimeSteps == 1) {
        bounds_o[0] = xfmBounds(instance->local2world[0],instance->object->bounds);
      } else {
        bounds_o[0] = xfmBounds(instance->local2world[0],instance->object->bounds);
        bounds_o[1] = xfmBounds(instance->local2world[1],instance->object->bounds);
      }
    }

    RTCBoundsFunc2 InstanceBoundsFunc = (RTCBoundsFunc2) InstanceBoundsFunction;

    void FastInstanceIntersector1::intersect(const Instance* instance, Ray& ray, size_t item)
    {
      const AffineSpace3fa world2local = instance->getWorld2LocalSpecial(ray.time);
      const Vec3fa ray_org = ray.org;
      const Vec3fa ray_dir = ray.dir;
      const int ray_geomID = ray.geomID;
      const int ray_instID = ray.instID;
      ray.org = xfmPoint (world2local,ray_org);
      ray.dir = xfmVector(world2local,ray_dir);
      ray.geomID = -1;
      ray.instID = instance->id;
      instance->object->intersect((RTCRay&)ray,nullptr);
      ray.org = ray_org;
      ray.dir = ray_dir;
      if (ray.geomID == -1) {
        ray.geomID = ray_geomID;
        ray.instID = ray_instID;
      }
    }
    
    void FastInstanceIntersector1::occluded (const Instance* instance, Ray& ray, size_t item)
    {
      const AffineSpace3fa world2local = instance->getWorld2LocalSpecial(ray.time);
      const Vec3fa ray_org = ray.org;
      const Vec3fa ray_dir = ray.dir;
      ray.org = xfmPoint (world2local,ray_org);
      ray.dir = xfmVector(world2local,ray_dir);
      ray.instID = instance->id;
      instance->object->occluded((RTCRay&)ray,nullptr);
      ray.org = ray_org;
      ray.dir = ray_dir;
    }
    
    DEFINE_SET_INTERSECTOR1(InstanceIntersector1,FastInstanceIntersector1);

    void FastInstanceIntersector1M::intersect(const Instance* instance, const RTCIntersectContext* context, Ray** rays, size_t M, size_t item)
    {
      assert(M<MAX_INTERNAL_STREAM_SIZE);
      Ray lrays[MAX_INTERNAL_STREAM_SIZE];
      AffineSpace3fa world2local = instance->getWorld2Local();

      for (size_t i=0; i<M; i++)
      {
        if (unlikely(instance->numTimeSteps != 1)) 
          world2local = instance->getWorld2Local(rays[i]->time);

        lrays[i].org = xfmPoint (world2local,rays[i]->org);
        lrays[i].dir = xfmVector(world2local,rays[i]->dir);
        lrays[i].tnear = rays[i]->tnear;
        lrays[i].tfar = rays[i]->tfar;
        lrays[i].time = rays[i]->time;
        lrays[i].mask = rays[i]->mask;
        lrays[i].geomID = -1;
        lrays[i].instID = instance->id;
      }

      rtcIntersect1M((RTCScene)instance->object,context,(RTCRay*)lrays,M,sizeof(Ray));
        
      for (size_t i=0; i<M; i++)
      {
        if (lrays[i].geomID == -1) continue;
        rays[i]->instID = lrays[i].instID;
        rays[i]->geomID = lrays[i].geomID;
        rays[i]->primID = lrays[i].primID;
        rays[i]->u = lrays[i].u;
        rays[i]->v = lrays[i].v;
        rays[i]->tfar = lrays[i].tfar;
        rays[i]->Ng = lrays[i].Ng;
      }
    }
    
    void FastInstanceIntersector1M::occluded (const Instance* instance, const RTCIntersectContext* context, Ray** rays, size_t M, size_t item)
    {
      assert(M<MAX_INTERNAL_STREAM_SIZE);
      Ray lrays[MAX_INTERNAL_STREAM_SIZE];
      AffineSpace3fa world2local = instance->getWorld2Local();
      
      for (size_t i=0; i<M; i++)
      {
        if (unlikely(instance->numTimeSteps != 1)) 
          world2local = instance->getWorld2Local(rays[i]->time);

        lrays[i].org = xfmPoint (world2local,rays[i]->org);
        lrays[i].dir = xfmVector(world2local,rays[i]->dir);
        lrays[i].tnear = rays[i]->tnear;
        lrays[i].tfar = rays[i]->tfar;
        lrays[i].time = rays[i]->time;
        lrays[i].mask = rays[i]->mask;
        lrays[i].geomID = -1;
        lrays[i].instID = instance->id;
      }

      rtcOccluded1M((RTCScene)instance->object,context,(RTCRay*)lrays,M,sizeof(Ray));
        
      for (size_t i=0; i<M; i++)
      {
        if (lrays[i].geomID == -1) continue;
        rays[i]->geomID = 0;
      }
    }

    DEFINE_SET_INTERSECTOR1M(InstanceIntersector1M,FastInstanceIntersector1M);
  }
}
