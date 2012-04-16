#version 120

attribute vec4 position;
attribute vec3 color;
attribute float light;

varying vec3 v_color;
varying float v_light;

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * position;
	v_color = color;
	v_light = light;
}
