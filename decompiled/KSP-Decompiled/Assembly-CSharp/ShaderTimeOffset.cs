using UnityEngine;

[RequireComponent(typeof(Renderer))]
public class ShaderTimeOffset : MonoBehaviour
{
	public float frequency;

	public string valueName;

	[HideInInspector]
	public Material mat;

	private void Reset()
	{
		frequency = 1f;
		valueName = "_Offset";
	}

	private void Start()
	{
		mat = GetComponent<Renderer>().sharedMaterial;
	}

	private void Update()
	{
		mat.SetFloat(valueName, frequency * Time.realtimeSinceStartup);
	}
}
