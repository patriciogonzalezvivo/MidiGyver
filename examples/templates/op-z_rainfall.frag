#ifdef GL_ES
precision mediump float;
#endif

uniform float       u_kick;
uniform float       u_snare;
uniform float       u_perc;
uniform float       u_sample;
uniform float       u_bass;
uniform float       u_lead;
uniform float       u_arp;
uniform float       u_chord;

uniform sampler2D   u_buffer0;
uniform sampler2D   u_buffer1;
uniform vec2        u_resolution;
uniform float       u_time;

vec3 hsv2rgb(in vec3 hsb) {
    vec3 rgb = clamp(abs(mod(hsb.x * 6. + vec3(0., 4., 2.), 
                            6.) - 3.) - 1.,
                    0., 1.);
    return hsb.z * mix(vec3(1.), rgb, hsb.y);
}

float getChannel(int channel) {
    float tracks[8];
    tracks[0] = u_kick;
    tracks[1] = u_snare;
    tracks[2] = u_perc;
    tracks[3] = u_sample;
    tracks[4] = u_bass;
    tracks[5] = u_lead;
    tracks[6] = u_arp;
    tracks[7] = u_chord;

    return tracks[channel];
}

void main(void) {
    vec3 color = vec3(0.0);
    vec2 st = gl_FragCoord.xy / u_resolution;
    st.y = 1.0-st.y;
    vec2 pixel = 1.0/u_resolution;
    int col = int(floor(st.x * 8.0));
    float tone = getChannel(col);
    vec3 hue = hsv2rgb(vec3( float(col) / 8.0, 1., tone) );

#if defined(BUFFER_0)
    color = texture2D(u_buffer1, st).rgb;

#elif defined(BUFFER_1)
    color += texture2D(u_buffer0, st + pixel * vec2(0.0, 1.)).rgb * 0.999;

    float pct = 1.0 - smoothstep(0.0, 0.01, st.y);
    color += hue * pct;
    
#else
    color = texture2D(u_buffer1, st).rgb;
    color += hue;
#endif

    gl_FragColor = vec4(color, 1.0);
}

