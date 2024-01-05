#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "ShapeSkin.h"
#include "GLSL.h"
#include "Program.h"
#include "TextureMatrix.h"

using namespace std;
using namespace glm;

ShapeSkin::ShapeSkin() :
	prog(NULL),
	elemBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0)
{
	T = make_shared<TextureMatrix>();
}

ShapeSkin::~ShapeSkin()
{
}

void ShapeSkin::setTextureMatrixType(const std::string &meshName)
{
	T->setType(meshName);
}

void ShapeSkin::loadMesh(const string &meshName)
{
	// Load geometry
	// This works only if the OBJ file has the same indices for v/n/t.
	// In other words, the 'f' lines must look like:
	// f 70/70/70 41/41/41 67/67/67
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		posBuf = attrib.vertices;
		norBuf = attrib.normals;
		texBuf = attrib.texcoords;
		assert(posBuf.size() == norBuf.size());
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			const tinyobj::mesh_t &mesh = shapes[s].mesh;
			size_t index_offset = 0;
			for(size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
				size_t fv = mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = mesh.indices[index_offset + v];
					elemBuf.push_back(idx.vertex_index);
				}
				index_offset += fv;
				// per-face material (IGNORE)
				//shapes[s].mesh.material_ids[f];
			}
		}
	}
}

void ShapeSkin::loadAttachment(const std::string &filename)
{
	// TODO
    ifstream in;
    in.open(filename);
    if(!in.good()) {
        cout << "Cannot read " << filename << endl;
        return;
    }
    cout << "Loading " << filename << endl;
    
    string line;
    while(1) {
        getline(in, line);
        if(in.eof()) {
            break;
        }
        if(line.empty()) {
            continue;
        }
        // Skip comments
        if(line.at(0) == '#') {
            continue;
        }
        // Parse lines
        stringstream ss(line);
        if(line.length()<11){
            ss>>vertNum>>boneNum>>maxInf;
            continue;
        }
        float lineIdx;
        ss>>lineIdx;
        for(int i=0; i<maxInf; i++){
            if(i<lineIdx){
                int bone;
                float infl;
                ss>>bone>>infl;
                indBuf.emplace_back(bone);
                wgtBuf.emplace_back(infl);
            }
            else{
                indBuf.emplace_back(0);
                wgtBuf.emplace_back(0);
            }
        }
        
    }
    in.close();
}

