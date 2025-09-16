#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

uniform mat4 u_MVP;

void main()
{
   gl_Position = u_MVP * vec4(aPos, 1.0);
   ourColor = aColor;
   TexCoord = aTexCoord;
}

#shader fragment
#version 330 core
out vec4 FragColor;
  
in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D u_Texture;

void main()
{
   FragColor = texture(u_Texture, TexCoord);
}