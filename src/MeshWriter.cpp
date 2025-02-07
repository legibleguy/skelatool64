#include "MeshWriter.h"

#include <set>
#include <sstream>

#include "RCPState.h"
#include "DisplayListGenerator.h"
#include "StringUtils.h"

MaterialCollector::MaterialCollector(): mSceneCount(0) {}

void MaterialCollector::UseMaterial(const std::string& material, DisplayListSettings& settings) {
    auto materialDL = settings.mMaterials.find(material);

    if (materialDL == settings.mMaterials.end()) {
        return;
    }

    auto prevCount = mMaterialUseCount.find(material);

    if (prevCount == mMaterialUseCount.end()) {
        mMaterialUseCount[material] = 1;

        for (auto resource = materialDL->second.mUsedResources.begin(); resource != materialDL->second.mUsedResources.end(); ++resource) {
            mUsedResources.insert(*resource);
        }
    } else {
        mMaterialUseCount[material] = prevCount->second + 1;
    }
}

void MaterialCollector::CollectMaterialResources(const aiScene* scene, std::vector<RenderChunk>& renderChunks, DisplayListSettings& settings) {
    for (auto chunk = renderChunks.begin(); chunk != renderChunks.end(); ++chunk) {
        std::string materialName = scene->mMaterials[chunk->mMesh->mMesh->mMaterialIndex]->GetName().C_Str();
        UseMaterial(materialName, settings);
    }
    ++mSceneCount;
}

void MaterialCollector::GenerateMaterials(CFileDefinition& fileDefinition, DisplayListSettings& settings, std::ostream& output) {
    std::vector<std::shared_ptr<MaterialResource>> resources(mUsedResources.begin(), mUsedResources.end());
    
    Material::WriteResources(resources, mResourceNameMapping, fileDefinition, output);
    for (auto useCount = mMaterialUseCount.begin(); useCount != mMaterialUseCount.end(); ++useCount) {
        if (useCount->second > 1 || mSceneCount > 1) {
            DisplayList materialDL(fileDefinition.GetUniqueName(useCount->first));
            settings.mMaterials.find(useCount->first)->second.WriteToDL(mResourceNameMapping, materialDL);
            mMaterialNameMapping[useCount->first] = materialDL.GetName();
            
            materialDL.Generate(fileDefinition, output);
        }
    }
}

void generateMeshIntoDLWithMaterials(const aiScene* scene, CFileDefinition& fileDefinition, MaterialCollector& materials, std::vector<RenderChunk>& renderChunks, DisplayListSettings& settings, DisplayList &displayList) {
    RCPState rcpState(settings.mVertexCacheSize, settings.mMaxMatrixDepth, settings.mCanPopMultipleMatrices);
    for (auto chunk = renderChunks.begin(); chunk != renderChunks.end(); ++chunk) {
        std::string materialName = scene->mMaterials[chunk->mMesh->mMesh->mMaterialIndex]->GetName().C_Str();
        displayList.AddCommand(std::unique_ptr<DisplayListCommand>(new CommentCommand("Material " + materialName)));
        auto mappedMaterialName = materials.mMaterialNameMapping.find(materialName);

        if (mappedMaterialName != materials.mMaterialNameMapping.end()) {
            displayList.AddCommand(std::unique_ptr<DisplayListCommand>(new CallDisplayListByNameCommand(mappedMaterialName->second)));
        } else {
            auto material = settings.mMaterials.find(materialName);

            if (material != settings.mMaterials.end()) {
                material->second.WriteToDL(materials.mResourceNameMapping, displayList);
            }
        }

        displayList.AddCommand(std::unique_ptr<DisplayListCommand>(new CommentCommand("End Material " + materialName)));
        
        int vertexBuffer = fileDefinition.GetVertexBuffer(chunk->mMesh, chunk->mVertexType);
        generateGeometry(*chunk, rcpState, vertexBuffer, displayList, settings.mHasTri2);
    }
    rcpState.TraverseToBone(nullptr, displayList);
}


void generateMeshIntoDL(const aiScene* scene, CFileDefinition& fileDefinition, std::vector<RenderChunk>& renderChunks, DisplayListSettings& settings, DisplayList &displayList, std::ostream& output) {
    MaterialCollector materials;

    materials.CollectMaterialResources(scene, renderChunks, settings);
    materials.GenerateMaterials(fileDefinition, settings, output);

    generateMeshIntoDLWithMaterials(scene, fileDefinition, materials, renderChunks, settings, displayList);

    fileDefinition.GenerateVertexBuffers(output, settings.mScale, settings.mRotateModel);
}

std::string generateMesh(const aiScene* scene, CFileDefinition& fileDefinition, std::vector<RenderChunk>& renderChunks, DisplayListSettings& settings, std::ostream& output) {
    DisplayList displayList(fileDefinition.GetUniqueName("model_gfx"));
    generateMeshIntoDL(scene, fileDefinition, renderChunks, settings, displayList, output);
    displayList.Generate(fileDefinition, output);
    return displayList.GetName();
}


void generateWireframeIntoDL(const aiScene* scene, CFileDefinition& fileDefinition, std::vector<RenderChunk>& renderChunks, DisplayListSettings& settings, DisplayList &displayList) {
    
}