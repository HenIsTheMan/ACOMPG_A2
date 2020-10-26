#include "ShaderChief.h"

uint ShaderChief::currID = 0;

ShaderChief::ShaderChief(cstr vsPath, cstr fsPath, cstr gsPath): shaderProgID(999){
	shaderFilePaths[0] = vsPath;
	shaderFilePaths[1] = fsPath;
	shaderFilePaths[2] = gsPath;
}

ShaderChief::~ShaderChief(){
	glDeleteProgram(shaderProgID);
}

void ShaderChief::Init(){
	shaderProgID = glCreateProgram();
	std::vector<uint> shaderRefIDs{glCreateShader(GL_VERTEX_SHADER), glCreateShader(GL_FRAGMENT_SHADER)};
	if(shaderFilePaths[2] != ""){
		shaderRefIDs.emplace_back(glCreateShader(GL_GEOMETRY_SHADER)); //Takes set of vertices that form a primitive as input (so its input data from the vertex shader is always represented as arrays of vertex data even though we only have a single vertex right now??)
	} //Shapes are dynamically generated on the GPU with geometry shaders (better than defining shapes within vertex buffers) so good for simple repeating forms like cubes in a voxel world or grass in a field
	for(short i = 0; i < shaderRefIDs.size(); ++i){
		ParseShader(shaderFilePaths[i], shaderRefIDs[i]);
		glAttachShader(shaderProgID, shaderRefIDs[i]);
	}
	LinkProg();
	for(const auto& shaderRefID: shaderRefIDs){
		glDeleteShader(shaderRefID);
	}
}

void ShaderChief::ParseShader(cstr fPath, uint shaderID) const{
	int infoLogLength;
	str srcCodeStr, line;
	std::ifstream stream(fPath);

	if(!stream.is_open()){
		printf("Failed to open and read \"%s\"\n", fPath);
		return;
	}
	while(getline(stream, line)){
		srcCodeStr += "\n" + line;
	}
	stream.close();

	printf("Compiling \"%s\"...\n", fPath);
	cstr srcCodeCStr = srcCodeStr.c_str();
	glShaderSource(shaderID, 1, &srcCodeCStr, 0);
	glCompileShader(shaderID);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if(infoLogLength){
		char* errorMsg = (char*)_malloca(infoLogLength * sizeof(char)); //Allocate memory on the stack dynamically
		glGetShaderInfoLog(shaderID, infoLogLength, &infoLogLength, errorMsg);
		printf("Failed to compile \"%s\"!\n%s\n", fPath, errorMsg);
	}
}

void ShaderChief::LinkProg() const{
	int infoLogLength;
	printf("Linking programme...\n\n");
	glLinkProgram(shaderProgID); //Vars in diff shaders are linked here too

	glGetProgramiv(shaderProgID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if(infoLogLength){
		char* errorMsg = (char*)_malloca(infoLogLength * sizeof(char)); //Allocate memory on the stack dynamically
		glGetProgramInfoLog(shaderProgID, infoLogLength, &infoLogLength, errorMsg);
		printf("%s\n", errorMsg);
	}
}

void ShaderChief::SetUni1f(cstr uniName, float value, bool warn){
	int uniLocation = glGetUniformLocation(currID, uniName); //Query location of uniform
	if(uniLocation != -1){
		glUniform1f(uniLocation, value); //Sets uniform on the currently active shader prog
	} else if(warn){
		printf("%s: Failed to find '%s'\n", std::to_string(currID).c_str(), uniName);
	}
}

void ShaderChief::SetUniMtx4fv(cstr uniName, float* address, bool warn){
	int uniLocation = glGetUniformLocation(currID, uniName); //Query location of uniform
	if(uniLocation != -1){
		glUniformMatrix4fv(uniLocation, 1, GL_FALSE, address); //Sets uniform on the currently active shader prog
	} else if(warn){
		printf("%s: Failed to find '%s'\n", std::to_string(currID).c_str(), uniName);
	}
}

void ShaderChief::SetUni2f(cstr uniName, float value1, float value2, bool warn){
	int uniLocation = glGetUniformLocation(currID, uniName); //Query location of uniform
	if(uniLocation != -1){
		glUniform2f(uniLocation, value1, value2); //Sets uniform on the currently active shader prog
	} else if(warn){
		printf("%s: Failed to find '%s'\n", std::to_string(currID).c_str(), uniName);
	}
}

void ShaderChief::SetUni3f(cstr uniName, float value1, float value2, float value3, bool warn){
	int uniLocation = glGetUniformLocation(currID, uniName); //Query location of uniform
	if(uniLocation != -1){
		glUniform3f(uniLocation, value1, value2, value3); //Sets uniform on the currently active shader prog
	} else if(warn){
		printf("%s: Failed to find '%s'\n", std::to_string(currID).c_str(), uniName);
	}
}

void ShaderChief::SetUni4f(cstr uniName, float values[4], bool warn){
	int uniLocation = glGetUniformLocation(currID, uniName); //Query location of uniform
	if(uniLocation != -1){
		glUniform4f(uniLocation, values[0], values[1], values[2], values[3]); //Sets uniform on the currently active shader prog
	} else if(warn){
		printf("%s: Failed to find '%s'\n", std::to_string(currID).c_str(), uniName);
	}
}

void ShaderChief::SetUni1i(cstr uniName, int value, bool warn){
	int uniLocation = glGetUniformLocation(currID, uniName); //Query location of uniform
	if(uniLocation != -1){
		glUniform1i(uniLocation, value); //Sets uniform on the currently active shader prog
	} else if(warn){
		printf("%s: Failed to find '%s'\n", std::to_string(currID).c_str(), uniName);
	}
}

void ShaderChief::UseProg(){
	if(shaderProgID == 999){
		Init();
	}
	if(currID != shaderProgID){
		glUseProgram(shaderProgID);
		currID = shaderProgID;
	}
}