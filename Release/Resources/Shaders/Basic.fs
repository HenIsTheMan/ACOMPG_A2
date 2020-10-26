#version 330 core

///Multiple Render Targets (MRT, can get multiple fs outputs in 1 render pass)
layout (location = 0) out vec4 FragColor; //Layout location specifier used (location corresponds to colour attachment of currently bound FBO the frag, that the fs is currently processing, will be rendered to [fs writes to...])
layout (location = 1) out vec4 BrightFragColor; //...

in myInterface{
    vec4 Colour;
    vec2 TexCoords;
    vec3 Normal;
    vec3 FragPosWorldSpace;
} fsIn;

struct Mat{
    sampler2D dMap; //Sets colour the surface reflects under diffuse lighting //sampler2D is an opaque type (cannot be instantiated but can only be defined as uniforms)
    sampler2D sMap; //Sets colour of specular highlight on surface or possibly reflect a surface-specific color //Hack: use to make some surfaces less lit due to details like depth
    sampler2D eMap; //Stores tex unit of emission map (stores emission values [colours an obj might emit as if it contains a light source itself] per frag)
    sampler2D nMap; //Stores per-fragment normals as colours (per-component range: [0.f, 1.f]) so sample... and use them over interpolated per-surface normals //Bluelish as normals (per-component range: [-1.f, 1.f]) are closely pointing towards +ve z-axis
    sampler2D rMap; //Stores reflection map (texture, determine which parts of the model reflect light and by what intensity)
    float shininess; //Impacts the scattering of light and hence radius of the specular highlight
};

struct PointLight{ //Positional light source
    vec3 pos;
    float constant; //Constant term //Makes sure the denominator >= 1.f
    float linear; //Linear term //Multiplied with dist to reduce light intensity in a linear fashion
    float quadratic; //Quadratic term //Multiplied with the quadrant of the dist to set quadratic decreases in light intensity //Less significant compared to linear term when dist is small
};

struct DirectionalLight{ //Directional light source
    vec3 dir;
};

struct Spotlight{ //Positional light that shoots light rays in 1 dir, objs within its cut-offs (a certain radius of its dir) are lit
    vec3 pos;
    vec3 dir;
    float cosInnerCutoff; //cos(inner cut-off angle)
    float cosOuterCutoff; //cos(outer cut-off angle)
};

layout (std140) uniform Settings{
    float brightness;
};

#define maxAmtP 10
#define maxAmtD 10
#define maxAmtS 10
uniform int amtP, amtD, amtS;
uniform PointLight pLights[maxAmtP];
uniform DirectionalLight dLights[maxAmtD];
uniform Spotlight sLights[maxAmtS];
uniform Mat material;

uniform bool wave;
uniform vec3 camPos;
uniform samplerCube cubemapSampler;
uniform bool cubemap;
uniform bool skydome;
uniform bool useFlatColour;
uniform bool emission, bump, reflection;
uniform bool useReflectionMap;
uniform bool useMat;
const float gamma = 2.2f; //Since gamma not applied again before gamma correction as sRGB texs are alr in sRGB space (perceived as linear space by the eyes as input voltage is exponentially related to brightness [closely match how humans measure brightness as brightness is also displayed with a similar inverse pow relationship??, perceived linear brightness not equals to physical...])

in mat3 TBN; //Change-of-basis matrix that transforms vecs in tangent space (local [relative to local ref frame of each primitive] to surface of each primitive, local space of the normal map's vecs) to another coord space (world space in this case so dot product makes sense as lighting vecs like lightDir and viewDir are in world space)
vec3 normal = normalize(bump ? TBN * (texture(material.nMap, fsIn.TexCoords).rgb * 2.f - 1.f) : fsIn.Normal); //Reverse process of mapping normals in tangent space to colours in RGB space (n / 2.f + .5f) by mapping components of sampled colours from [0, 1] to [-1, 1]
//vec3 normal = normalize(bump ? texture(material.nMap, fsIn.TexCoords).rgb * 2.f - 1.f : fsIn.Normal);

