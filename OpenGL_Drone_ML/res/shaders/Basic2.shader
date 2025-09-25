#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in float aTexIndex;

out vec3 ourColor;
out vec2 TexCoord;
out float vTexIndex;

uniform mat4 u_MVP;

void main()
{
   gl_Position = u_MVP * vec4(aPos, 1.0);
   ourColor = aColor;
   TexCoord = aTexCoord;
   vTexIndex = aTexIndex;
}

#shader fragment
#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;
in float vTexIndex;

uniform sampler2D u_Textures[8];

void main()
{
   int index = int(vTexIndex);
   if (index < 0) {
      FragColor = vec4(ourColor, 1.0);
   }
   else {
      FragColor = texture(u_Textures[index], TexCoord);
   }
}