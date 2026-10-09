using UnityEngine;

public class ColorMaterialProperty : MaterialPropertyExtensions.MaterialProperty
{
	private readonly Color Color;

	public ColorMaterialProperty(string name, Color color)
	{
		Name = name;
		Color = color;
	}

	public override void Apply(ref MaterialPropertyBlock block)
	{
		block.SetColor(Name, Color);
	}
}
