using UnityEngine;

[AddComponentMenu("PQuadSphere/Mods/Material/Quad Projective UV")]
public class PQSMod_MaterialQuadRelative : PQSMod
{
	private static int shaderPropertyUpMatrix;

	private static int shaderPropertyLocalMatrix;

	private static int shaderPropertySubDiv;

	private Matrix4x4 matUp;

	private Matrix4x4 matW2L;

	public override void OnSetup()
	{
		requirements = PQS.ModiferRequirements.UniqueMaterialInstances;
		shaderPropertyUpMatrix = Shader.PropertyToID("upMatrix");
		shaderPropertyLocalMatrix = Shader.PropertyToID("localMatrix");
		shaderPropertySubDiv = Shader.PropertyToID("_subdiv");
	}

	public override void OnQuadBuilt(PQ quad)
	{
		matUp = Matrix4x4.TRS(Vector3.zero, Quaternion.FromToRotation(Vector3.up, quad.positionPlanetRelative), Vector3.one);
		matW2L = Matrix4x4.TRS(quad.positionPlanet, Quaternion.FromToRotation(Vector3.up, quad.positionPlanetRelative), quad.quadScale);
		quad.GetComponent<Renderer>().material.SetMatrix(shaderPropertyUpMatrix, matUp);
		quad.GetComponent<Renderer>().material.SetMatrix(shaderPropertyLocalMatrix, matW2L);
		quad.GetComponent<Renderer>().material.SetFloat(shaderPropertySubDiv, quad.subdivision);
	}
}
