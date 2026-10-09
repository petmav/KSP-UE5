using UnityEngine;

public class PartPointer : MonoBehaviour
{
	[SerializeField]
	private Part part;

	public Part PartReference => part;

	public void SetPart(Part part)
	{
		this.part = part;
	}

	internal Part GetPart()
	{
		return part;
	}
}
