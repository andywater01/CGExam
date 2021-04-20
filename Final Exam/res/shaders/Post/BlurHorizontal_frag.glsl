#version 420

layout(binding = 0) uniform sampler2D s_screenTex; //Source image
uniform float uPixelSize; //1.0 / Window_Width

out vec4 FragColor;

layout(location = 0) in vec2 inUV;

void main() 
{
	//Sample pixels in a horizontal row
	//Weight should add up to 1
	FragColor = vec4(0.0, 0.0, 0.0, 0.0);

	FragColor += texture(s_screenTex, vec2(inUV.x - 4.0 * uPixelSize, inUV.y)) * 0.06;
	FragColor += texture(s_screenTex, vec2(inUV.x - 3.0 * uPixelSize, inUV.y)) * 0.09;
	FragColor += texture(s_screenTex, vec2(inUV.x - 2.0 * uPixelSize, inUV.y)) * 0.12;
	FragColor += texture(s_screenTex, vec2(inUV.x - 		uPixelSize, inUV.y)) * 0.15;
	FragColor += texture(s_screenTex, vec2(inUV.x,						inUV.y)) * 0.16;
	FragColor += texture(s_screenTex, vec2(inUV.x + 		uPixelSize, inUV.y)) * 0.15;
	FragColor += texture(s_screenTex, vec2(inUV.x + 2.0 * uPixelSize, inUV.y)) * 0.12;
	FragColor += texture(s_screenTex, vec2(inUV.x + 3.0 * uPixelSize, inUV.y)) * 0.09;
	FragColor += texture(s_screenTex, vec2(inUV.x + 4.0 * uPixelSize, inUV.y)) * 0.06;
}