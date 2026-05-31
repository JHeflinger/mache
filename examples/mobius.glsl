// =====================================================================
// Seamless Luminous Möbius Waterfall - Cinematic Contrast
// =====================================================================

#define MAX_STEPS 150
#define MAX_DIST 25.0
#define SURF_DIST 0.001

// Strip Dimensions
#define R 2.5   // Major radius
#define W 0.7   // Half-width
#define T 0.05  // Half-thickness

#define SPEED 1.0

// ---------------------------------------------------------------------
// Noise & Utilities
// ---------------------------------------------------------------------
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

float hash(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.x + p.y) * p.z);
}

float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(i+vec3(0,0,0)), hash(i+vec3(1,0,0)), f.x),
                   mix(hash(i+vec3(0,1,0)), hash(i+vec3(1,1,0)), f.x), f.y),
               mix(mix(hash(i+vec3(0,0,1)), hash(i+vec3(1,0,1)), f.x),
                   mix(hash(i+vec3(0,1,1)), hash(i+vec3(1,1,1)), f.x), f.y), f.z);
}

float fbm(vec3 p) {
    float f = 0.0, amp = 0.5;
    for(int i = 0; i < 4; i++) {
        f += amp * noise(p); p *= 2.0; amp *= 0.5;
    }
    return f;
}

// ---------------------------------------------------------------------
// Geometry & Mapping
// ---------------------------------------------------------------------

float map(vec3 p) {
    float a = atan(p.z, p.x);
    float r = length(p.xz) - R;
    
    float c = cos(a * 0.5);
    float s = sin(a * 0.5);
    
    vec2 q = vec2(r * c + p.y * s, -r * s + p.y * c);
    
    vec2 d = abs(q) - vec2(W, T);
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - 0.02;
}

vec3 surfaceFlow(vec3 p, float a, float time, float stretch) {
    float r = length(p.xz) - R;
    float c = cos(a * 0.5);
    float s = sin(a * 0.5);
    
    float u = r * c + p.y * s;
    float v = -r * s + p.y * c;
    
    float a0 = a - time;
    float c0 = cos(a0 * 0.5);
    float s0 = sin(a0 * 0.5);
    
    float r0 = u * c0 - v * s0;
    float y0 = u * s0 + v * c0;
    
    float Reff = R / stretch;
    float x0 = (Reff + r0) * cos(a0);
    float z0 = (Reff + r0) * sin(a0);
    
    return vec3(x0, y0, z0);
}

float fluidHeight(vec3 p, float a, float time) {
    vec3 p0 = surfaceFlow(p, a, time * 1.2, 3.0);
    float h = fbm(p0 * 2.5);
    
    vec3 p1 = surfaceFlow(p, a, time * 0.8, 1.5);
    h += 0.5 * fbm(p1 * 5.0 + vec3(12.34));
    
    vec3 p2 = surfaceFlow(p, a, time * 1.5, 4.0);
    float caustics = abs(sin(fbm(p2 * 6.0) * 8.0));
    h += 0.25 * pow(1.0 - caustics, 1.5);
    
    return h * 0.57;
}

// ---------------------------------------------------------------------
// Normals & Shading
// ---------------------------------------------------------------------

vec3 calcNormal(vec3 p) {
    vec2 e = vec2(0.005, 0.0);
    return normalize(vec3(
        map(p + e.xyy) - map(p - e.xyy),
        map(p + e.yxy) - map(p - e.yxy),
        map(p + e.yyx) - map(p - e.yyx)
    ));
}

float unwrap(float a, float ref) {
    float d = a - ref;
    if (d > 3.14159) return a - 6.28318;
    if (d < -3.14159) return a + 6.28318;
    return a;
}

vec3 calcBumpNormal(vec3 p, vec3 n, float time) {
    vec2 e = vec2(0.01, 0.0);
    float a = atan(p.z, p.x);
    
    float ax1 = unwrap(atan(p.z, p.x + e.x), a);
    float ax2 = unwrap(atan(p.z, p.x - e.x), a);
    float ay  = a; 
    float az1 = unwrap(atan(p.z + e.x, p.x), a);
    float az2 = unwrap(atan(p.z - e.x, p.x), a);
    
    float hx = fluidHeight(p + e.xyy, ax1, time) - fluidHeight(p - e.xyy, ax2, time);
    float hy = fluidHeight(p + e.yxy, ay,  time) - fluidHeight(p - e.yxy, ay,  time);
    float hz = fluidHeight(p + e.yyx, az1, time) - fluidHeight(p - e.yyx, az2, time);
    
    vec3 grad = vec3(hx, hy, hz) / (2.0 * e.x);
    grad -= n * dot(grad, n); 
    return normalize(n - grad * 0.25);
}

