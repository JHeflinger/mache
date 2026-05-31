#define S smoothstep
#define SLOWDOWN 0.07

vec4 Line(vec2 uv, float speed, float height, vec3 col) {
    uv.y += S(1., 0., abs(uv.x)) * sin(SLOWDOWN * iTime * speed + uv.x * height) * .2;
    return vec4(S(.06 * S(.2, .9, abs(uv.x)), 0., abs(uv.y) - .004) * col, 1.0) * S(1., .3, abs(uv.x));
}

void mainImage(out vec4 O, in vec2 I) {
    vec2 uv = (I - .5 * iResolution.xy) / iResolution.y;
    O = vec4 (0.);
    for (float i = 0.; i <= 5.; i += 1.) {
        float t = i / 5.;
        O += Line(uv, 1. + t, 4. + t, mix(
            vec3(0.75, 0.90, 1.00), // pale blue
            vec3(0.25, 0.10, 0.80), // deep blue-purple
            t));
    }
}
