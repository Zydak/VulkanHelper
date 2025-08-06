struct Particle
{
    float2 Position;
    float2 Velocity;
};

#define POINTS_COUNT 10000

[[vk::binding(0, 0)]] RWStructuredBuffer<Particle> ParticlesBuffer;
[[vk::binding(1, 0)]] RWTexture2D<float4> RenderTarget;

float2 ComputeGravitationalForce(Particle p1, Particle p2)
{
    const float G = 6.67430e-11;
    const float massp1 = 5000.0f;
    const float massp2 = 5000.0f;

    const float2 r = p2.Position - p1.Position;
    const float distanceSquared = dot(r, r); 
    const float distance = sqrt(distanceSquared);
    if (distance < 0.001f) // Ignore small distances
        return float2(0.0f);
    const float e = 10.0f;
    const float magnitude = (G * massp1 * massp2 * sqrt(distanceSquared)) / pow(distanceSquared + e * e, 3.0f / 2.0f);
    return magnitude * (r / distance);
}

[shader("compute")]
[numthreads(32,1,1)]
void Main(uint3 threadId : SV_DispatchThreadID)
{
    uint index = threadId.x;
    if (index >= POINTS_COUNT) return;
    Particle particle = ParticlesBuffer[index];

    float2 force = float2(0.0f);
    for (int i = 0; i < POINTS_COUNT; ++i)
    {
        if (i == index) continue; // Skip self interaction

        Particle otherParticle = ParticlesBuffer[i];
        force += ComputeGravitationalForce(particle, otherParticle);
    }

    // Update particle's velocity and position based on the computed force
    particle.Velocity += force;
    particle.Position += particle.Velocity;

    // If touches boundary reset velocity
    if (particle.Position.x < 0.0f || particle.Position.x > 100.0f ||
        particle.Position.y < 0.0f || particle.Position.y > 100.0f)
    {
        particle.Velocity = float2(0.0f);
        particle.Position = clamp(particle.Position, float2(0.0f), float2(100.0f, 100.0f));
    }

    ParticlesBuffer[index] = particle;
}