

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2    u_resolution;
uniform float   u_time;

uniform float   u_pulse;    // pulse 
uniform float   u_polygon;  // number of coorners on the polygon

uniform float   u_knob00;   // Angle
uniform float   u_fader00;  // Shape (between circel and polygon)
uniform float   u_fader01;  // Size
uniform float   u_fader02;  // Width
uniform vec4    u_fader03;  // Color

uniform float   u_knob04;   // Angle
uniform vec3    u_fader04;  // Position
uniform vec4    u_fader05;  // Color

#define TAU 6.2831853071795864769252867665590
#define PI 3.1415926535897932384626433832795

vec2 ratio(in vec2 st, in vec2 s) {
    return mix( vec2((st.x*s.x/s.y)-(s.x*.5-s.y*.5)/s.y,st.y),
                vec2(st.x,st.y*(s.y/s.x)-(s.y*.5-s.x*.5)/s.x),
                step(s.x,s.y));
}

vec2 rotate(vec2 st, float a) {
    st = mat2(cos(a),-sin(a),
              sin(a),cos(a))*(st-.5);
    return st+.5;
}

#ifdef GL_OES_standard_derivatives
#extension GL_OES_standard_derivatives : enable
#endif
float aastep(float threshold, float value) {
    #ifdef GL_OES_standard_derivatives
    float afwidth = 0.7 * length(vec2(dFdx(value), dFdy(value)));
    return smoothstep(threshold-afwidth, threshold+afwidth, value);
    #else
    return step(threshold, value);
    #endif
}

float stroke(float x, float size, float w) {
    float d = aastep(size, x+w*.5) - aastep(size, x-w*.5);
    return clamp(d, 0., 1.);
}

float circleSDF(vec2 st) {
    return length(st-.5)*2.;
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

#ifdef DRAW_SHAPE
    float circle = circleSDF(st);
    float poly = polySDF(rotate(st, u_knob00), u_polygon);
    float sdf = fract( mix(circle, poly, u_fader00) - u_pulse * 0.5);

    color.rgb += u_fader03.rgb * stroke( sdf, u_fader01, u_fader02); 
#endif

#ifdef DRAW_AXIS
    vec2 st2 = rotate(st, u_knob04);
    st2 = st2 * -2.0 + 1.;
    float axis = (  stroke(st2.x, u_fader04.x, 3.0/u_resolution.x) +
                    stroke(st2.y, u_fader04.y, 3.0/u_resolution.y) );
    color.rgb = mix(color.rgb, u_fader05.rgb, clamp(axis, 0.0, 1.0)); 

#endif

    gl_FragColor = color;
}

