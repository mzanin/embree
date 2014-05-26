// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
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

#pragma once

#include "builders/parallel_builder.h"

#include "bvh4i/bvh4i.h"
#include "bvh4i/bvh4i_builder_util.h"
#include "bvh4i/bvh4i_builder_util_mic.h"
#include "bvh4i_statistics.h"

#define BVH_NODE_PREALLOC_FACTOR                 1.15f

namespace embree
{
  class BVH4iBuilder : public ParallelBuilderInterface
  {
    ALIGNED_CLASS;
  public:

    enum { 
      BVH4I_BUILDER_DEFAULT, 
      BVH4I_BUILDER_PRESPLITS, 
      BVH4I_BUILDER_VIRTUAL_GEOMETRY, 
      BVH4I_BUILDER_MEMORY_CONSERVATIVE
    };
 
    /*! Constructor. */
    BVH4iBuilder (BVH4i* bvh, BuildSource* source, void* geometry);
    virtual ~BVH4iBuilder();

    /*! creates the builder */
    static Builder* create (void* accel, BuildSource* source, void* geometry, size_t mode = BVH4I_BUILDER_DEFAULT);

    /* virtual function interface */
    virtual void build            (const size_t threadIndex, const size_t threadCount);
    virtual void allocateData     (const size_t threadCount, const size_t newNumPrimitives);
    virtual void computePrimRefs  (const size_t threadIndex, const size_t threadCount);
    virtual void createAccel      (const size_t threadIndex, const size_t threadCount);
    virtual void convertQBVHLayout(const size_t threadIndex, const size_t threadCount);

    virtual size_t getNumPrimitives();
    virtual void printBuilderName();

    virtual void buildSubTree(BuildRecord& current, 
			      NodeAllocator& alloc, 
			      const size_t mode,
			      const size_t threadID, 
			      const size_t numThreads);


  protected:

    void allocateMemoryPools(const size_t numPrims, 
			     const size_t numNodes,
			     const size_t sizeNodeInBytes  = sizeof(BVHNode),
			     const size_t sizeAccelInBytes = sizeof(Triangle1));

    void checkBuildRecord(const BuildRecord &current);
    void checkLeafNode(const BVHNode &node);


    TASK_FUNCTION(BVH4iBuilder,computePrimRefsTriangles);
    TASK_FUNCTION(BVH4iBuilder,createTriangle1Accel);
    TASK_FUNCTION(BVH4iBuilder,convertToSOALayout);
    TASK_FUNCTION(BVH4iBuilder,parallelBinningGlobal);
    TASK_FUNCTION(BVH4iBuilder,parallelPartitioningGlobal);
    TASK_RUN_FUNCTION(BVH4iBuilder,build_parallel);
    LOCAL_TASK_FUNCTION(BVH4iBuilder,parallelBinningLocal);
    LOCAL_TASK_FUNCTION(BVH4iBuilder,parallelPartitioningLocal);

  public:


    /*! splitting function that selects between sequential and parallel mode */
    bool split(BuildRecord& current, BuildRecord& left, BuildRecord& right, const size_t mode, const size_t threadID, const size_t numThreads);

    /*! perform sequential binning and splitting */
    bool splitSequential(BuildRecord& current, BuildRecord& leftChild, BuildRecord& rightChild);

    /*! perform parallel splitting */
    void parallelPartitioning(BuildRecord& current,
			      PrimRef * __restrict__ l_source,
			      PrimRef * __restrict__ r_source,
			      PrimRef * __restrict__ l_dest,
			      PrimRef * __restrict__ r_dest,
			      const Split &split,
			      Centroid_Scene_AABB &local_left,
			      Centroid_Scene_AABB &local_right);			      
			      
    /*! perform parallel binning and splitting using all threads on all cores*/
    bool splitParallelGlobal(BuildRecord& current, BuildRecord& leftChild, BuildRecord& rightChild, const size_t threadID, const size_t threads);

    /*! perform parallel binning and splitting using only the threads per core*/
    bool splitParallelLocal(BuildRecord& current, BuildRecord& leftChild, BuildRecord& rightChild, const size_t threadID);
    
    /*! creates a leaf node */
    void createLeaf(BuildRecord& current, NodeAllocator& alloc, const size_t threadIndex, const size_t threadCount);

    /*! select between recursion and stack operations */
    void recurse(BuildRecord& current, NodeAllocator& alloc, const size_t mode, const size_t threadID, const size_t numThreads);
    
