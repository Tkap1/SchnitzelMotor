#version 430

// Input
layout (location = 0) in vec2 textureCoordsIn;

// Output
layout (location = 0) out vec4 fragColor;

// Bindings
layout (binding = 0) uniform sampler2D textureAtlas;

void main()
{
  vec4 textureColor = texelFetch(textureAtlas, ivec2(textureCoordsIn), 0);

  if(textureColor.a == 0.0)
  {
    discard;
  }

  // Fill with White
  fragColor = textureColor;
}