///Light components (store in class??)
const vec3 lightAmbient = vec3(.05f); //Small value for small impact
const vec3 lightDiffuse = vec3(.8f); //Set to desired colour of light
const vec3 lightSpecular = vec3(1.f);

in vec3 FragPosLocalSpace;
uniform bool useMultiTex;
uniform sampler2D lowTex;
uniform sampler2D highTex;

vec3 CalcAmbient(){ //Ambient lighting (ensures objs always have colour due to light from distant light source(s))
    if(!useMultiTex){
        return lightAmbient * texture(material.dMap, fsIn.TexCoords).rgb; //Ambient component of light * that of frag's material //Shadows are rarely completely dark due to light scattering so not included...
    }
    float height = FragPosLocalSpace.y;
    if(height <= .3f){
        return lightAmbient * pow(texture(lowTex, fsIn.TexCoords).rgb, vec3(gamma));
    } else if(height <= .35f){
        return lightAmbient * pow(mix(texture(lowTex, fsIn.TexCoords).rgb, texture(material.dMap, fsIn.TexCoords).rgb, .5f), vec3(gamma));
    } else if(height <= .65f){
        return lightAmbient * pow(texture(material.dMap, fsIn.TexCoords).rgb, vec3(gamma));
    } else if(height <= .7f){
        return lightAmbient * pow(mix(texture(material.dMap, fsIn.TexCoords).rgb, texture(highTex, fsIn.TexCoords).rgb, .5f), vec3(gamma));
    } else{
        return lightAmbient * pow(texture(highTex, fsIn.TexCoords).rgb, vec3(gamma));
    }
}

vec3 CalcDiffuse(vec3 lightDir){ //Diffuse lighting (simulates directional impact of light, light intensity is affected by facing of obj) //Texs for colouring objs like diffuse texs are usually in sRGB space
    float dImpact = max(dot(normal, -lightDir), 0.f); //Diffuse impact of light on curr frag
    if(!useMultiTex){
        return dImpact * lightDiffuse * pow(texture(material.dMap, fsIn.TexCoords).rgb, vec3(gamma)); //Diffuse component (> 0.f && <= 1.f when angle between... (>= 0.f && < 90.f) || (> 270.f && <= 360.f)) of frag
    }
    float height = FragPosLocalSpace.y;
    if(height <= .3f){
        return dImpact * lightDiffuse * pow(texture(lowTex, fsIn.TexCoords).rgb, vec3(gamma));
    } else if(height <= .35f){
        return dImpact * lightDiffuse * pow(mix(texture(lowTex, fsIn.TexCoords).rgb, texture(material.dMap, fsIn.TexCoords).rgb, .5f), vec3(gamma));
    } else if(height <= .65f){
        return dImpact * lightDiffuse * pow(texture(material.dMap, fsIn.TexCoords).rgb, vec3(gamma));
    } else if(height <= .7f){
        return dImpact * lightDiffuse * pow(mix(texture(material.dMap, fsIn.TexCoords).rgb, texture(highTex, fsIn.TexCoords).rgb, .5f), vec3(gamma));
    } else{
        return dImpact * lightDiffuse * pow(texture(highTex, fsIn.TexCoords).rgb, vec3(gamma));
    }
}

uniform bool useSpecular;

vec3 CalcSpecular(vec3 lightDir){ //Specular lighting (simulates bright spot of light that appears on shiny objs, specular highlights are more inclined to light colour than obj colour)
    if(!useSpecular){
        return vec3(0.f);
    }

    ///If use Phong lighting/reflection/shading model (specular reflections break when specular area is large and rough due to low shininess)
    //vec3 reflectDir = reflect(lightDir, normal); //Not -lightDir as reflect(...) expects the 1st vec to point from the light source to the curr frag's pos
    //float sImpact = pow(max(dot(-viewDir, reflectDir), 0.f), material.shininess);

    vec3 viewDir = normalize(fsIn.FragPosWorldSpace - camPos);
    //vec3 viewDir = TBN * normalize(fsIn.FragPosWorldSpace - camPos);
    vec3 halfwayDir = -normalize(lightDir + viewDir); //Unit vec halfway between lightDir and viewDir (more specular contribution the more it's aligned with the surface normal)
    float sImpact = pow(max(dot(normal, halfwayDir), 0.f), material.shininess);
    return sImpact * lightSpecular * texture(material.sMap, fsIn.TexCoords).rgb;
} //Texs for retrieving lighting parameters like specular and bump/... texs are usually in linear space

