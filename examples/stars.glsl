#define PI 3.14159265359

mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x)
         + (c - a) * u.y * (1.0 - u.x)
         + (d - b) * u.x * u.y;
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;

    for (int i = 0; i < 6; i++) {
        v += a * noise(p);
        p = rot(0.55) * p * 2.05 + 17.3;
        a *= 0.52;
    }

    return v;
}

float ridge(vec2 p) {
    float v = 0.0;
    float a = 0.55;

    for (int i = 0; i < 7; i++) {
        float n = noise(p);
        n = abs(n * 2.0 - 1.0);
        n = 1.0 - n;
        v += n * n * a;

        p = rot(0.7) * p * 2.0 + 4.0;
        a *= 0.5;
    }

    return v;
}

float star(vec2 uv, float flare) {
    float d = length(uv);
    float m = 0.018 / max(d, 0.001);

    float rays = max(0.0, 1.0 - abs(uv.x * uv.y * 900.0));
    m += rays * flare;

    uv *= rot(PI * 0.25);
    rays = max(0.0, 1.0 - abs(uv.x * uv.y * 900.0));
    m += rays * flare * 0.35;

    m *= smoothstep(1.0, 0.15, d);
    return m;
}

vec3 starLayer(vec2 uv) {
    vec3 col = vec3(0.0);

    vec2 gv = fract(uv) - 0.5;
    vec2 id = floor(uv);

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 offs = vec2(float(x), float(y));
            float h = hash21(id + offs);

            if (h > 0.82) {
                vec2 pos = offs + vec2(hash21(id + offs + 13.1),
                                       hash21(id + offs + 91.7)) - 0.5;

                float size = mix(0.15, 0.75, pow(h, 8.0));
                float twinkle = 0.65 + 0.35 * sin(iTime * 2.0 + h * 60.0);

                vec3 tint = mix(
                    vec3(0.55, 0.72, 1.0),
                    vec3(1.0, 0.65, 0.32),
                    hash21(id + offs + 4.7)
                );

                col += tint * star(gv - pos, 0.018 * size) * size * twinkle;
            }
        }
    }

    return col;
}

vec2 flow(vec2 p) {
    float t = iTime * 0.04;

    float n1 = fbm(p * 1.2 + vec2(t, -t));
    float n2 = fbm(p * 1.2 + vec2(12.4 - t, 8.7 + t));

    vec2 v = vec2(n1 - 0.5, n2 - 0.5);
    return v;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    uv *= 1.15;

    float t = iTime * 0.08;

    vec3 col = vec3(0.015, 0.018, 0.045);
    col += vec3(0.02, 0.04, 0.09) * (uv.y + 0.6);

    vec2 p = uv;
    p *= rot(-0.62);

    float curve = 0.18 * sin(p.y * 2.0 + t * 1.4)
                + 0.08 * sin(p.y * 5.0 - t);
    float coreD = abs(p.x + curve);

    float core = exp(-coreD * 5.5) * smoothstep(1.25, -0.45, abs(p.y));
    float coreFine = ridge(p * vec2(3.5, 1.4) + flow(p * 1.5) * 1.2);

    vec3 coreCol = mix(
        vec3(0.25, 0.55, 1.0),
        vec3(1.0, 0.62, 0.28),
        smoothstep(-0.1, 0.8, p.y + 0.2)
    );

    col += coreCol * core * (0.45 + 1.1 * coreFine);

    float leftGlow = exp(-length((uv - vec2(-0.95, 0.45)) * vec2(0.8, 1.2)) * 1.6);
    float rightGlow = exp(-length((uv - vec2(0.95, -0.55)) * vec2(0.9, 1.0)) * 1.8);
    float blueGlow = exp(-length((uv - vec2(0.35, 0.2)) * vec2(0.9, 1.0)) * 1.5);

    col += vec3(1.0, 0.35, 0.10) * leftGlow * 0.9;
    col += vec3(1.0, 0.28, 0.08) * rightGlow * 0.65;
    col += vec3(0.18, 0.48, 1.0) * blueGlow * 0.65;

    vec3 wisps = vec3(0.0);

    for (int i = 0; i < 5; i++) {
        float fi = float(i);

        vec2 q = uv;
        q *= rot(0.45 + fi * 0.7);
        q += flow(q * (1.1 + fi * 0.18) + fi * 9.1) * 0.55;

        float w = ridge(q * vec2(1.2, 4.5) + vec2(t * (0.4 + fi * 0.1), fi));
        w = smoothstep(0.55, 1.15, w);

        float strand = exp(-abs(q.x + 0.22 * sin(q.y * 2.5 + fi + t)) * 7.0);
        vec3 wc = mix(vec3(0.22, 0.55, 1.0), vec3(1.0, 0.42, 0.15), step(2.0, fi));

        wisps += wc * w * strand * 0.28;
    }

    col += wisps;

    vec2 knots[7];
    knots[0] = vec2(-0.85,  0.42);
    knots[1] = vec2(-0.55,  0.18);
    knots[2] = vec2(-0.25, -0.28);
    knots[3] = vec2( 0.10, -0.08);
    knots[4] = vec2( 0.32,  0.32);
    knots[5] = vec2( 0.58, -0.35);
    knots[6] = vec2(-0.15,  0.70);

    for (int i = 0; i < 7; i++) {
        vec2 k = knots[i];
        float d = length(uv - k);

        float glow = exp(-d * 12.0);
        float hot = exp(-d * 75.0);

        vec3 kc = mix(
            vec3(0.35, 0.65, 1.0),
            vec3(1.0, 0.55, 0.18),
            hash21(k * 10.0)
        );

        col += kc * glow * 0.55;
        col += vec3(1.0, 0.9, 0.65) * hot * 2.2;
    }

    col += starLayer(uv * 18.0) * 0.45;
    col += starLayer(uv * 34.0 + 12.7) * 0.22;
    col += starLayer(uv * 70.0 - 3.3) * 0.08;

    float dust = fbm(uv * 7.0 + flow(uv * 2.0));
    col += vec3(0.25, 0.35, 0.65) * pow(dust, 5.0) * 0.25;

    float vignette = smoothstep(1.35, 0.25, length(uv * vec2(0.85, 1.0)));
    col *= vignette;

    col = 1.0 - exp(-col * 1.15);
    col = pow(col, vec3(0.78));

    fragColor = vec4(col, 1.0);
}
