// Author:
// Title:

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2    u_resolution;

uniform vec4    u_color;
uniform vec3    u_vec;
uniform float   u_fader;


float stroke(float x, float size, float w) {
    float d = step(size, x+w*.5) - step(size, x-w*.5);
    return clamp(d, 0., 1.);
}

void main() {
    vec3 color = vec3(0.);
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
   
#ifdef MAGENTA
    color = vec3(1.0,0.0,1.0);
#else
    color = u_color.rgb;
#endif

    color.r += stroke(st.x, u_vec.x, 0.01);
    color.r += stroke(st.y, u_vec.y, 0.01);


    gl_FragColor = vec4(color,1.0);
}