in vec4 FragPosFromLightD;
in vec4 FragPosFromLightS;
uniform bool depthOnly;
uniform bool showShadowsD;
uniform bool showShadowsS;
uniform sampler2D shadowMapD;
uniform sampler2D shadowMapS;

float NotShadow(vec3 lightDir, sampler2D shadowMap, vec4 FragPosFromLight){ //Shadows (adds realism to a lit scene, makes spatial relationship between objs easier to observe, gives greater sense of depth to scene and objs in it) are formed with absence of light due to occlusion
    vec3 projectedFragCoords = FragPosFromLight.xyz / FragPosFromLight.w; //Transform pt in light's visible coord space/clip space (-w, w) to NDC (-1 to 1) thru perspective division (divide gl_Position's xyz coords by its w-component, done automatically after vertex shader step if output clip-space vertex pos thru gl_Position, allows both types of projection to be used)
    projectedFragCoords = projectedFragCoords * .5f + .5f; //Transform NDC to range of [0, 1] so can use to index/... from depth/... map //Because the depth from the depth map is in the range [0, 1]??
    if(projectedFragCoords.z > 1.f){ //Reduce oversampling of depth/... map (...) by accting for currDepth > 1.f when projectedFragCoords.z > 1.f if light-space projected frag is outside far plane of light's... (in dark region at the far end [in the dir of shadows] of light's...)
        return 1.f;
    } //All frags...
    float closestDepth = texture(shadowMap, projectedFragCoords.xy).r; //Use pt in light's... to index depth/... map to get closest visible depth from light's POV //Use r as colours of shadowMap range from red to black
    float currDepth = projectedFragCoords.z; //Curr depth of frag from light's POV
    float shadowBias = max(.05f * (1.f - dot(normal, -lightDir)), .005f); //Shadow bias (offset surface depth [currDepth] or depth/... map depth [closestDepth] such that frags are not considered below the surface) to solve shadow acne (shadow mapping artefact) //Max of .05f and min of .005f //Diff for each scene so increment until shadow acne is removed //Based on angle between light dir and surface normal
    //return currDepth - shadowBias <= closestDepth; //Surfaces almost perpendicular to dLight get a small bias?? If the surface would have a steep angle to the light source, the shadows may still display shadow acne??

    ////PCF (%-closer filtering, reduce jagged and blocky edges of shadows due to multiple frags sampling depth from same texel of the depth/... map with the depth/... map having a fixed resolution, sample depth/... map multiple times with diff texCoords every time then combine and avg all "NotInShadow" results to produce softer shadows)
    ///Sample surrounding texels of depth/... map then... (use more depth samples and/or vary texelSize to increase quality of soft shadows)
    ///Also can reduce... by increasing depth/... map resolution or fitting the light's visible frustum as closely to the scene as possible
    float notInShadowSum = 0.f;
    vec2 texelSize = 1.f / textureSize(shadowMap, 0); //Reciprocal of size of tex at mipmap lvl 0
    for(float x = -1.f; x <= 1.f; ++x){
        for(float y = -1.f; y <= 1.f; ++y){
            float closestDepth = texture(shadowMap, projectedFragCoords.xy + vec2(x, y) * texelSize).r;
            notInShadowSum += float(currDepth - shadowBias <= closestDepth);
        }
    }
    return notInShadowSum / 9.f; //9 depth samples taken
}

