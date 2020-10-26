#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aNormal; //Vertex normals supplied by the model (how??)
layout (location = 4) in vec3 aTangent;
layout (location = 5) in int aDiffuseTexIndex;
layout (location = 6) in mat4 instanceMatrix;

out myInterface{ //Output interface block
    vec4 Colour;
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPosWorldSpace;
} vsOut; //Instance

out vec3 FragPosLocalSpace;
out mat3 TBN;
out vec3 FragViewSpacePos;
out vec3 FragClipSpacePos;
flat out int diffuseTexIndex;

uniform mat4 MVP;
uniform mat4 model, view, projection;
uniform bool useMat;
uniform bool screenQuad;
uniform bool cubemap;
uniform bool explosion;
uniform bool drawNormals;
uniform bool wave;
uniform float time;

out vec4 FragPosFromLightD;
out vec4 FragPosFromLightS;
uniform mat4 dLightSpaceVP;
uniform mat4 sLightSpaceVP;

void main(){
    diffuseTexIndex = aDiffuseTexIndex;
    FragPosLocalSpace = aPos;
    vsOut.Colour = aColor;
    vsOut.TexCoords = aTexCoords;
    vsOut.Normal = mat3(transpose(inverse(model))) * aNormal; //Multiplication with normal matrix
    vsOut.FragPosWorldSpace = vec3(model * vec4(aPos, 1.f));
    FragPosFromLightD = dLightSpaceVP * vec4(vsOut.FragPosWorldSpace, 1.f);
    FragPosFromLightS = sLightSpaceVP * vec4(vsOut.FragPosWorldSpace, 1.f);

    if(wave){
		vec4 worldSpacePos = model * vec4(aPos, 1.f);
		worldSpacePos.y += sin(worldSpacePos.x + time) * cos(worldSpacePos.z + time) * 5.f;
		gl_Position = projection * view * worldSpacePos;
		//vsOut.TexCoords.x += time;
        FragViewSpacePos = vec3(view * worldSpacePos);
        FragClipSpacePos = gl_Position.xyz;
        return;
	}
    FragViewSpacePos = vec3(view * model * vec4(aPos, 1.f));

    vec3 T = normalize(mat3(transpose(inverse(model))) * aTangent);
    //vec3 B = normalize(mat3(transpose(inverse(model))) * aBitangent); //More precise to multiply with normal matrix as only orientation of vecs matters
    vec3 N = normalize(mat3(transpose(inverse(model))) * aNormal);

    ///Gram–Schmidt process (Tangent vecs of larger meshes that share many vertices are generally avged to give smooth results which causes TBN vectors to become non-orthogonal)
    T = normalize(T - dot(T, N) * N); //Re-orthogonalise T with respect to N
    vec3 B = cross(N, T);

    TBN = mat3(T, B, N);
    //TBN = transpose(mat3(T, B, N)); //Transpose of orthogonal matrix (every axis is a perpendicular unit vec) == its inverse

    if(useMat){
        vsOut.Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;
        vsOut.FragPosWorldSpace = vec3(instanceMatrix * vec4(aPos, 1.f));
        mat4 modelView = view * instanceMatrix;

        modelView[1][1] = 4; //4 is scale of instance
        modelView[0][0] = modelView[1][1];
        modelView[2][2] = modelView[1][1];
        modelView[0][1] = 0;
        modelView[1][0] = 0;
        modelView[0][2] = 0;
        modelView[2][0] = 0;
        modelView[2][1] = 0;
        modelView[1][2] = 0;
        FragViewSpacePos = vec3(modelView * vec4(aPos, 1.f));

        gl_Position = projection * vec4(FragViewSpacePos, 1.f);
        FragClipSpacePos = gl_Position.xyz;
        return;
    }
    if(screenQuad){
        gl_Position = model * vec4(aPos.x, aPos.y, 0.f, 1.f); //What if z != 0.f??
        FragClipSpacePos = gl_Position.xyz;
        return;
    }
    if(cubemap){
        gl_Position = projection * view * vec4(aPos, 1.f);
        gl_Position = gl_Position.xyww; //Resulting NDC after perspective division will have a z value (gl_FragCoord.z) equal to 1.f
        FragClipSpacePos = gl_Position.xyz;
        return;
    }
    if(explosion){
        gl_Position = vec4(aPos, 1.f);
        FragClipSpacePos = gl_Position.xyz;
        return;
    }
    if(drawNormals){
        mat3 normalMtx = mat3(transpose(inverse(view * model)));
        vsOut.Normal = normalize(vec3(projection * vec4(normalMtx * aNormal, 0.f))); //Transformed clip-space normal vec //Multiply by normalMatrix to acct for scaling and rotations due to view and model matrix??
    }
    gl_Position = MVP * vec4(aPos, 1.f);
    FragClipSpacePos = gl_Position.xyz;
}