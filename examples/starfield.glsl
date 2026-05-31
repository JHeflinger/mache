float hash(vec2 p) {
    p = fract(p * vec2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv  = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    float t  = iTime * 0.3;

    vec3 col = vec3(0.0);

    // Five depth layers
    for (int i = 0; i < 5; i++) {
        float fi    = float(i);
        float speed = 0.1 + fi * 0.08;
        float scale = 20.0 + fi * 15.0;
        float bright = 0.9 - fi * 0.15;

        vec2 grid = uv * scale;
        grid.y   += t * speed * 40.0;          // scroll downward
        vec2 cell = floor(grid);
        vec2 frac = fract(grid);

        float h  = hash(cell + fi * 57.3);
        float h2 = hash(cell + fi * 57.3 + vec2(1.7, 3.1));

        // Only ~15% of cells get a star
        if (h < 0.15) {
            // Star centre with sub-cell jitter
            vec2 offset = vec2(h2, fract(h2 * 13.7)) * 0.6 + 0.2;
            float dist  = length(frac - offset);

            // Twinkle
            float twinkle = 0.8 + 0.2 * sin(iTime * (3.0 + h * 7.0) + h * 6.28);
            float star = bright * twinkle * smoothstep(0.08, 0.0, dist);

            // Slight colour temperature variation
            vec3 starCol = mix(vec3(0.6, 0.8, 1.0), vec3(1.0, 0.9, 0.7), h2);
            col += star * starCol;
        }
    }

    // Subtle deep-blue background gradient
    col += vec3(0.02, 0.03, 0.08) * (1.0 - length(uv) * 0.8);

    fragColor = vec4(col, 1.0);
}
