#ifndef _C_FILE_DEFINITION_H
#define _C_FILE_DEFINITION_H

#include <assimp/mesh.h>
#include <map>
#include <string>
#include <set>
#include <ostream>

#include "./ErrorCode.h"
#include "./ExtendedMesh.h"

class VertexBufferDefinition {
public:
    VertexBufferDefinition(ExtendedMesh* targetMesh, std::string name, VertexType vertexType);

    ExtendedMesh* mTargetMesh;
    std::string mName;
    VertexType mVertexType;

    ErrorCode Generate(std::ostream& output, float scale, aiQuaternion rotate);
private:
};

class CFileDefinition {
public:
    CFileDefinition(std::string prefix);
    int GetVertexBuffer(ExtendedMesh* mesh, VertexType vertexType);
    int GetCullingBuffer(const std::string& name, const aiVector3D& min, const aiVector3D& max);

    const std::string GetVertexBufferName(int vertexBufferID);
    std::string GetUniqueName(std::string requestedName);

    ErrorCode GenerateVertexBuffers(std::ostream& output, float scale, aiQuaternion rotate);
private:
    int GetNextID();

    std::string mPrefix;
    std::set<std::string> mUsedNames;
    std::map<int, VertexBufferDefinition> mVertexBuffers;
    int mNextID;
};

#endif