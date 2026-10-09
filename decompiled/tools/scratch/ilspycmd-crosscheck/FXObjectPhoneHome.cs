using UnityEngine;

public class FXObjectPhoneHome : MonoBehaviour
{
	public FXObject parent;

	protected void OnDestroy()
	{
		FXMonger.RemoveFXOjbect(parent);
	}
}
