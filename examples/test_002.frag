

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2    u_resolution;
 
uniform float   u_fader;
uniform float   u_polygon;  // number of coorners on the polygon

#define TAU 6.2831853071795864769252867665590
#define PI 3.1415926535897932384626433832795

vec2 ratio(in vec2 st, in vec2 s) {
    return mix( vec2((st.x*s.x/s.y)-(s.x*.5-s.y*.5)/s.y,st.y),
                vec2(st.x,st.y*(s.y/s.x)-(s.y*.5-s.x*.5)/s.x),
                step(s.x,s.y));
}

float stroke(float x, float size, float w) {
    float d = step(size, x+w*.5) - step(size, x-w*.5);
    return clamp(d, 0., 1.);
}

float polySDF(vec2 st, float V) {
    st = st*2.-1.;
    float a = atan(st.x,st.y)+PI;
    float r = length(st);
    float v = TAU/V;
    return cos(floor(.5+a/v)*v-a)*r;
}

void main(void) {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    vec2 st = gl_FragCoord.xy / u_resolution;
    st = ratio(st, u_resolution); 

    float sdf = polySDF(st, u_polygon);
    color.rgb += stroke(sdf, u_fader, 0.1); 

    gl_FragColor = color;
}

