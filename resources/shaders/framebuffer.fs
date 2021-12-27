#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;

uniform int option;
uniform bool hdr;
uniform float exposure;
uniform float gamma;
uniform bool bloom;

const float offset = 1.0 / 300.0;
vec2 offsets[9] = vec2[](
    vec2(-offset,  offset), // top-left
    vec2( 0.0f,    offset), // top-center
    vec2( offset,  offset), // top-right
    vec2(-offset,  0.0f),   // center-left
    vec2( 0.0f,    0.0f),   // center-center
    vec2( offset,  0.0f),   // center-right
    vec2(-offset, -offset), // bottom-left
    vec2( 0.0f,   -offset), // bottom-center
    vec2( offset, -offset)  // bottom-right
    );

vec3 col = vec3(0.0);
vec3 sampleTex[9];
float sharpenKernel[9] = float[](
    -1, -1, -1,
    -1,  9, -1,
    -1, -1, -1
    );
float blurKernel[9] = float[](
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16
    );
float edgeKernel[9] = float[](
    1, 1, 1,
    1, -8, 1,
    1, 1, 1
    );

vec3 genHdrColor();

void main()
{
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    // Selected effect
    switch(option){
        // No Effect
        default:
        case 0:
            // FragColor = texture(screenTexture, TexCoords);
            FragColor = vec4(genHdrColor(), 1.0);
            break;

        // Grayscale
        case 1:
            // hdrColor = texture(screenTexture, TexCoords);
            vec3 hdrColor = genHdrColor();
            float average = 0.2126 * hdrColor.r + 0.7152 * hdrColor.g + 0.0722 * hdrColor.b;
            FragColor = vec4(average, average, average, 1.0);
            break;

        // Inversion
        case 2:
            FragColor = vec4(vec3(1.0 - genHdrColor()), 1.0);
            break;

        // Sharpen
        // TODO: HDR
        case 3:
            for(int i = 0; i < 9; i++)
                sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
            col = vec3(0.0);
            for(int i = 0; i < 9; i++)
                col += sampleTex[i] * sharpenKernel[i];
            FragColor = vec4(col, 1.0);
            break;

        // Blur
        // TODO: HDR
        case 4:
            for(int i = 0; i < 9; i++)
                sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
            col = vec3(0.0);
            for(int i = 0; i < 9; i++)
                col += sampleTex[i] * blurKernel[i];
            FragColor = vec4(col, 1.0);
            break;

        // Edge detect
        // TODO: HDR
        case 5:
            for(int i = 0; i < 9; i++)
                sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
            col = vec3(0.0);
            for(int i = 0; i < 9; i++)
                col += sampleTex[i] * edgeKernel[i];
            FragColor = vec4(col, 1.0);
            break;
    }
}

vec3 genHdrColor(){
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    if(hdr) {
        if(bloom)
            hdrColor += bloomColor;

        // reinhard
        // vec3 result = hdrColor / (hdrColor + vec3(1.0));
        // exposure
        vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
        // also gamma correct while we're at it
        return pow(result, vec3(1.0 / gamma));
    } else {
        return hdrColor;
    }
}