vec3 CalcPointLight(PointLight light){ //Calc point light's contribution vec
    vec3 lightDir = normalize(fsIn.FragPosWorldSpace - light.pos); //Dir of directed light ray (diff vec between...)
    //vec3 lightDir = TBN * normalize(fsIn.FragPosWorldSpace - light.pos);

    ///Attenuation (reducing light intensity over the dist it travels, use linear without gamma correction [quadratic attenuation too strong and gives lights a small radius] as it closely resembles quadratic with gamma applied and use quadratic (physically correct) with... as linear attenuation looks too weak)
    float dist = length(fsIn.FragPosWorldSpace - light.pos);
    float attenuation = 1.f / (light.constant + light.linear * dist + light.quadratic * dist * dist); //Other attenuation functions (change brightness)?? //Diff parameters to acct for gamma correction??
    return attenuation * (CalcAmbient() + CalcDiffuse(lightDir) + CalcSpecular(lightDir));
}

vec3 CalcDirectionalLight(DirectionalLight light){ //Calc directional light's contribution vec
    vec3 lightDir = normalize(light.dir);
    //vec3 lightDir = TBN * normalize(light.dir);

    return CalcAmbient() + (showShadowsD ? NotShadow(lightDir, shadowMapD, FragPosFromLightD) : 1.f) * (CalcDiffuse(lightDir) + CalcSpecular(lightDir));
}

vec3 CalcSpotlight(Spotlight light){ //Calc spotlight's contribution vec
    vec3 lightDir = normalize(fsIn.FragPosWorldSpace - light.pos); //Dir of directed light ray (diff vec between...)
    //vec3 lightDir = TBN * normalize(fsIn.FragPosWorldSpace - light.pos);

    float cosTheta = dot(lightDir, normalize(light.dir));
    float epsilon = light.cosInnerCutoff - light.cosOuterCutoff; //Soft/Smooth edges (using inner and outer cone, interpolate between outer cos and inner cos based on theta)
    float lightIntensity = clamp((cosTheta - light.cosOuterCutoff) / epsilon, 0.f, 1.f); //-ve when outside the outer cone of the spotlight and > 1.f when inside... before clamping
    return CalcAmbient() + (showShadowsS ? NotShadow(lightDir, shadowMapS, FragPosFromLightS) : 1.f) * lightIntensity * (CalcDiffuse(lightDir) + CalcSpecular(lightDir)); //Leave ambient component unaffected by lightIntensity so length(ambient) > 0
}

struct Fog{
    vec3 colour;
    float start; //For linear fog
    float end; //For linear fog
    float density; //For exponential fog
    int type;
};

in vec3 FragViewSpacePos;
in vec3 FragClipSpacePos;
uniform Fog fog;
uniform bool useFog;

float CalcFogFactor(Fog fog, float fogDist){
    float fogInverseFactor = 0.f;
    switch(fog.type){
        case 0: fogInverseFactor = (fog.end - fogDist) / (fog.end - fog.start); break; //Linear
        case 1: fogInverseFactor = exp(-fog.density * fogDist); break; //Exponential
        case 2: fogInverseFactor = exp(-pow(fog.density * fogDist, 2.f)); //Exponential Squared
    }
    return 1.f - clamp(fogInverseFactor, 0.f, 1.f);
}

uniform bool particle;
flat in int diffuseTexIndex;
uniform sampler2D diffuseMaps[32];
uniform bool useColourMultiplier;
uniform vec3 colourMultiplier;

