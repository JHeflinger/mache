void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;

    // Normalise to [-1, 1] with correct aspect ratio
    vec2 p = (uv * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);

    float t = iTime * 0.5 * 0.25 * 0.25;

    // Layered sine waves → plasma effect
    float v  = sin(p.x * 4.0 + t);
         v += sin(p.y * 4.0 + t * 1.3);
         v += sin((p.x + p.y) * 3.0 + t * 0.7);
         v += sin(length(p) * 5.0 - t * 1.1);

    // Map to colour
    float r = 0.5 + 0.5 * sin(v * 3.14159);
    float g = 0.5 + 0.5 * sin(v * 3.14159 + 2.094);  // +120°
    float b = 0.5 + 0.5 * sin(v * 3.14159 + 4.189);  // +240°

    fragColor = vec4(r, g, b, 1.0);
}
