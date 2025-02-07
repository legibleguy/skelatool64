
#include <assimp/mesh.h>

#include <iostream>
#include <fstream>

#include "src/SceneWriter.h"
#include "src/CommandLineParser.h"
#include "src/MaterialParser.h"
#include "src/LevelWriter.h"
#include "src/ThemeDefinition.h"
#include "src/ThemeWriter.h"
#include "src/SceneLoader.h"

bool parseMaterials(const std::string& filename, DisplayListSettings& output) {
    std::fstream file(filename, std::ios::in);

    struct ParseResult parseResult;
    parseMaterialFile(file, parseResult);
    output.mMaterials.insert(parseResult.mMaterialFile.mMaterials.begin(), parseResult.mMaterialFile.mMaterials.end());

    return true;
}

bool getVectorByName(const aiScene* scene, const std::string name, aiVector3D& result) {
    int axis;
    if (!scene->mMetaData->Get(name, axis)) {
        return false;
    }

    switch (axis) {
        case 0: result = aiVector3D(1.0f, 0.0f, 0.0f); break;
        case 1: result = aiVector3D(0.0f, 1.0f, 0.0f); break;
        case 2: result = aiVector3D(0.0f, 0.0f, 1.0f); break;
        default: return false;
    }

    int upSign;
    if (scene->mMetaData->Get(name + "Sign", upSign)) {
        if (upSign < 0) {
            result = -result;
        }
    }

    return true;
}

float angleBetween(const aiVector3D& a, const aiVector3D& b) {
    return acos(a * b / (a - b).SquareLength());
}

aiQuaternion getUpRotation(const aiVector3D& euler) {
    // aiVector3D upVector(0.0f, 0.0f, 0.0f);
    // aiVector3D frontVector(0.0f, 0.0f, 0.0f);
    // aiVector3D originalUpVector(0.0f, 0.0f, 1.0f);
    // aiVector3D originalFrontVector(0.0f, 1.0f, 0.0f);

    // aiQuaternion result;

    // if (getVectorByName(scene, "UpAxis", upVector)) {
    //     getVectorByName(scene, "OriginalUpAxis", originalUpVector);

    //     aiVector3D modifiedUp = result.Rotate(originalUpVector);

    //     aiVector3D upRotateVector = modifiedUp ^ upVector;
    //     float vectorAngles = angleBetween(originalFrontVector, frontVector);

    //     if (upRotateVector.SquareLength() > 0.01f) {
    //         result = aiQuaternion(upRotateVector, vectorAngles) * result;
    //     } else if (vectorAngles < -0.5f) {
    //         result = aiQuaternion(M_PI, 0.0f, 0.0f) * result;
    //     }
    // }

    // if (getVectorByName(scene, "FrontAxis", frontVector)) {
    //     getVectorByName(scene, "OriginalFrontAxis", frontVector);

    //     aiVector3D forwardRotateVector = originalFrontVector ^ frontVector;
    //     float vectorAngles = angleBetween(originalFrontVector, frontVector);
    //     aiQuaternion result;
        
    //     if (forwardRotateVector.SquareLength() > 0.01f) {
    //         result = aiQuaternion(forwardRotateVector, vectorAngles) * result;
    //     } else if (vectorAngles < -0.5f) {/*  */
    //         result = aiQuaternion(M_PI, 0.0f, 0.0f) * result;
    //     }
    // }

    // return result;

    // return aiQuaternion(aiVector3D(1.0f, 0.0f, 0.0f), -M_PI * 0.5f) * aiQuaternion(aiVector3D(0.0f, 0.0f, 1.0f), M_PI);

    return aiQuaternion(euler.y * M_PI / 180.0f, euler.z * M_PI / 180.0f, euler.x * M_PI / 180.0f);
}

void generateLevelDef(const std::string& filename, DisplayListSettings& settings) {
    ThemeDefinitionList themeDef;
    parseThemeDefinition(filename, themeDef);

    for (auto it = themeDef.mThemes.begin(); it != themeDef.mThemes.end(); ++it) {
        std::cout << "Generating theme " << it->mName << std::endl;
        generateThemeDefiniton(*it, settings);
    }

    std::ofstream levelList;
    levelList.open(themeDef.mLevelList, std::ios_base::out | std::ios_base::trunc);

    std::ofstream themeList;
    themeList.open(themeDef.mThemeList, std::ios_base::out | std::ios_base::trunc);

    for (auto theme = themeDef.mThemes.begin(); theme != themeDef.mThemes.end(); ++theme) {
        themeList << "DEFINE_THEME(" << theme->mCName << ")" << std::endl;
        for (auto level = theme->mLevels.begin(); level != theme->mLevels.end(); ++level) {
            levelList << "DEFINE_LEVEL(\"";
            levelList << level->mName << "\", ";
            levelList << level->mCName << ", ";
            levelList << theme->mCName << ", ";
            levelList << level->mMaxPlayers << ", ";
            levelList << 0;
            if (level->mFlags.size()) {
                for (auto it = level->mFlags.begin(); it != level->mFlags.end(); ++it) {
                    levelList << " | " << *it;
                }
            }
            levelList << ")" << std::endl;
        }
    }

    levelList.close();
    themeList.close();
}

/**
 * F3DEX2 - 32 vertices in buffer
 * F3D - 16 vetcies in buffer
 */

int main(int argc, char *argv[]) {
    CommandLineArguments args;

    if (!parseCommandLineArguments(argc, argv, args)) {
        return 1;
    }

    DisplayListSettings settings = DisplayListSettings();

    settings.mScale = args.mScale;
    settings.mRotateModel = getUpRotation(args.mEulerAngles);
    settings.mPrefix = args.mPrefix;
    settings.mExportAnimation = args.mExportAnimation;
    settings.mExportGeometry = args.mExportGeometry;

    bool hasError = false;

    for (auto materialFile = args.mMaterialFiles.begin(); materialFile != args.mMaterialFiles.end(); ++materialFile) {
        if (!parseMaterials(*materialFile, settings)) {
            hasError = true;
        }
    }

    if (hasError) {
        return 1;
    }

    if (args.mIsLevelDef) {
        std::cout << "Generating from level def "  << args.mInputFile << std::endl;
        generateLevelDef(args.mInputFile, settings);
    } else if (args.mIsLevel) {
        std::cout << "Generating from level "  << args.mInputFile << std::endl;
        const aiScene* scene = loadScene(args.mInputFile, args.mIsLevel, settings.mVertexCacheSize);

        if (!scene) {
            return 1;
        }
        std::cout << "Saving to "  << args.mOutputFile << std::endl;
        generateLevelFromSceneToFile(scene, args.mOutputFile, nullptr, settings);
    } else {
        std::cout << "Generating from mesh "  << args.mInputFile << std::endl;
        const aiScene* scene = loadScene(args.mInputFile, args.mIsLevel, settings.mVertexCacheSize);

        if (!scene) {
            return 1;
        }
        std::cout << "Saving to "  << args.mOutputFile << std::endl;
        generateMeshFromSceneToFile(scene, args.mOutputFile, settings);
    }
    
    return 0;
}