#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColour;

struct PointLight{
	vec4 position; //Ignore w
	vec4 colour; // w -> intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo{
	mat4 projection;
	mat4 view;
	mat4 invView;
	vec4 ambientLightColour;
	PointLight pointLights[10]; //Hardcoded value in the point light array
	int numLights;
} ubo;

layout(push_constant) uniform Push{
	vec4 position;
	vec4 colour;
	float radius;
}push;


void main(){
	float dist = sqrt(dot(fragOffset,fragOffset));
	if(dist > 1){
		discard;
	}

	outColour = vec4(push.colour.xyz, 1.0);
}