using UnityEngine;

public static class MaterialPropertyExtensions
{
	public abstract class MaterialProperty
	{
		protected string Name;

		public abstract void Apply(ref MaterialPropertyBlock block);
	}

	private static MaterialPropertyBlock block = new MaterialPropertyBlock();

	public static void SetupProperties(this Renderer renderer, params MaterialProperty[] properties)
	{
		block.Clear();
		int num = properties.Length;
		for (int i = 0; i < num; i++)
		{
			properties[i].Apply(ref block);
		}
		renderer.SetPropertyBlock(block);
	}

	public static void UpdateProperties(this Renderer renderer, params MaterialProperty[] properties)
	{
		renderer.GetPropertyBlock(block);
		int num = properties.Length;
		for (int i = 0; i < num; i++)
		{
			properties[i].Apply(ref block);
		}
		renderer.SetPropertyBlock(block);
	}
}
