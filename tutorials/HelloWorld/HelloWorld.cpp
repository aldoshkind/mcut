/***************************************************************************
 *
 *  Copyright (C) 2023 CutDigital Enterprise Ltd
 *  Licensed under the GNU GPL License, Version 3.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      https://www.gnu.org/licenses/gpl-3.0.html.
 *
 *  For your convenience, a copy of the License has been included in this
 *  repository.
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * HelloWorld.cpp
 *
 * \brief:
 *  Simple "hello world" program using MCUT.
 *
 * Author(s):
 *
 *    Floyd M. Chitalu    CutDigital Enterprise Ltd.
 *
 **************************************************************************/

#include "mcut/mcut.h"
#include "mio/mio.h"

#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <string>

#define my_assert(cond)                                                                            \
	if(!(cond))                                                                                    \
	{                                                                                              \
		fprintf(stderr, "MCUT error: %s\n", #cond);                                                \
		std::abort();                                                                              \
	}

McInt32 main()
{
    //
    // Create meshes to intersect
    //

    // the source-mesh (a cube)
    
    McFloat srcMeshVertices[] = {
        -5, -5, 5,  // vertex 0
        5, -5, 5,   // vertex 1
        5, 5, 5,    // vertex 2
        -5, 5, 5,   // vertex 3
        -5, -5, -5, // vertex 4
        5, -5, -5,  // vertex 5
        5, 5, -5,   // vertex 6
        -5, 5, -5   // vertex 7
    };

    McUint32 srcMeshFaces[] = {
        0, 1, 2, 3, // face 0
        7, 6, 5, 4, // face 1
        1, 5, 6, 2, // face 2
        0, 3, 7, 4, // face 3
        3, 2, 6, 7, // face 4
        4, 5, 1, 0  // face 5
    };
    
    McUint32 srcMeshFaceSizes[] = { 4, 4, 4, 4, 4, 4};

    McUint32 srcMeshVertexCount = 8;
    McUint32 srcMeshFaceCount = 6;

    // the cut mesh (a quad formed of two triangles)

    McFloat cutMeshVertices[] = {
        -20, -4, 0, // vertex 0
        0, 20, 20,  // vertex 1
        20, -4, 0,  // vertex 2
        0, 20, -20  // vertex 3
    };

    McUint32 cutMeshFaces[] = {
        0, 1, 2, // face 0
        0, 2, 3  // face 1
    };

    // McUint32 cutMeshFaceSizes[] = { 3, 3};
    
    McUint32 cutMeshVertexCount = 4;
    McUint32 cutMeshFaceCount = 2;

    //
    // create a context
    // 

    McContext context = MC_NULL_HANDLE;
    McResult status = mcCreateContext(&context, MC_NULL_HANDLE);

    my_assert (status == MC_NO_ERROR);

    //
    // do the cutting
    // 

    status = mcDispatch(
        context,
        MC_DISPATCH_VERTEX_ARRAY_FLOAT,
        srcMeshVertices,
        srcMeshFaces,
        srcMeshFaceSizes,
        srcMeshVertexCount,
        srcMeshFaceCount,
        cutMeshVertices,
        cutMeshFaces,
        nullptr, // cutMeshFaceSizes, // no need to give 'cutMeshFaceSizes' parameter since the cut-mesh is a triangle mesh
        cutMeshVertexCount,
        cutMeshFaceCount);

    my_assert (status == MC_NO_ERROR);

    //
    // query the number of available connected components after the cut
    // 

    McUint32 connectedComponentCount;
    std::vector<McConnectedComponent> connectedComponents;

    status = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_ALL, 0, NULL, &connectedComponentCount);

    my_assert (status == MC_NO_ERROR);

    if (connectedComponentCount == 0) {
        fprintf(stdout, "no connected components found\n");
        exit(EXIT_FAILURE);
    }

    connectedComponents.resize(connectedComponentCount); // allocate for the amount we want to get

    status = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_ALL, (McUint32)connectedComponents.size(), connectedComponents.data(), NULL);

    my_assert (status == MC_NO_ERROR);

    //
    //  query the data of each connected component 
    // 

    for (McInt32 i = 0; i < (McInt32)connectedComponents.size(); ++i) {
        
        McConnectedComponent cc = connectedComponents[i]; // connected compoenent id
        McSize numBytes = 0;

        //
        // vertices
        // 

        status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_VERTEX_FLOAT, 0, NULL, &numBytes);

        my_assert (status == MC_NO_ERROR);

        McUint32 ccVertexCount = (McUint32)(numBytes / (sizeof(McFloat) * 3ull));
        std::vector<McFloat> ccVertices(ccVertexCount * 3u);

        status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_VERTEX_FLOAT, numBytes, (McVoid*)ccVertices.data(), NULL);

        my_assert (status == MC_NO_ERROR);

        //
        // faces
        // 

        numBytes = 0; 

        status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_FACE, 0, NULL, &numBytes);

        my_assert (status == MC_NO_ERROR);

        std::vector<McUint32> ccFaceIndices;
        ccFaceIndices.resize(numBytes / sizeof(McUint32));

        status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_FACE, numBytes, (McVoid*)ccFaceIndices.data(), NULL);

        my_assert (status == MC_NO_ERROR);

        //
        // face sizes (vertices per face)
        // 

        numBytes = 0;

        status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_FACE_SIZE, 0, NULL, &numBytes);
        
        my_assert (status == MC_NO_ERROR);

        std::vector<McUint32> ccFaceSizes;
        ccFaceSizes.resize(numBytes / sizeof(McUint32));

        status = mcGetConnectedComponentData(context, cc, MC_CONNECTED_COMPONENT_DATA_FACE_SIZE, numBytes, (McVoid*)ccFaceSizes.data(), NULL);

        my_assert (status == MC_NO_ERROR);

        //
		// save connected component (mesh) to an .obj file
		// 

		char fnameBuf[64];
		sprintf(fnameBuf, "OUT_conncomp%d.obj", i);
		std::string fpath(OUTPUT_DIR "/" + std::string(fnameBuf));

        // "mioWriteOBJ" expects a vertex array of doubles. So we temporarilly create one.
        std::vector<McDouble> ccVertices64(ccVertices.size(), McDouble(0.0));

        for(McUint32 v =0; v < (McUint32)ccVertices.size(); ++v)
        {
            ccVertices64[v] = (McDouble)ccVertices[v];
        }

        mioWriteOBJ(
            fpath.c_str(), 
            ccVertices64.data(), 
            nullptr, // pNormals
            nullptr, // pTexCoords
            ccFaceSizes.data(), 
            ccFaceIndices.data(), 
            nullptr, // pFaceVertexTexCoordIndices
            nullptr, // pFaceVertexNormalIndices 
            ccVertexCount, 
            0, //numNormals 
            0, // numTexCoords
            (McUint32)ccFaceSizes.size());
    }

    //
    // free memory of _all_ connected components (could also free them individually inside above for-loop)
    // 

    status = mcReleaseConnectedComponents(context, 0, NULL);

    my_assert (status == MC_NO_ERROR);

    //
    // free memory of context
    // 
    
    status = mcReleaseContext(context);

    my_assert (status == MC_NO_ERROR);

    return 0;
}