    /*! recursive build function */
    void recurseSAH(BuildRecord& current, NodeAllocator& alloc, const size_t mode, const size_t threadID, const size_t numThreads);

  protected:
    BVH4i* bvh;                   //!< Output BVH


    /* work record handling */
  protected:

  public:

    /* shared structure for multi-threaded binning and partitioning */
    struct __aligned(64) SharedBinningPartitionData
    {
      __aligned(64) BuildRecord rec;
      __aligned(64) Centroid_Scene_AABB left;
      __aligned(64) Centroid_Scene_AABB right;
      __aligned(64) Split split;
      __aligned(64) AlignedAtomicCounter32 lCounter;
      __aligned(64) AlignedAtomicCounter32 rCounter;
    };

    /* single structure for all worker threads */
    __aligned(64) SharedBinningPartitionData global_sharedData;

    /* one 16-bins structure per thread */
    __aligned(64) Bin16 global_bin16[MAX_MIC_THREADS];

    /* one shared binning/partitoning structure per core */
    __aligned(64) SharedBinningPartitionData local_sharedData[MAX_MIC_CORES];

  protected:
    PrimRef*   prims;
    BVHNode*   node;
    Triangle1* accel;

    size_t numNodesToAllocate;
    size_t size_prims;



    /*! node allocation */
    __forceinline unsigned int allocNode(int size)
    {
      const unsigned int currentIndex = atomicID.add(size);
      if (unlikely(currentIndex >= numAllocatedNodes)) {
        FATAL("not enough nodes allocated");
      }
      return currentIndex;
    }

  };

  /*! derived binned-SAH builder supporting triangle pre-splits */  
  class BVH4iBuilderPreSplits : public BVH4iBuilder
  {
  public:

  BVH4iBuilderPreSplits(BVH4i* bvh, BuildSource* source, void* geometry) : BVH4iBuilder(bvh,source,geometry) 
      {
      }

    virtual void allocateData   (const size_t threadCount, const size_t newNumPrimitives);
    virtual void computePrimRefs(const size_t threadIndex, const size_t threadCount);
    virtual void printBuilderName();

  protected:
    size_t numMaxPrimitives;
    size_t numMaxPreSplits;

    __aligned(64) AlignedAtomicCounter32 dest0;
    __aligned(64) AlignedAtomicCounter32 dest1;

    static const size_t RADIX_BITS = 8;
    static const size_t RADIX_BUCKETS = (1 << RADIX_BITS);
    static const size_t RADIX_BUCKETS_MASK = (RADIX_BUCKETS-1);
    __aligned(64) unsigned int radixCount[MAX_MIC_THREADS][RADIX_BUCKETS];

    TASK_FUNCTION(BVH4iBuilderPreSplits,countAndComputePrimRefsPreSplits);
    TASK_FUNCTION(BVH4iBuilderPreSplits,radixSortPreSplitIDs);
    TASK_FUNCTION(BVH4iBuilderPreSplits,computePrimRefsFromPreSplitIDs);
    
  };


  /*! derived memory conservative binned-SAH builder */  
  class BVH4iBuilderMemoryConservative : public BVH4iBuilder
  {
  public:

  BVH4iBuilderMemoryConservative(BVH4i* bvh, BuildSource* source, void* geometry) : BVH4iBuilder(bvh,source,geometry) 
      {
      }

    virtual void allocateData   (const size_t threadCount, const size_t newNumPrimitives);
    virtual void printBuilderName();
    virtual void createAccel    (const size_t threadIndex, const size_t threadCount);   

  protected:
    TASK_FUNCTION(BVH4iBuilderMemoryConservative,createMemoryConservativeAccel);
 
  };



  /*! derived binned-SAH builder supporting virtual geometry */  
  class BVH4iBuilderVirtualGeometry : public BVH4iBuilder
  {
  public:
  BVH4iBuilderVirtualGeometry(BVH4i* bvh, BuildSource* source, void* geometry) : BVH4iBuilder(bvh,source,geometry) 
      {
      }

    virtual size_t getNumPrimitives();
    virtual void computePrimRefs(const size_t threadIndex, const size_t threadCount);
    virtual void createAccel    (const size_t threadIndex, const size_t threadCount);
    virtual void printBuilderName();

  protected:
    TASK_FUNCTION(BVH4iBuilderVirtualGeometry,computePrimRefsVirtualGeometry);
    TASK_FUNCTION(BVH4iBuilderVirtualGeometry,createVirtualGeometryAccel);
    
  };

}
