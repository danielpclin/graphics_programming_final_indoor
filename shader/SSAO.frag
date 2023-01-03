#version 410 core

uniform sampler2D depthMap;
uniform sampler2D normalMap;
uniform sampler2D noiseMap;
uniform mat4 vp; // view projection matrix
uniform mat4 invvp; // inverse of vp

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
	vec4 position = invvp * vec4(vec3(vertexData.texcoord, depth) * 2.0 - 1.0, 1.0);
	position /= position.w; // unproject the input fragment to world space

	// get normal from normal map
	vec3 normal = texture(normalMap, vertexData.texcoord).rgb * 2.0 - 1.0;
	normal = normalize(normal);

	// get random vector from noise map
	vec3 randomvec = texture(noiseMap, vertexData.texcoord).rgb * 2.0 - 1.0;

	// compute local coordinate system (TBN)
	vec3 tangent = normalize(randomvec - normal * dot(randomvec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 tbn = mat3(tangent, bitangent, normal); // tangent to world

	const float radius = 0.5;
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
		if(sampleZ >= samplePoint.z + 0.025 || length(position - invPoint) > radius) {
			ao += 1.0;
		}
	}
	ao /= 64.0;
	fragAO = vec4(vec3(ao), 1.0);
}