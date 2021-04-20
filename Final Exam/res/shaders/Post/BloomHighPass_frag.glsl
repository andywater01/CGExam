#version 420

layout(binding = 0) uniform sampler2D s_screenTex; //Source image
uniform float uThreshold;

out vec4 FragColor;


layout(location = 0) in vec2 inUV;

void main()
{
    vec4 colour = texture(s_screenTex, inUV);

    float luminance = (colour.r + colour.g + colour.b) / 3.0;

    if (luminance > uThreshold)
    {
        FragColor = colour;
    }
    else
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}