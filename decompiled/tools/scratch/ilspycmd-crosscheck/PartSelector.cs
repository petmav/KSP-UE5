using UnityEngine;

public class PartSelector : MonoBehaviour
{
	private Part lastHoveredPart;

	private Part part;

	private Callback<Part> onSelect;

	private bool hover;

	private Color edgeHighlightColor;

	private Color edgeHoverColor;

	private Color highlightColor;

	private Color hoverColor;

	public Part Host => part;

	public static PartSelector Create(Part host, Callback<Part> onSelect, Color highlightColor, Color hoverColor)
	{
		PartSelector partSelector = host.gameObject.AddComponent<PartSelector>();
		partSelector.part = host;
		partSelector.onSelect = onSelect;
		partSelector.highlightColor = highlightColor;
		partSelector.hoverColor = hoverColor;
		partSelector.edgeHighlightColor = highlightColor;
		partSelector.edgeHoverColor = hoverColor;
		partSelector.Setup();
		return partSelector;
	}

	public static PartSelector Create(Part host, Callback<Part> onSelect, Color highlightColor, Color hoverColor, Color edgeHighlightColor, Color edgeHoverColor)
	{
		PartSelector partSelector = host.gameObject.AddComponent<PartSelector>();
		partSelector.part = host;
		partSelector.onSelect = onSelect;
		partSelector.highlightColor = highlightColor;
		partSelector.hoverColor = hoverColor;
		partSelector.edgeHighlightColor = edgeHighlightColor;
		partSelector.edgeHoverColor = edgeHoverColor;
		partSelector.Setup();
		return partSelector;
	}

	public void Dismiss()
	{
		Object.Destroy(this);
	}

	private void Setup()
	{
		part.SetHighlightDefault();
		part.SetHighlightType(Part.HighlightType.AlwaysOn);
		part.SetHighlight(active: true, recursive: false);
		part.SetHighlightColor(highlightColor);
	}

	private void OnMouseEntered()
	{
		hover = true;
		part.SetHighlightColor(hoverColor);
	}

	private void LateUpdate()
	{
		if (lastHoveredPart != part && Mouse.HoveredPart == part)
		{
			OnMouseEntered();
		}
		else if (lastHoveredPart == part && Mouse.HoveredPart != part)
		{
			OnMouseExited();
		}
		lastHoveredPart = Mouse.HoveredPart;
		if (hover && ((Mouse.Left.GetClick() && !Mouse.Left.WasDragging()) || (Mouse.Right.GetClick() && !Mouse.Right.WasDragging())))
		{
			onSelect(part);
		}
	}

	private void OnMouseExited()
	{
		hover = false;
		part.SetHighlightColor(highlightColor);
	}

	private void OnDestroy()
	{
		if (part != null)
		{
			part.SetHighlightDefault();
		}
	}
}