void main(){ //Blinn-Phong lighting/reflection/shading model (angle between halfway vec and surface normal vec < angle between view vec and reflection vec so set Blinn-Phong shininess 2 - 4 times Phong shininess to get similar visuals, used in the earlier fixed function pipeline of OpenGL??)
    if((useMat && texture(material.dMap, fsIn.TexCoords).a < .05f) || (diffuseTexIndex >= 0 && diffuseTexIndex < 32 && particle && texture(diffuseMaps[diffuseTexIndex], fsIn.TexCoords).a < .05f)){
        discard; //Discard fragments (not stored in the color buffer) over blending them for fully transparent objs so no depth issues
    }
    if(depthOnly){ //No need processing as no colour buffer
        //gl_FragDepth = gl_FragCoord.z; //What happens behind the scenes
        return; //Depth buffer is updated after fragment shader run
    }
    if(cubemap){
        FragColor = texture(cubemapSampler, fsIn.FragPosWorldSpace); //Use dir vec representing a 3D texCoord for indexing/sampling the cubemap (similar to interpolated local vertex pos vec of a cube) //Can be a non-unit vec
        FragColor.rgb = pow(FragColor.rgb, vec3(gamma));
        if(useFog){
            FragColor.rgb = mix(FragColor.xyz, fog.colour, CalcFogFactor(fog, abs(FragViewSpacePos.z)));
        }
        return;
    }
    if(skydome){
        FragColor = texture(material.dMap, fsIn.TexCoords) * vec4(vec3(brightness), 1.f);
        FragColor.rgb = pow(FragColor.rgb, vec3(gamma));
        if(useFog){
            FragColor.rgb = mix(FragColor.xyz, fog.colour, CalcFogFactor(fog, abs(FragViewSpacePos.z)));
        }
        return;
    }
    if(useFlatColour){
        FragColor = fsIn.Colour;
        FragColor.rgb = pow(FragColor.rgb, vec3(gamma));
        if(useFog){
            FragColor.rgb = mix(FragColor.xyz, fog.colour, CalcFogFactor(fog, abs(FragViewSpacePos.z)));
        }
        return;
    }

    if(particle){
        FragColor = fsIn.Colour;
    } else{
        const float ratio = 1.f / 1.52f; //n of air / n of glass (ratio between refractive indices of both materials)
        vec3 incidentRay = normalize(fsIn.FragPosWorldSpace - camPos);
        vec3 reflectedRay = reflect(incidentRay, normal);
        vec3 refractedRay = refract(incidentRay, normal, ratio);

        if(amtP + amtD + amtS == 0){
            FragColor = vec4(CalcAmbient(), 1.f);
        } else{
            if(useColourMultiplier){
                FragColor = vec4(CalcAmbient(), 1.f);
                FragColor.rgb *= colourMultiplier;
            } else{
                FragColor = vec4(vec3(0.f), 1.f);
                for(int i = 0; i < amtP; ++i){
                    FragColor.rgb += CalcPointLight(pLights[i]) + (emission ? vec3(texture(material.eMap, fsIn.TexCoords)) : vec3(0.f));
                }
                for(int i = 0; i < amtD; ++i){
                    FragColor.rgb += CalcDirectionalLight(dLights[i]) + (emission ? vec3(texture(material.eMap, fsIn.TexCoords)) : vec3(0.f));
                }
                for(int i = 0; i < amtS; ++i){
                    FragColor.rgb += CalcSpotlight(sLights[i]) + (emission ? vec3(texture(material.eMap, fsIn.TexCoords)) : vec3(0.f));
                }
            }
        }
        if(reflection){ //For environment mapping
            FragColor.rgb += texture(cubemapSampler, reflectedRay).rgb * (useReflectionMap ? vec3(texture(material.rMap, fsIn.TexCoords)) : vec3(1.f)); //texture(...) returns colour of tex at an interpolated set of texCoords
        }
        if(useFog){
            FragColor.rgb = mix(FragColor.xyz, fog.colour, CalcFogFactor(fog, abs(FragViewSpacePos.z)));
        }
        if(wave){
            FragColor.a = .5f;
        }
    }

    float brightness = dot(FragColor.rgb, vec3(.2126f, .7152f, .0722f)); //Transform FragColor to grayscale with dot product
    BrightFragColor = vec4(FragColor.rgb * vec3(float(brightness >= 3.f)), 1.f); //3.f is brightness threshold (outside LDR with HDR rendering so more control over what is considered bright)

    //Amplify and give noticeable visual cues to bright coloured regions with high light intensities with bloom (PPE, glow and light bleeding, good when subtle)
    //Render lit scene to floating pt texColourBuffer and get the extracted brightness img. Blur... (use blur filter) then render it over the HDR scene img
    //Visual quality of bloom affected by range and strength of the blur filter used ++ quality and type??
    //Bright regions are extended in both width and height due to the blur filter??
}