void ShapeSkin::init()
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	
	// Send the texcoord array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	
	// Send the element array to the GPU
	glGenBuffers(1, &elemBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::update(int k, vector<glm::mat4> bindPoseN, vector<vector<mat4> > animTransform)
{
	// TODO: CPU skinning calculations.
	// After computing the new positions and normals, send the new data
	// to the GPU by copying and pasting the relevant code from the
	// init() function.
    

    vector<mat4> mjkm;
    vector<float> posTBuf;
    vector<float> norTBuf;
    for(int j=0;j<boneNum;j++){
        mjkm.emplace_back(animTransform[k][j]*bindPoseN[j]);
    }

    for (int i=0; i<vertNum; i++){

        vec4 ipos(posBuf[i*3],posBuf[i*3+1],posBuf[i*3+2],1);//homo coord:1 apply the translation when multiplying matrix
        vec4 resultPos(0,0,0,0);
        vec4 inor(norBuf[i*3],norBuf[i*3+1],norBuf[i*3+2],0);//homo coord:0 do not apply the translation when multiplying matrix
        vec4 resultNorm(0,0,0,0);
        for(int j=0;j<maxInf;j++){
            int nj=j+i*maxInf;
            int jbone=indBuf[nj];
            float wij=wgtBuf[nj];
            resultPos+=wij*mjkm[jbone]*ipos;
            resultNorm+=wij*mjkm[jbone]*inor;
        }
        posTBuf.emplace_back(resultPos.x);
        posTBuf.emplace_back(resultPos.y);
        posTBuf.emplace_back(resultPos.z);
        norTBuf.emplace_back(resultNorm.x);
        norTBuf.emplace_back(resultNorm.y);
        norTBuf.emplace_back(resultNorm.z);
    }
    
//     Send the position array to the GPU
    glGenBuffers(1, &posBufID);
    glBindBuffer(GL_ARRAY_BUFFER, posBufID);
    glBufferData(GL_ARRAY_BUFFER, posTBuf.size()*sizeof(float), &posTBuf[0], GL_DYNAMIC_DRAW);
    
    // Send the normal array to the GPU
    glGenBuffers(1, &norBufID);
    glBindBuffer(GL_ARRAY_BUFFER, norBufID);
    glBufferData(GL_ARRAY_BUFFER, norTBuf.size()*sizeof(float), &norTBuf[0], GL_DYNAMIC_DRAW);
    
    // Send the texcoord array to the GPU
    glGenBuffers(1, &texBufID);
    glBindBuffer(GL_ARRAY_BUFFER, texBufID);
    glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
    
    // Send the element array to the GPU
    glGenBuffers(1, &elemBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
    
    // Unbind the arrays
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::updateDual(int k, vector<glm::mat4> bindPoseN, vector<vector<mat4> > animTransform)
{
    // TODO: Dual Quaternion Skinning.
    
    vector<mat4> mjkm;
    vector<pair<vec4,vec4> >Qjk;
    vector<float> posTBuf;
    vector<float> norTBuf;
    
    for(int j=0;j<boneNum;j++){
        mat4 m4=animTransform[k][j]*bindPoseN[j];
        mjkm.emplace_back(m4);
        quat q0=quat_cast(m4);
        vec3 t(m4[3].x,m4[3].y,m4[3].z);
        Qjk.emplace_back(make_pair(vec4(q0.w,q0.x,q0.y,q0.z), 
                                   vec4(
                                   -0.5*(t[0]*q0[1] + t[1]*q0[2] + t[2]*q0[3]),
                                    0.5*( t[0]*q0[0] + t[1]*q0[3] - t[2]*q0[2]),
                                    0.5*(-t[0]*q0[3] + t[1]*q0[0] + t[2]*q0[1]),
                                    0.5*( t[0]*q0[2] - t[1]*q0[1] + t[2]*q0[0]))
                                   ));
    }
    
    for (int i=0; i<vertNum; i++){

        vec4 ipos(posBuf[i*3],posBuf[i*3+1],posBuf[i*3+2],1);//homo coord:1
        
        vec4 inor(norBuf[i*3],norBuf[i*3+1],norBuf[i*3+2],0);//homo coord:0
        pair<vec4,vec4> Qik(vec4(0,0,0,0),vec4(0,0,0,0));
        for(int j=0;j<maxInf;j++){
            int nj=j+i*maxInf;
            int jbone=indBuf[nj];
            float wij=wgtBuf[nj];
            if(dot(Qjk[jbone].first,Qjk[indBuf[i*maxInf]].first)<0){
                Qjk[jbone].first*=-1;
                Qjk[jbone].second*=-1;
            }
            Qik.first+=wij*Qjk[jbone].first;
            Qik.second+=wij*Qjk[jbone].second;
        }
        float qnorm=length(Qik.first);
        Qik.first/=qnorm;
        Qik.second/=qnorm;
        mat4 m=mat4_cast(quat(Qik.first[0],Qik.first[1],Qik.first[2],Qik.first[3]));
        m[3][0]=2.0*(-Qik.second[0]*Qik.first[1]+Qik.second[1]*Qik.first[0]-Qik.second[2]*Qik.first[3]+Qik.second[3]*Qik.first[2]);
        m[3][1]=2.0*(-Qik.second[0]*Qik.first[2]+Qik.second[1]*Qik.first[3]+Qik.second[2]*Qik.first[0]-Qik.second[3]*Qik.first[1]);
        m[3][2]=2.0*(-Qik.second[0]*Qik.first[3]-Qik.second[1]*Qik.first[2]+Qik.second[2]*Qik.first[1]+Qik.second[3]*Qik.first[0]);
        vec4 resultPos=m*ipos;
        vec4 resultNorm=m*inor;
        posTBuf.emplace_back(resultPos.x);
        posTBuf.emplace_back(resultPos.y);
        posTBuf.emplace_back(resultPos.z);
        norTBuf.emplace_back(resultNorm.x);
        norTBuf.emplace_back(resultNorm.y);
        norTBuf.emplace_back(resultNorm.z);
    }
    
//     Send the position array to the GPU
    glGenBuffers(1, &posBufID);
    glBindBuffer(GL_ARRAY_BUFFER, posBufID);
    glBufferData(GL_ARRAY_BUFFER, posTBuf.size()*sizeof(float), &posTBuf[0], GL_DYNAMIC_DRAW);
    
    // Send the normal array to the GPU
    glGenBuffers(1, &norBufID);
    glBindBuffer(GL_ARRAY_BUFFER, norBufID);
    glBufferData(GL_ARRAY_BUFFER, norTBuf.size()*sizeof(float), &norTBuf[0], GL_DYNAMIC_DRAW);
    
    // Send the texcoord array to the GPU
    glGenBuffers(1, &texBufID);
    glBindBuffer(GL_ARRAY_BUFFER, texBufID);
    glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
    
    // Send the element array to the GPU
    glGenBuffers(1, &elemBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);
    
    // Unbind the arrays
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    
    GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::draw(int k) const
{
	assert(prog);

	// Send texture matrix
	glUniformMatrix3fv(prog->getUniform("T"), 1, GL_FALSE, glm::value_ptr(T->getMatrix()));
	
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
	
	// Draw
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glDrawElements(GL_TRIANGLES, (int)elemBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}
