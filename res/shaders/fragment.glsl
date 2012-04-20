#version 120

varying vec3 v_color;
varying float v_occlusion;

void main(void) {
	vec3 outside = vec3(1.0, 1.0, 1.0);
	vec3 inside = vec3(0.0, 0.0, 1.0);
	vec3 ambient = mix(outside, inside, v_occlusion);

	vec3 color = v_color * ambient;
	gl_FragColor = vec4(color, 1.0);
}
