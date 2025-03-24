#version 440 core

uniform vec3 uColor;
uniform uint uWidth;
uniform uint uHeight;
uniform double uXmax;
uniform double uXmin;
uniform double uYmax;
uniform double uYmin;
uniform int uMaxIteration;
out vec4 color;

vec3 getMathsTownColor(int iteration, int maxIteration)
{
    float t = float(iteration) / float(maxIteration);

    // Gradient color stops
    vec3 color1 = vec3(0.0, 0.0, 0.2);   // Deep Blue
    vec3 color2 = vec3(0.0, 0.8, 1.0);   // Electric Cyan
    vec3 color3 = vec3(1.0, 1.0, 0.5);   // Bright Yellow
    vec3 color4 = vec3(1.0, 0.5, 0.0);   // Fiery Orange

    // Multi-step gradient
    if (t < 0.3) 
        return mix(color1, color2, smoothstep(0.0, 0.3, t));
    else if (t < 0.6) 
        return mix(color2, color3, smoothstep(0.3, 0.6, t));
    else 
        return mix(color3, color4, smoothstep(0.6, 1.0, t));
}

void main()
{
    double deviceX = gl_FragCoord.x;
    double deviceY = gl_FragCoord.y;

    double x = (uXmax - uXmin)/double(uWidth)*deviceX + uXmin;
    double y = (uYmax - uYmin) / double(uHeight) * (uHeight - deviceY) + uYmin;

    double a = 0.0;
    double b = 0.0;

    int iteration = 0;
    while (a*a + b*b <= 4.0 && iteration < uMaxIteration)
    {
        double xtemp = a*a - b*b + x;
        b = 2*a*b + y;
        a = xtemp;
        iteration++;
    }

    color = vec4(getMathsTownColor(iteration, uMaxIteration), 1.0);
}