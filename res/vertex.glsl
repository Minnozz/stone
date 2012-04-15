#version 120

attribute vec4 position;
attribute vec3 color;

varying vec3 col;

void main(){
	gl_Position = gl_ModelViewProjectionMatrix * position;
	col = color;
}
