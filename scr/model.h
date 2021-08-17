#ifndef MODEL_H
#define MODEL_H

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"
#include "boundingbox.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>  
#include <vector>
#include <glm/glm.hpp>
#include <cmath>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <eigen3/Eigen/Core>
#include <cstdlib>
#include <ctime>
using namespace std;
using namespace Eigen;
unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);
#define M_PI 3.14159265358979323846


class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    Vector3f curPos = Vector3f(0.3087, -1.055, 0.1746);
    Vector3f aimPos = Vector3f(0.3087, -1.055, 0.1746);
    Vector3f curFront = Vector3f(-1, 0, 0);
    Vector3f aimFront = Vector3f(-1, 0, 0);
    Quaternionf curRotate = Quaternionf(1.0f, 0.0f, 0.0, 0.0f);
    Quaternionf aimRotate = Quaternionf(1.0f, 0.0f, 0.0f, 0.0f);
    clock_t startRoateTime = 0;
    bool isMoving = false;
    bool isRoating = false;
    bool isForwading = false;
    double rotateTime = 0;
    Vector3f c = Vector3f(-1, 0, 0);
    // constructor, expects a filepath to a 3D model.
    Model()
    {
    }
    Model(string const &path,  bool autoNormalize=true)
    {
        loadModel(path, autoNormalize);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path, bool auto_normalize=true)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
        if (auto_normalize)
        {
            normalizeMeshes();
        }
    }
    void processMove(const glm::fvec4& selected) {
        Vector3f aim = Vector3f(selected[0], selected[1], selected[2]);
        Vector3f front = (aim - curPos);
        if (front.norm() > 0.01 && isMoving == false) {
            isMoving = true;
            isForwading = true;
            aimPos = aim;
            float angle = acos(front.dot(curFront) / (front.norm() * curFront.norm()));
            if (angle > 3 * M_PI / 180) {
                isRoating = true;
                startRoateTime = clock();
                Quaternionf tmp;
                tmp = tmp.setFromTwoVectors(curFront, front);
                tmp.normalize();
                aimRotate = tmp * curRotate;
                aimRotate.normalize();
                curFront = tmp.toRotationMatrix() * curFront;
                rotateTime = 1;
            }
            

        }
    }
    glm::mat4 getRotateMat() {
        glm::quat ans;
        if (isMoving && isRoating) {
            clock_t curRotateTime = clock();
            double rotateRate = (double)(curRotateTime - startRoateTime) / (CLOCKS_PER_SEC *  rotateTime);
            if (rotateRate > 1.0) {
                curRotate = curRotate.slerp(1.0, aimRotate);
                curRotate.normalize();
                ans = glm::quat(curRotate.w(), curRotate.x(), curRotate.y(), curRotate.z());
                isRoating = false;
                //cout << "curFront= " << curFront[0] << ' ' << curFront[1] << ' ' << curFront[2] << endl << endl;;
            }
            else {

                Quaternionf qt = curRotate.slerp(rotateRate, aimRotate);
                Quaternionf tmp = qt ;
                tmp.normalize();
                ans = glm::quat(tmp.w(), tmp.x(), tmp.y(), tmp.z());
            } 
        }
        else {
            ans = glm::quat(curRotate.w(), curRotate.x(), curRotate.y(), curRotate.z());
        }
        return glm::mat4_cast(ans);
    }
    glm::mat4 getTransformMat() {
        glm::mat4 ans;
        if (isMoving &&isRoating == false && isForwading) {
            clock_t curRotateTime = clock();
            double forwardRate = (double)(curRotateTime - startRoateTime) / (CLOCKS_PER_SEC * rotateTime) - 1.0;
            if (forwardRate > 1.0) {
                curPos = aimPos;
                ans = glm::translate(glm::mat4(1.0f), glm::fvec3(curPos[0], curPos[1], curPos[2]));
                isMoving = false;
                isForwading = false;
                return ans;
            }
            else {
                Vector3f tmp = forwardRate * (aimPos - curPos) + curPos;
                ans = glm::translate(glm::mat4(1.0f), glm::fvec3(tmp[0], tmp[1], tmp[2]));
                return ans;
            }
        }
        else {
            ans = glm::translate(glm::mat4(1.0f), glm::fvec3(curPos[0], curPos[1], curPos[2]));
        }
        return ans;
    }
private:

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        BoundingBox box;
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }
    void normalizeMeshes()
    {
        BoundingBox box;
        for (auto &m : meshes)
        {
            box.AddBoundingBox(getMeshBBox(m));
        }
        for (auto& m : meshes)
        {
            for (auto &p : m.vertices)
            {
                p.Position = (p.Position - box.center()) / box.radius();
            }
        }
    }
    BoundingBox getMeshBBox(Mesh &mesh)
    {
        BoundingBox box;
        for (auto &p:mesh.vertices)
        {
            box.AddPoint3(p.Position);
        }
        return box;
    }
    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};


inline unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
#endif
