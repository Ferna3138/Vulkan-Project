#version 450

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 0) out vec4 outColor;


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
	mat4 modelMatrix; //projection + view * model
	mat4 normalMatrix;
} push;


void main(){
	vec3 diffuseLight = ubo.ambientLightColour.xyz * ubo.ambientLightColour.w;
	vec3 specularLight = vec3(0.0);
	vec3 surfaceNormal = normalize(fragNormalWorld);

	vec3 cameraPosWorld = ubo.invView[3].xyz;
	vec3 viewDirection = normalize(cameraPosWorld-fragPosWorld);

	for(int i = 0; i < ubo.numLights; i++){
		PointLight light = ubo.pointLights[i];
		vec3 directionToLight = light.position.xyz - fragPosWorld;
		float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
		directionToLight = normalize(directionToLight);

		float cosAngIncidence = max(dot(surfaceNormal, directionToLight),0);
		vec3 intensity = light.colour.xyz * light.colour.w * attenuation;
	
		diffuseLight += intensity * cosAngIncidence;

		//Specular light
		vec3 halfAngle = normalize(directionToLight + viewDirection);
		float blinnTerm = dot(surfaceNormal, halfAngle);
		blinnTerm = clamp(blinnTerm, 0, 1);
		blinnTerm = pow(blinnTerm,512.0); //Higher value gives a sharper reflection

		specularLight += intensity * blinnTerm;
	}

	outColor = vec4(diffuseLight * fragColour + specularLight * fragColour, 1.0);
}