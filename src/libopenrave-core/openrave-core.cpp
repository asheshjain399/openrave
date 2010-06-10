// Copyright (C) 2006-2009 Rosen Diankov (rdiankov@cs.cmu.edu)
//
// This file is part of OpenRAVE.
// OpenRAVE is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include "ravep.h"

namespace OpenRAVE {
    EnvironmentBasePtr CreateEnvironment(bool bLoadAllPlugins) {
        srand(GetMilliTime());
        RaveInitRandomGeneration(GetMilliTime());

        boost::shared_ptr<Environment> p(new Environment(bLoadAllPlugins));
        p->Init();
        return p;
    }
}

#include <streambuf>

bool ParseDirectories(const char* pdirs, std::vector<std::string>& vdirs)
{
    vdirs.resize(0);

    if( pdirs == NULL )
        return false;

    // search for all directories separated by ':'
    string tmp = pdirs;
    std::string::size_type pos = 0, newpos=0;
    while( pos < tmp.size() ) {

#ifdef _WIN32
        newpos = tmp.find(';', pos);
#else
		newpos = tmp.find(':', pos);
#endif

        std::string::size_type n = newpos == std::string::npos ? tmp.size()-pos : (newpos-pos);
        vdirs.push_back(tmp.substr(pos, n));

        if( newpos == std::string::npos )
            break;
            
        pos = newpos+1;
    }

    return true;
}

#ifdef OPENRAVE_COIN3D

#include <Inventor/SbMatrix.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/nodes/SoShape.h>

// Coin specific routines
static SbMatrix s_ModelMatrix;
static void _TriangulateCB(void *data, SoCallbackAction *action,
	      const SoPrimitiveVertex *vertex1,
	      const SoPrimitiveVertex *vertex2,
	      const SoPrimitiveVertex *vertex3)
{
    KinBody::Link::TRIMESH* ptri = (KinBody::Link::TRIMESH*)data;
    s_ModelMatrix = action->getModelMatrix();

    // set the vertices (SCALED)
//    ptri->vertices.push_back(Vector(&vertex1->getPoint()[0]));
//    ptri->vertices.push_back(Vector(&vertex2->getPoint()[0]));
//    ptri->vertices.push_back(Vector(&vertex3->getPoint()[0]));
    SbVec3f v;
    s_ModelMatrix.multVecMatrix(vertex1->getPoint(), v);
    ptri->vertices.push_back(Vector(&v[0]));

    s_ModelMatrix.multVecMatrix(vertex2->getPoint(), v);
    ptri->vertices.push_back(Vector(&v[0]));

    s_ModelMatrix.multVecMatrix(vertex3->getPoint(), v);
    ptri->vertices.push_back(Vector(&v[0]));
}

void CreateTriMeshData(SoNode* pnode, KinBody::Link::TRIMESH& tri)
{
    tri.vertices.resize(0);
    tri.vertices.reserve(256);

    // create the collision model and triangulate
    SoCallbackAction triAction;

    // add the callbacks for all nodes
    triAction.addTriangleCallback(SoShape::getClassTypeId(), _TriangulateCB, &tri);
    pnode->ref();
    triAction.apply(pnode);
    //pnode->unref();

    Vector scale;
    scale.x = sqrtf(s_ModelMatrix[0][0]*s_ModelMatrix[0][0]+s_ModelMatrix[1][0]*s_ModelMatrix[1][0]+s_ModelMatrix[2][0]*s_ModelMatrix[2][0]);
    scale.y = sqrtf(s_ModelMatrix[0][1]*s_ModelMatrix[0][1]+s_ModelMatrix[1][1]*s_ModelMatrix[1][1]+s_ModelMatrix[2][1]*s_ModelMatrix[2][1]);
    scale.z = sqrtf(s_ModelMatrix[0][2]*s_ModelMatrix[0][2]+s_ModelMatrix[1][2]*s_ModelMatrix[1][2]+s_ModelMatrix[2][2]*s_ModelMatrix[2][2]);

    tri.indices.resize(tri.vertices.size());
    for(size_t i = 0; i < tri.vertices.size(); ++i) {
        tri.indices[i] = i;
        tri.vertices[i].x *= scale.x;
        tri.vertices[i].y *= scale.y;
        tri.vertices[i].z *= scale.z;
    }
}

#endif

