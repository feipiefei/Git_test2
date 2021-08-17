#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    vec2 coords=TexCoords;
//    coords=coords*9.f;
//    coords=coords%2;
//    if(coords.x>1)coords.x=2-coords.x;
//    if(coords.y>1)coords.y=2-coords.y;
    vec3 col = texture(screenTexture, coords).rgb;
    //vec3 col = vec3(TexCoords.x,TexCoords.y,1-TexCoords.x);
    FragColor = vec4(col, 1.0);
} 