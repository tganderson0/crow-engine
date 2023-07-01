#version 330 core
out vec4 FragColor;
  
in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D albedo;

void main()
{
    FragColor = texture(albedo, TexCoord);
    // FragColor = vec4(ourColor, 1.0);
}