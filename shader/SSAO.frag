#version 410 core

uniform sampler2D depthMap;
uniform sampler2D normalMap;
uniform sampler2D noiseMap;
uniform mat4 invvp;
uniform mat4 vp;
uniform vec2 noise_scale;

layout(std140) uniform SSAOKernals
{
	vec3 val[64];
} ssaoKernals;

in VertexData
{
	vec2 texcoord;
} vertexData;

layout(location = 0) out vec4 fragAO;

void main()                                                                                     
{                                                                                               
    float depth = texture(depthMap, vertexData.texcoord).r;
	if(depth == 1.0) {
		discard;
	}

	vec4 position = invvp * vec4(vec3(vertexData.texcoord, depth) * 2.0 - 1.0, 1.0);
	position /= position.w;

	vec3 normal = texture(normalMap, vertexData.texcoord).rgb;
	normal = normalize(normal);

	vec3 randomvec = texture(noiseMap, vertexData.texcoord * noise_scale).rgb * 2.0 - 1.0;
	vec3 tangent = normalize(randomvec - normal * dot(randomvec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 tbn = mat3(tangent, bitangent, normal);

	const float radius = 0.25;
	float ao = 0.0;
	for(int i = 0; i < 64; ++i)
	{
		vec3 sampleWorld = position.xyz + tbn * ssaoKernals.val[i] * radius;
		vec4 samplePoint = vp * vec4(sampleWorld, 1.0);
		samplePoint /= samplePoint.w;
		samplePoint = samplePoint * 0.5 + 0.5; // mapping to texture space
		float sampleZ = texture(depthMap, samplePoint.xy).r;
		vec4 invPoint = invvp * vec4(vec3(samplePoint.xy, sampleZ) * 2.0 - 1.0, 1.0);
		invPoint /= invPoint.w;
		// compare and range check
		if(sampleZ > samplePoint.z || length(position - invPoint) > radius)
			ao += 1.0;
	}
	ao /= 64.0;
	fragAO = vec4(vec3(ao), 1.0);

}                                                                                               