float calcAO(vec3 p, vec3 n) {
    float occ = 0.0, sca = 1.0;
    for(int i = 0; i < 5; i++) {
        float h = 0.02 + 0.15 * float(i) / 4.0;
        float d = map(p + h * n);
        occ += (h - d) * sca;
        sca *= 0.7;
    }
    return clamp(1.0 - 2.0 * occ, 0.0, 1.0);
}

// ---------------------------------------------------------------------
// Render Loop
// ---------------------------------------------------------------------

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
    // Darker, high-contrast background (vignette fade to pitch black)
    vec3 col = mix(vec3(0.02, 0.03, 0.05), vec3(0.0), smoothstep(0.0, 1.0, length(uv) * 1.5));
    
    // Camera
    float camTime = iTime * 0.2 * SPEED;
    vec3 ro = vec3(5.5 * cos(camTime), 3.0 + 1.5 * sin(camTime * 0.7), 5.5 * sin(camTime));
    vec3 ta = vec3(0.0, 0.0, 0.0);
    vec3 cw = normalize(ta - ro);
    vec3 cu = normalize(cross(cw, vec3(0.0, 1.0, 0.0)));
    vec3 cv = cross(cu, cw);
    vec3 rd = normalize(uv.x * cu + uv.y * cv + 1.5 * cw);
    
    // Raymarching
    float t = 0.0, glow = 0.0;
    for(int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + rd * t;
        float d = map(p);
        
        // Sharper, more intense volumetric bloom
        glow += exp(-d * 12.0) * 0.02;
        
        if(d < SURF_DIST) break;
        if(t > MAX_DIST) { t = MAX_DIST; break; }
        
        t += d * 0.75; 
    }
    
    // Lighting & Material
    if(t < MAX_DIST) {
        vec3 p = ro + rd * t;
        vec3 n_geo = calcNormal(p);
        vec3 n_bump = calcBumpNormal(p, n_geo, iTime * SPEED);
        
        vec3 v = -rd;
        float ao = calcAO(p, n_geo);
        
        // Fluid characteristics
        float a = atan(p.z, p.x);
        float flow = fluidHeight(p, a, iTime * SPEED);
        
        // Deeper base color for higher contrast
        vec3 baseCol = mix(vec3(0.005, 0.01, 0.03), vec3(0.0, 0.3, 0.8), smoothstep(0.3, 0.9, flow));
        
        // Sharper, brighter emission
        vec3 emitCol = vec3(0.1, 0.8, 1.0) * pow(flow, 4.0) * 4.0;
        
        // Cinematic Lighting Setup
        vec3 l1 = normalize(vec3(3.0, 4.0, -2.0)); // Main key light
        vec3 l2 = normalize(vec3(-3.0, -1.0, 2.0)); // Deep Rim/Fill
        
        // Harder shadows / faster falloff
        float dif1 = pow(max(dot(n_bump, l1), 0.0), 2.0);
        float dif2 = max(dot(n_bump, l2), 0.0) * 0.15; // Very subtle fill
        
        vec3 h1 = normalize(l1 + v);
        float spec1 = pow(max(dot(n_bump, h1), 0.0), 128.0) * 2.0; // Sharper, brighter spec
        float spec2 = pow(max(dot(n_bump, normalize(l2 + v)), 0.0), 64.0) * 0.5;
        
        // Intense Fresnel for wet edges
        float fresnel = pow(1.0 - max(dot(n_bump, v), 0.0), 5.0);
        
        // Combine with high contrast
        col = baseCol * (dif1 + dif2 + 0.02) * ao; // 0.02 is a very dark ambient floor
        col += spec1 * vec3(1.0) + spec2 * vec3(0.2, 0.5, 1.0);
        col += emitCol * ao;
        col += vec3(0.2, 0.7, 1.0) * fresnel * 1.5 * ao;
    }
    
    // Apply volumetric glow (boosted slightly)
    col += vec3(0.02, 0.4, 1.0) * glow * 1.5;
    
    // Cinematic ACES Tonemapping Approximation
    col = col * 1.2; // Exposure bump
    col = (col * (2.51 * col + 0.03)) / (col * (2.43 * col + 0.59) + 0.14);
    
    // Gamma Correction
    col = pow(col, vec3(1.0 / 2.2));
    
    // Subtle vignette to pull focus center
    col *= 1.0 - 0.4 * dot(uv, uv);
    
    fragColor = vec4(col, 1.0);
}
