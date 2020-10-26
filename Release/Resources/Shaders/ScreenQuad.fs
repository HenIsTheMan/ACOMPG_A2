#version 330 core
out vec4 FragColor;

in myInterface{
    vec4 Colour;
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPosWorldSpace;
} fsIn;

uniform sampler2D screenTex;
uniform sampler2D blurredTex;
uniform bool lineariseDepth;
uniform float near;
uniform float far;
uniform int typePPE; //Type of post-processing effect

void ApplyKernel(vec2 offsets[9], float kernel[9]){ //Apply 3x3 kernel
    FragColor = vec4(vec3(0.f), 1.f);
    for(int i = 0; i < 9; ++i){
        FragColor.rgb += vec3(texture(screenTex, fsIn.TexCoords.st + offsets[i]) + texture(blurredTex, fsIn.TexCoords.st + offsets[i])) * kernel[i]; //Multiply sampled tex values with weighted kernel values and add the products
    }
}

float LineariseDepth(float depth){ //Reverse the process of projection for depth values
    float NDC = depth * 2.f - 1.f; //Re-transform depth values in the range [0, 1] to NDC (range [-1, 1])
    return (2.f * near * far) / (far + near - NDC * (far - near));	//Reverse non-linear eqn from the projection matrix and apply this inversed transformation eqn
}

void main(){
    const float offset = 1.f / 300.f;
    vec2 offsets[9] = vec2[]( //For each surrounding texCoord
        vec2(-offset, offset), //Top left
        vec2(0.f, offset), //Top centre
        vec2(offset, offset), //Top right
        vec2(-offset, 0.f), //Centre left
        vec2(0.f, 0.f), //Centre
        vec2(offset, 0.f), //Centre right
        vec2(-offset, -offset), //Bottom left
        vec2(0.f, -offset), //Bottom centre
        vec2(offset, -offset) //Bottom right
    );

    vec4 colResult = vec4(vec3(texture(screenTex, fsIn.TexCoords) + texture(blurredTex, fsIn.TexCoords)), texture(screenTex, fsIn.TexCoords).a); //Additive blending
    switch(typePPE){
        case 0: FragColor = colResult; break;
        case 1: FragColor = vec4(vec3(1.f) - vec3(colResult), 1.f); break; //Colour Inversion
        case 2: { //Grayscale
            FragColor = colResult;
            //float avg = (FragColor.r + FragColor.g + FragColor.b) / 3.f;
            float avg = .2126f * FragColor.r + .7152f * FragColor.g + .0722f * FragColor.b; //Weighted grayscale (weighted colour channels used, most physically accurate)
            FragColor = vec4(vec3(avg), 1.f);
        } break;

        case 3: ApplyKernel(offsets, float[](-1, -1, -1, -1, 9, -1, -1, -1, -1)); break; //Sharpen kernel (sharpens each colour value by sampling all surrounding pixels)
        case 4: ApplyKernel(offsets, float[](.0625f, .125f, .0625f, .125f, .25f, .125f, .0625f, .125f, .0625f)); break; //Blur kernel (vary blur amt over time for drunk effect, can use blur for smoothing colour values) //Because all values add up to 16, directly returning the combined sampled colors would result in an extremely bright color so we have to divide each value of the kernel by 16??
        default: ApplyKernel(offsets, float[](1.f, 1.f, 1.f, 1.f, -8.f, 1.f, 1.f, 1.f, 1.f)); //Edge-detection kernel (highlights all edges and darkens the rest)
    }
    if(lineariseDepth){ //shorten?? //gamma correction??
        float depthVal = texture(screenTex, fsIn.TexCoords).r;
        FragColor = vec4(vec3(LineariseDepth(depthVal) / far), 1.f);
    }

    //FragColor.rgb = FragColor.rgb / (FragColor.rgb + vec3(1.f)); //Reinhard tone mapping alg (evenly balances out colour values, tend to slightly favour bright areas)
    const float exposure = .2f; //Default is 1.f
    FragColor.rgb = vec3(1.f) - exp(-FragColor.rgb * exposure); //Exposure tone mapping (day and night cycle, config lighting parameters with exposure parameter [selectively favors bright or dark regions])??

    const float gamma = 2.2f; //sRGB colour space roughly corresponds to a monitor gamma of 2.2
    FragColor.rgb = pow(FragColor.rgb, vec3(1.f / gamma)); //Gamma correction (makes monitor display colours as linearly set, makes dark areas show more details, need as we config colour and lighting vars in sRGB space, multiply linear input values with reciprocal of gamma to brighten them 1st)
    //We generally set colour and lighting values higher (makes most linear-space calculations wrong) since the monitor darkens intermediate... values (as human eyes are more susceptible to changes in dark colours)
    //Non-linear mapping of CRT monitors outputs more pleasing brightness to our eyes //Makes clear colour lighter
}