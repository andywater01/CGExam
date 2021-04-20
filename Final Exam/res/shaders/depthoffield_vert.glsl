#version 410

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_ModelViewProjection;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inUV;

layout(location = 0) out vec2 outUV;
uniform float u_depth;

void main()
{
    inPosition = u_ModelViewProjection * inPosition; // Convert to viewpoint space

    outUV = inUV;
    u_depth = inPosition.z; // Record camera depth information

    inPosition = u_LightSpaceMatrix * inPosition; // Convert to projection space

}




















//#version 410
//
//layout (location = 0) in vec3 inPosition;
//layout (location = 1) in vec2 inUV;
//
//layout(location = 0) out vec2 outUV;
//
//void main()
//{ 
//	outUV = inUV;
//	gl_Position = vec4(inPosition, 1.0);
//}