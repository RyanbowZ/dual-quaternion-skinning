#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "TextureMatrix.h"

using namespace std;
using namespace glm;

TextureMatrix::TextureMatrix()
{
	type = Type::NONE;
	T = mat3(1.0f);
}

TextureMatrix::~TextureMatrix()
{
	
}

void TextureMatrix::setType(const string &str)
{
	if(str.find("Body") != string::npos) {
		type = Type::BODY;
	} else if(str.find("Mouth") != string::npos) {
		type = Type::MOUTH;
	} else if(str.find("Eyes") != string::npos) {
		type = Type::EYES;
	} else if(str.find("Brows") != string::npos) {
		type = Type::BROWS;
	} else {
		type = Type::NONE;
	}
}

void TextureMatrix::update(unsigned int key)
{
	// Update T here
	if(type == Type::BODY) {
		// Do nothing
	} else if(type == Type::MOUTH) {
		// TODO
        if(key=='m'){
            T[2][0]+=0.1;
            if(T[2][0]>0.3)T[2][0]=0;
        }
        if(key=='M'){
            T[2][1]+=0.1;
            if(T[2][0]>1.0)T[2][0]=0;
        }
	} else if(type == Type::EYES) {
        if(key=='e'){
            T[2][0]+=0.2;
            if(T[2][0]>0.6)T[2][0]=0;
        }
        if(key=='E'){
            T[2][1]+=0.1;
            if(T[2][0]>1.0)T[2][0]=0;
        }
		
	} else if(type == Type::BROWS) {
		// TODO
        if(key=='b'){
            T[2][1]+=0.1;
            if(T[2][0]>1.0)T[2][0]=0;
        }
	}
}