#ifdef OPENRAVE_COLLADA_SUPPORT
#include "colladareader.h"
#include "colladawriter.h"

bool RaveParseColladaFile(EnvironmentBasePtr penv, const string& filename)
{
    ColladaReader reader(penv);
    if (!reader.InitFromFile(filename))
        return false;
    return reader.Extract(penv);
}

bool RaveParseColladaFile(EnvironmentBasePtr penv, KinBodyPtr& pbody, const string& filename)
{
    ColladaReader reader(penv);
    if (!reader.InitFromFile(filename))
        return false;
    return reader.Extract(pbody);
}

bool RaveParseColladaFile(EnvironmentBasePtr penv, RobotBasePtr& probot, const string& filename)
{
    ColladaReader reader(penv);
    if (!reader.InitFromFile(filename))
        return false;
    return reader.Extract(probot);
}

bool RaveParseColladaData(EnvironmentBasePtr penv, const string& pdata) {
    ColladaReader reader(penv);
    if (!reader.InitFromData(pdata))
        return false;
    return reader.Extract(penv);
}

bool RaveParseColladaData(EnvironmentBasePtr penv, KinBodyPtr& pbody, const string& pdata)
{
    ColladaReader reader(penv);
    if (!reader.InitFromData(pdata))
        return false;
    return reader.Extract(pbody);
}

bool RaveParseColladaData(EnvironmentBasePtr penv, RobotBasePtr& probot, const string& pdata)
{
    ColladaReader reader(penv);
    if (!reader.InitFromData(pdata))
        return false;
    return reader.Extract(probot);
}

bool RaveWriteColladaFile(EnvironmentBasePtr penv, const string& filename)
{
    RAVELOG_VERBOSEA("bool RaveWriteColladaFile(EnvironmentBasePtr penv, const string& filename)\n");

    ColladaWriter writer(penv);

    if( !writer.Write(penv) )
    {
        return false;
    }

    return writer.Save(filename);
}

bool RaveWriteColladaFile(KinBodyPtr pbody, const string& filename)
{
    RAVELOG_VERBOSEA("bool RaveWriteColladaFile(KinBodyPtr pbody, const string& filename)\n");

    ColladaWriter writer(pbody->GetEnv());

    if( !writer.Write(pbody) )
    {
        return false;
    }

    return writer.Save(filename);
}

bool RaveWriteColladaFile(RobotBasePtr probot, const string& filename)
{
    RAVELOG_VERBOSEA("bool RaveWriteColladaFile(RobotBasePtr probot, const string& filename)\n");

    ColladaWriter writer(probot->GetEnv());

    if( !writer.Write(probot) )
    {
        return false;
    }

    return writer.Save(filename);
}

#else

bool RaveParseColladaFile(EnvironmentBasePtr penv, const string& filename)
{
    RAVELOG_ERRORA("collada files not supported\n");
    return false;
}

bool RaveParseColladaFile(EnvironmentBasePtr penv, KinBodyPtr& pbody, const string& filename)
{
    RAVELOG_ERRORA("collada files not supported\n");
    return false;
}

bool RaveParseColladaFile(EnvironmentBasePtr penv, RobotBasePtr& probot, const string& filename)
{
    RAVELOG_ERRORA("collada files not supported\n");
    return false;
}

bool RaveParseColladaData(EnvironmentBasePtr penv, const string& pdata)
{
    RAVELOG_ERRORA("collada files not supported\n");
    return false;
}

bool RaveParseColladaData(EnvironmentBasePtr penv, KinBodyPtr& pbody, const string& pdata)
{
    RAVELOG_ERRORA("collada files not supported\n");
    return false;
}

bool RaveParseColladaData(EnvironmentBasePtr penv, RobotBasePtr& probot, const string& pdata)
{
    RAVELOG_ERRORA("collada files not supported\n");
    return false;
}


bool RaveWriteColladaFile(EnvironmentBasePtr penv, const string& filename)
{
    RAVELOG_ERRORA("collada files not supported\n");
    return false;
}

bool RaveWriteColladaFile(KinBodyPtr pbody, const string& filename)
{
    RAVELOG_ERRORA("collada files not supported\n");
    return false;
}

bool RaveWriteColladaFile(RobotBasePtr probot, const string& filename)
{
    RAVELOG_ERRORA("collada files not supported\n");
    return false;
}

#endif
