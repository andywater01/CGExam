#version 420

layout(binding = 0) uniform sampler2D s_screenTex;
uniform float u_depth = 0.6;
layout(location = 0) in vec2 inUV;


out vec4 frag_color;

void main()
{
    frag_color = texture2D(s_screenTex, inUV);

    float blur = 0;

    float near_distance = 10.0; // blur attenuation range of the near plane
    float far_distance = 10.0; // blur attenuation range of the far plane

    float near_plane = -20.0; // near plane
    float far_plane = -25.0; // far plane

    // Calculate the blurring factor based on depth
    if (u_depth <= near_plane && u_depth >= far_plane)
    {
        blur = 0;
    }
    else if (u_depth > near_plane)
    {
        blur = clamp(u_depth, near_plane, near_plane + near_distance);
        blur = (blur - near_plane) / near_distance;
    }
    else if (u_depth < far_plane)
    {
        blur = clamp(u_depth, far_plane - far_distance, far_plane);
        blur = (far_plane - blur) / far_distance;
    }

    // Write the blur factor to the alpha channel
    frag_color.a = blur;
}































// Sepia Stuff

//layout(location = 0) in vec2 inUV;
//
//out vec4 frag_color;
//
//layout (binding = 0) uniform sampler2D s_screenTex;
//
////Intensity of the sepia effect
////Lower the number, closer to regular color
//uniform float u_Intensity = 0.6;
//
//void main() 
//{
//	vec4 source = texture(s_screenTex, inUV);
//
//	vec3 sepiaColor;
//	sepiaColor.r = ((source.r * 0.393) + (source.g * 0.769) + (source.b * 0.189));
//	sepiaColor.g = ((source.r * 0.349) + (source.g * 0.686) + (source.b * 0.168));
//	sepiaColor.b = ((source.r * 0.272) + (source.g * 0.534) + (source.b * 0.131));
//
//	frag_color.rgb = mix(source.rgb, sepiaColor.rgb, u_Intensity);
//	frag_color.a = source.a;
//}